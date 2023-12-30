/* -*- Mode: C; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Thread management for memcached.
 */
#include "memcached.h"
#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sched.h>
#include <mutex>

#ifdef __sun
#include <atomic.h>
#endif

#define ITEMS_PER_ALLOC 64

/* An item in the connection queue. */
typedef struct conn_queue_item CQ_ITEM;
struct conn_queue_item {
    int               sfd;
    enum conn_states  init_state;
    int               event_flags;
    int               read_buffer_size;
    enum network_transport     transport;
    CQ_ITEM          *next;
};

/* A connection queue. */
typedef struct conn_queue CQ;
struct conn_queue {
    CQ_ITEM *head;
    CQ_ITEM *tail;
    pthread_mutex_t lock;
    pthread_cond_t  cond;
};

/* Lock for cache operations (item_*, assoc_*) */
pthread_mutex_t cache_lock;

/* Connection lock around accepting new connections */
pthread_mutex_t conn_lock = PTHREAD_MUTEX_INITIALIZER;

#if !defined(HAVE_GCC_ATOMICS) && !defined(__sun)
pthread_mutex_t atomics_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

/* Lock for global stats */
static pthread_mutex_t stats_lock;

/* Free list of CQ_ITEM structs */
static CQ_ITEM *cqi_freelist;
static pthread_mutex_t cqi_freelist_lock;

static pthread_mutex_t *item_locks;
/* size of the item lock hash table */
static uint32_t item_lock_count;
/* size - 1 for lookup masking */
static uint32_t item_lock_mask;

static LIBEVENT_DISPATCHER_THREAD dispatcher_thread;

/*
 * Each libevent instance has a wakeup pipe, which other threads
 * can use to signal that they've put a new connection on its queue.
 */
static LIBEVENT_THREAD *threads;

/*
 * Number of worker threads that have finished setting themselves up.
 */
static int init_count = 0;
static pthread_mutex_t init_lock;
static pthread_cond_t init_cond;


static void thread_libevent_process(int fd, short which, void *arg);

unsigned short refcount_incr(unsigned short *refcount) {
#ifdef HAVE_GCC_ATOMICS
    return __sync_add_and_fetch(refcount, 1);
#elif defined(__sun)
    return atomic_inc_ushort_nv(refcount);
#else
    unsigned short res;
    mutex_lock(&atomics_mutex);
    (*refcount)++;
    res = *refcount;
    pthread_mutex_unlock(&atomics_mutex);
    return res;
#endif
}

unsigned short refcount_decr(unsigned short *refcount) {
#ifdef HAVE_GCC_ATOMICS
    return __sync_sub_and_fetch(refcount, 1);
#elif defined(__sun)
    return atomic_dec_ushort_nv(refcount);
#else
    unsigned short res;
    mutex_lock(&atomics_mutex);
    (*refcount)--;
    res = *refcount;
    pthread_mutex_unlock(&atomics_mutex);
    return res;
#endif
}

void item_lock(uint32_t hv) {
#ifdef MEMC3_ASSOC_CHAIN 
    mutex_lock(&item_locks[hv & item_lock_mask]);
#endif
    mutex_lock(&item_locks[hv & item_lock_mask]);
    // pthread_mutex_lock(&item_locks[hv & item_lock_mask]);

}

void item_unlock(uint32_t hv) {
#ifdef MEMC3_ASSOC_CHAIN 
    pthread_mutex_unlock(&item_locks[hv & item_lock_mask]);
#endif
    mutex_unlock(&item_locks[hv & item_lock_mask]);
    // pthread_mutex_unlock(&item_locks[hv & item_lock_mask]);
}

/*
 * Initializes a connection queue.
 */
static void cq_init(CQ *cq) {
    pthread_mutex_init(&cq->lock, NULL);
    pthread_cond_init(&cq->cond, NULL);
    cq->head = NULL;
    cq->tail = NULL;
}

/*
 * Looks for an item on a connection queue, but doesn't block if there isn't
 * one.
 * Returns the item, or NULL if no item is available
 */
static CQ_ITEM *cq_pop(CQ *cq) {
    CQ_ITEM *item;

    pthread_mutex_lock(&cq->lock);
    item = cq->head;
    if (NULL != item) {
        cq->head = item->next;
        if (NULL == cq->head)
            cq->tail = NULL;
    }
    pthread_mutex_unlock(&cq->lock);

    return item;
}

/*
 * Adds an item to a connection queue.
 */
static void cq_push(CQ *cq, CQ_ITEM *item) {
    item->next = NULL;

    pthread_mutex_lock(&cq->lock);
    if (NULL == cq->tail)
        cq->head = item;
    else
        cq->tail->next = item;
    cq->tail = item;
    pthread_cond_signal(&cq->cond);
    pthread_mutex_unlock(&cq->lock);
}

/*
 * Returns a fresh connection queue item.
 */
static CQ_ITEM *cqi_new(void) {
    CQ_ITEM *item = NULL;
    pthread_mutex_lock(&cqi_freelist_lock);
    if (cqi_freelist) {
        item = cqi_freelist;
        cqi_freelist = item->next;
    }
    pthread_mutex_unlock(&cqi_freelist_lock);

    if (NULL == item) {
        int i;

        /* Allocate a bunch of items at once to reduce fragmentation */
        item = (CQ_ITEM *)malloc(sizeof(CQ_ITEM) * ITEMS_PER_ALLOC);
        if (NULL == item)
            return NULL;

        /*
         * Link together all the new items except the first one
         * (which we'll return to the caller) for placement on
         * the freelist.
         */
        for (i = 2; i < ITEMS_PER_ALLOC; i++)
            item[i - 1].next = &item[i];

        pthread_mutex_lock(&cqi_freelist_lock);
        item[ITEMS_PER_ALLOC - 1].next = cqi_freelist;
        cqi_freelist = &item[1];
        pthread_mutex_unlock(&cqi_freelist_lock);
    }

    return item;
}


/*
 * Frees a connection queue item (adds it to the freelist.)
 */
static void cqi_free(CQ_ITEM *item) {
    pthread_mutex_lock(&cqi_freelist_lock);
    item->next = cqi_freelist;
    cqi_freelist = item;
    pthread_mutex_unlock(&cqi_freelist_lock);
}


/*
 * Creates a worker thread.
 */
static void create_worker(void *(*func)(void *), void *arg, int i) {
    pthread_t       thread;
    pthread_attr_t  attr;
    int             ret;

#ifdef __linux__
    cpu_set_t cpuset;
    int coreid;
    // coreid = 6 + i;
    coreid = i;
    CPU_ZERO(&cpuset);
    CPU_SET(coreid, &cpuset);
    pthread_attr_init(&attr);
    pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpuset);  
#endif

    if ((ret = pthread_create(&thread, &attr, func, arg)) != 0) {
        fprintf(stderr, "Can't create thread: %s\n",
                strerror(ret));
        exit(1);
    }
}

/*
 * Sets whether or not we accept new connections.
 */
// void accept_new_conns(const bool do_accept) {
//     pthread_mutex_lock(&conn_lock);
//     do_accept_new_conns(do_accept);
//     pthread_mutex_unlock(&conn_lock);
// }
/****************************** LIBEVENT THREADS *****************************/

/*
 * Set up a thread's information.
 */
// static void setup_thread(LIBEVENT_THREAD *me) {
    
//     me->base = event_init();
//     if (! me->base) {
//         fprintf(stderr, "Can't allocate event base\n");
//         exit(1);
//     }

//     /* Listen for notifications from other threads */
//     event_set(&me->notify_event, me->notify_receive_fd,
//               EV_READ | EV_PERSIST, thread_libevent_process, me);
//     event_base_set(me->base, &me->notify_event);

//     if (event_add(&me->notify_event, 0) == -1) {
//         fprintf(stderr, "Can't monitor libevent notify pipe\n");
//         exit(1);
//     }

//     me->new_conn_queue = (conn_queue *)malloc(sizeof(struct conn_queue));
//     if (me->new_conn_queue == NULL) {
//         perror("Failed to allocate memory for connection queue");
//         exit(EXIT_FAILURE);
//     }
//     cq_init(me->new_conn_queue);

//     if (pthread_mutex_init(&me->stats.mutex, NULL) != 0) {
//         perror("Failed to initialize mutex");
//         exit(EXIT_FAILURE);
//     }

//     me->suffix_cache = cache_create("suffix", SUFFIX_SIZE, sizeof(char*),
//                                     NULL, NULL);
//     if (me->suffix_cache == NULL) {
//         fprintf(stderr, "Failed to create suffix cache\n");
//         exit(EXIT_FAILURE);
//     }
// }


/******************
*
* 类型:
* 内容:libevent主循环
* 添加者:bwb
* 时间:2023-xx-xx
*
******************/
/*
 * Worker thread: main event loop
 */
// static void *worker_libevent(void *arg) {
//     LIBEVENT_THREAD *me = ()arg;

// #ifdef __linux__
//     pthread_mutex_lock(&conn_lock);
//     fprintf(stderr, "bind worker thread (%ld) to core %d\n", syscall(SYS_gettid),  sched_getcpu());
//     pthread_mutex_unlock(&conn_lock);
// #endif

//     /* Any per-thread setup can happen here; thread_init() will block until
//      * all threads have finished initializing.
//      */

//     pthread_mutex_lock(&init_lock);
//     init_count++;
//     pthread_cond_signal(&init_cond);
//     pthread_mutex_unlock(&init_lock);

//     event_base_loop(me->base, 0);
//     return NULL;
// }


/*
 * Processes an incoming "handle a new connection" item. This is called when
 * input arrives on the libevent wakeup pipe.
 */

/* Which thread we assigned a connection to most recently. */
// static int last_thread = -1;

/*
 * Dispatches a new connection to another thread. This is only ever called
 * from the main thread, either during initialization (for UDP) or because
 * of an incoming connection.
 */
// void dispatch_conn_new(int sfd, enum conn_states init_state, int event_flags,
//                        int read_buffer_size, enum network_transport transport) {
//     CQ_ITEM *item = cqi_new();
//     int tid = (last_thread + 1) % settings.num_threads;

//     LIBEVENT_THREAD *thread = threads + tid;

//     last_thread = tid;

//     item->sfd = sfd;
//     item->init_state = init_state;
//     item->event_flags = event_flags;
//     item->read_buffer_size = read_buffer_size;
//     item->transport = transport;

//     cq_push(thread->new_conn_queue, item);

//     MEMCACHED_CONN_DISPATCH(sfd, thread->thread_id);
//     if (write(thread->notify_send_fd, "", 1) != 1) {
//         perror("Writing to thread notify pipe");
//     }
// }

/*
 * Returns true if this is the thread that listens for new TCP connections.
 */
// int is_listen_thread() {
//     return pthread_self() == dispatcher_thread.thread_id;
// }

/********************************* ITEM ACCESS *******************************/

/* 分配一个新item
 * Allocates a new item.
 */
item *item_alloc(char *key, size_t nkey, int flags, rel_time_t exptime, int nbytes, int *evict_flag) {
    item *it;
    /* do_item_alloc handles its own locks */
    it = do_item_alloc(key, nkey, flags, exptime, nbytes, evict_flag);
    return it;
}

/*
 * Returns an item if it hasn't been marked as expired,
 * lazy-expiring as needed.
 */
item *item_get(const char *key, const size_t nkey) {
    item *it;
    uint32_t hv;
    hv = hash(key, nkey, 0);
        item_lock(hv);
    it = do_item_get(key, nkey, hv);
    item_unlock(hv);

    return it;
}

//do touch就是get之后再检查一下有没有过期，似乎只能靠touch来触发
item *item_touch(const char *key, size_t nkey, uint32_t exptime) {
    item *it;
    uint32_t hv;
    hv = hash(key, nkey, 0);
    item_lock(hv);
    it = do_item_touch(key, nkey, exptime, hv);
    item_unlock(hv);
    return it;
}

/* 把item链到LRU和table里
 * Links an item into the LRU and hashtable.
 */
int item_link(item *item) {
    int ret;
    uint32_t hv;

    hv = hash(ITEM_key(item), item->nkey, 0);
    item_lock(hv);
    ret = do_item_link(item, hv);
    item_unlock(hv);
    return ret;
}

/*
 * Decrements the reference count on an item and adds it to the freelist if
 * needed.
 */
void item_remove(item *item) {
    uint32_t hv;
     hv = hash(ITEM_key(item), item->nkey, 0);

    item_lock(hv);
    do_item_remove(item);
    item_unlock(hv);
}

/*
 * Replaces one item with another in the hashtable.
 * Unprotected by a mutex lock since the core server does not require
 * it to be thread-safe.
 */
int item_replace(item *old_it, item *new_it, const uint32_t hv) {
    return do_item_replace(old_it, new_it, hv);
}

/* 解link
 * Unlinks an item from the LRU and hashtable.
 */
void item_unlink(item *item) {
    uint32_t hv;
    hv = hash(ITEM_key(item), item->nkey, 0);
    item_lock(hv);
    do_item_unlink(item, hv);
    item_unlock(hv);
}

/* 把项移动到LRU队列尾部，更新操作，使得其更不易被淘汰
 * Moves an item to the back of the LRU queue.
 */
void item_update(item *item) {
    uint32_t hv;
    hv = hash(ITEM_key(item), item->nkey, 0);

    item_lock(hv);
    do_item_update(item);
    item_unlock(hv);
}



/* 存一个item
 * Stores an item in the cache (high level, obeys set/add/replace semantics)
 */
enum store_item_type store_item(item *item, int comm/* , conn* c */) {
    enum store_item_type ret;
    uint32_t hv;

    hv = hash(ITEM_key(item), item->nkey, 0);
    item_lock(hv);
    ret = do_store_item(item, hv);
    item_unlock(hv);

    return ret;
}


enum store_item_type do_store_item(item *it, const uint32_t hv) {
    enum store_item_type stored;
    char *key = ITEM_key(it);
    item *old_it = do_item_get(key, it->nkey, hv);//typedef struct _stritem item
    //上面的get操作仅仅是取出hash table中已存在的一个key相等的项（且确实存在的话，把低数第4位赋1）
    if (old_it != NULL) {//该项已存在
        // printf("该项已存在\n");
        // return STORED;
        assert(old_it != it);
        //内存屏障:是一类同步屏障指令，是CPU或编译器在对内存随机访问的操作中的一个同步点
        //使得此点之前的所有读写操作都执行后才可以开始执行此点之后的操作。置于写之前，读之后。
        
        // before_write(old_it);

        #ifdef SINGLE_THREAD
            do_item_unlink_nolock(old_it, hv);//解锁旧项与hash table的连接（可能问题所在）
        #else
            do_item_unlink(old_it, hv);//加锁版unlink
        #endif
        //do_item_remove(old_it);  //这一行附加在了上一行的函数中（但是没执行）
        item_free(old_it);//（这个实际上是do_item_remove的实际执行）ITEM_SLABBED在这里或1
        
        // after_write(old_it);

        // assert((old_it->it_flags & ITEM_LINKED) == 0);//检查低第1位为0，对应do_item_unlink_nolock()里面的操作
        // assert((old_it->it_flags & ITEM_SLABBED) != 0);//检查低第3位为1，对应do_slabs_free()
        if ((old_it->it_flags & ITEM_LINKED) != 0) printf("link_flag:%d\n",old_it->it_flags);
        if ((old_it->it_flags & ITEM_SLABBED) == 0) printf("slab_flag:%d\n",old_it->it_flags);
        // after_write(old_it);//原来在这
    }
    //出来后要么说明没有连接旧项，要么已经断开连接了，方便接下来链接新项
    before_write(it);

    #ifdef SINGLE_THREAD
        int ret = do_item_link_nolock(it, hv);//写新item，并连接
    #else
        int ret = do_item_link(it, hv);//加锁版link
    #endif
    
    after_write(it);
    //if ((it->it_flags & ITEM_LINKED) != 1) printf("item unlink!!!!\n");

    if (ret == 0) {
        stored = NOT_STORED;
        // printf("not stored\n");
    } else {
        stored = STORED;
    }
    
    return  stored;
}


/*
 * 注意：这个的do没了，被注释了
 * Flushes expired items after a flush_all call
 */
void item_flush_expired() {
    mutex_lock(&cache_lock);
    do_item_flush_expired();
    pthread_mutex_unlock(&cache_lock);
}

/* 被注释了
 * Dumps part of the cache
 */
char *item_cachedump(unsigned int slabs_clsid, unsigned int limit, unsigned int *bytes) {
    char *ret;

    mutex_lock(&cache_lock);
    ret = do_item_cachedump(slabs_clsid, limit, bytes);
    pthread_mutex_unlock(&cache_lock);
    return ret;
}

/* 被注释了
 * Dumps statistics about slab classes
 */
void  item_stats(ADD_STAT add_stats, void *c) {
    mutex_lock(&cache_lock);
    do_item_stats(add_stats, c);
    pthread_mutex_unlock(&cache_lock);
}

/* 被注释了
 * Dumps a list of objects of each size in 32-byte increments
 */
void  item_stats_sizes(ADD_STAT add_stats, void *c) {
    mutex_lock(&cache_lock);
    do_item_stats_sizes(add_stats, c);
    pthread_mutex_unlock(&cache_lock);
}

/******************************* GLOBAL STATS ******************************/

void STATS_LOCK() {
    pthread_mutex_lock(&stats_lock);
}

void STATS_UNLOCK() {
    pthread_mutex_unlock(&stats_lock);
}

void threadlocal_stats_reset(void) {
    int ii, sid;
    for (ii = 0; ii < settings.num_threads; ++ii) {
        pthread_mutex_lock(&threads[ii].stats.mutex);

        threads[ii].stats.get_cmds = 0;
        threads[ii].stats.get_misses = 0;
        threads[ii].stats.touch_cmds = 0;
        threads[ii].stats.touch_misses = 0;
        threads[ii].stats.delete_misses = 0;
        threads[ii].stats.incr_misses = 0;
        threads[ii].stats.decr_misses = 0;
        threads[ii].stats.cas_misses = 0;
        threads[ii].stats.bytes_read = 0;
        threads[ii].stats.bytes_written = 0;
        threads[ii].stats.flush_cmds = 0;
        threads[ii].stats.conn_yields = 0;
        threads[ii].stats.auth_cmds = 0;
        threads[ii].stats.auth_errors = 0;

        for(sid = 0; sid < MAX_NUMBER_OF_SLAB_CLASSES; sid++) {
            threads[ii].stats.slab_stats[sid].set_cmds = 0;
            threads[ii].stats.slab_stats[sid].get_hits = 0;
            threads[ii].stats.slab_stats[sid].touch_hits = 0;
            threads[ii].stats.slab_stats[sid].delete_hits = 0;
            threads[ii].stats.slab_stats[sid].incr_hits = 0;
            threads[ii].stats.slab_stats[sid].decr_hits = 0;
            threads[ii].stats.slab_stats[sid].cas_hits = 0;
            threads[ii].stats.slab_stats[sid].cas_badval = 0;
        }

        pthread_mutex_unlock(&threads[ii].stats.mutex);
    }
}

void threadlocal_stats_aggregate(struct thread_stats *stats) {
    int ii, sid;

    /* The struct has a mutex, but we can safely set the whole thing
     * to zero since it is unused when aggregating. */
    memset(stats, 0, sizeof(*stats));

    for (ii = 0; ii < settings.num_threads; ++ii) {
        pthread_mutex_lock(&threads[ii].stats.mutex);

        stats->get_cmds += threads[ii].stats.get_cmds;
        stats->get_misses += threads[ii].stats.get_misses;
        stats->touch_cmds += threads[ii].stats.touch_cmds;
        stats->touch_misses += threads[ii].stats.touch_misses;
        stats->delete_misses += threads[ii].stats.delete_misses;
        stats->decr_misses += threads[ii].stats.decr_misses;
        stats->incr_misses += threads[ii].stats.incr_misses;
        stats->cas_misses += threads[ii].stats.cas_misses;
        stats->bytes_read += threads[ii].stats.bytes_read;
        stats->bytes_written += threads[ii].stats.bytes_written;
        stats->flush_cmds += threads[ii].stats.flush_cmds;
        stats->conn_yields += threads[ii].stats.conn_yields;
        stats->auth_cmds += threads[ii].stats.auth_cmds;
        stats->auth_errors += threads[ii].stats.auth_errors;

        for (sid = 0; sid < MAX_NUMBER_OF_SLAB_CLASSES; sid++) {
            stats->slab_stats[sid].set_cmds +=
                threads[ii].stats.slab_stats[sid].set_cmds;
            stats->slab_stats[sid].get_hits +=
                threads[ii].stats.slab_stats[sid].get_hits;
            stats->slab_stats[sid].touch_hits +=
                threads[ii].stats.slab_stats[sid].touch_hits;
            stats->slab_stats[sid].delete_hits +=
                threads[ii].stats.slab_stats[sid].delete_hits;
            stats->slab_stats[sid].decr_hits +=
                threads[ii].stats.slab_stats[sid].decr_hits;
            stats->slab_stats[sid].incr_hits +=
                threads[ii].stats.slab_stats[sid].incr_hits;
            stats->slab_stats[sid].cas_hits +=
                threads[ii].stats.slab_stats[sid].cas_hits;
            stats->slab_stats[sid].cas_badval +=
                threads[ii].stats.slab_stats[sid].cas_badval;
        }

        pthread_mutex_unlock(&threads[ii].stats.mutex);
    }
}

void slab_stats_aggregate(struct thread_stats *stats, struct slab_stats *out) {
    int sid;

    out->set_cmds = 0;
    out->get_hits = 0;
    out->touch_hits = 0;
    out->delete_hits = 0;
    out->incr_hits = 0;
    out->decr_hits = 0;
    out->cas_hits = 0;
    out->cas_badval = 0;

    for (sid = 0; sid < MAX_NUMBER_OF_SLAB_CLASSES; sid++) {
        out->set_cmds += stats->slab_stats[sid].set_cmds;
        out->get_hits += stats->slab_stats[sid].get_hits;
        out->touch_hits += stats->slab_stats[sid].touch_hits;
        out->delete_hits += stats->slab_stats[sid].delete_hits;
        out->decr_hits += stats->slab_stats[sid].decr_hits;
        out->incr_hits += stats->slab_stats[sid].incr_hits;
        out->cas_hits += stats->slab_stats[sid].cas_hits;
        out->cas_badval += stats->slab_stats[sid].cas_badval;
    }
}

/*
 * Initializes the thread subsystem, creating various worker threads.
 *
 * nthreads  Number of worker event handler threads to spawn
 * main_base Event base for main thread
 */
void thread_init(int nthreads) {
    int         i;
    int         power;
    
    /******************
    *
    * 类型:
    * 内容:初始化一堆锁
    * 添加者:bwb
    * 时间:2023-xx-xx
    *
    ******************/
    pthread_mutex_init(&cache_lock, NULL);
    pthread_mutex_init(&stats_lock, NULL);

    pthread_mutex_init(&init_lock, NULL);
    pthread_cond_init(&init_cond, NULL);

    pthread_mutex_init(&cqi_freelist_lock, NULL);
    cqi_freelist = NULL;
    //确定锁表大小？
    /* Want a wide lock table, but don't waste memory */
    if (nthreads < 3) {
        power = 10;
    } else if (nthreads < 4) {
        power = 11;
    } else if (nthreads < 5) {
        power = 12;//默认4个线程，走的这条设置
    } else {
        /* 8192 buckets, and central locks don't scale much past 5 threads */
        power = 13;
    }

    item_lock_count = ((unsigned long int)1 << (power));
    item_lock_mask  = item_lock_count - 1;

    item_locks = (pthread_mutex_t *)calloc(item_lock_count, sizeof(pthread_mutex_t));
    if (! item_locks) {
        perror("Can't allocate item locks");
        exit(1);
    }
    for (i = 0; i < item_lock_count; i++) {
        pthread_mutex_init(&item_locks[i], NULL);
    }
}
//     // added by Bin since libevent is not used for cache bench
//     if (main_base == NULL) {
//         //printf("as you know we are not using libevent\n");
//         return;
//     }

//     threads = calloc(nthreads, sizeof(LIBEVENT_THREAD));
//     if (! threads) {
//         perror("Can't allocate thread descriptors");
//         exit(1);
//     }

//     dispatcher_thread.base = main_base;
//     dispatcher_thread.thread_id = pthread_self();

//     for (i = 0; i < nthreads; i++) {
//         int fds[2];
//         if (pipe(fds)) {
//             perror("Can't create notify pipe");
//             exit(1);
//         }

//         threads[i].notify_receive_fd = fds[0];
//         threads[i].notify_send_fd = fds[1];
    
//     /******************
//     *
//     * 类型:
//     * 内容:设置线程
//     * 添加者:bwb
//     * 时间:2023-xx-xx
//     *
//     ******************/
//         setup_thread(&threads[i]);
//         /* Reserve three fds for the libevent base, and two for the pipe */
//         stats.reserved_fds += 5;
//     }
    
//     /******************
//     *
//     * 类型:
//     * 内容:真正开启线程，进入主循环的一行
//     * 添加者:bwb
//     * 时间:2023-xx-xx
//     *
//     ******************/
//     /* Create threads after we've done all the libevent setup. */
//     for (i = 0; i < nthreads; i++) {
//         create_worker(worker_libevent, &threads[i], i);
//     }

//     /* Wait for all the threads to set themselves up before returning. */
//     pthread_mutex_lock(&init_lock);
//     while (init_count < nthreads) {
//         pthread_cond_wait(&init_cond, &init_lock);
//     }
//     pthread_mutex_unlock(&init_lock);
// }
