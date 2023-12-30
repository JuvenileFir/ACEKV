#include <assert.h>
#include <inttypes.h>
#include <sched.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <random>
#include <thread>
#include <vector>
#include <rte_byteorder.h>
#include <rte_cycles.h>
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_lcore.h>
#include <rte_mbuf.h>
#include <rte_flow.h>

#include "xxhash.h"
#include "cityhash/citycrc_mod.h"
#include "directory/directory_client.h"
#include "benchmark.h"
#include "zipf.h"//只能产生theta<1的数据分布
// #include "zipfgo.h"//生成theta>1的数据分布

// #define ratio595
// #define ratio630
#define ratiomemc3
// #define ratioacekv

#define GHR

#ifndef GHR
#define TPUT
#endif
// #define micaP30
// #define micaP50
// #define micaP150
// #define micaP200
// #define micaP300
// #define micaP400
// #define micaP700
// #define micaP1000

// #define protoP30
// #define protoP50
// #define protoP150
// #define protoP200
// #define protoP300
// #define protoP400
// #define protoP700
// #define protoP1000

#ifdef GHR
#define micaP100
#define protoP100
#endif

#ifdef TPUT
#define micaP120
#ifndef ratiomemc3
#define protoP120
#else
#define protoP1000
#endif
#endif

#define SetSeq //set

// #define DynamicGet
/******************
*
* 类型:注意
* 内容:严格注意下面这个宏开没开！
* 添加者:bwb
* 时间:2023-07-06
*
******************/
// #define RANDRANGE100 //锁定100%范围(应当与SetSeq互斥)

typedef client::directory::DirectoryClient DirectoryClient;

uint64_t total_set[NUM_QUEUE];
uint64_t total_get[NUM_QUEUE];
uint64_t total_packet[NUM_QUEUE];
std::random_device rd;
std::mt19937 mt(rd());
int kkey_len, kval_len, get_len, set_len;
const int kkey_hash_len = 8;
#ifdef ratioacekv
uint64_t entry_num = 117440512;
#endif

#ifdef ratio630
uint64_t entry_num = 100663296;
#endif

#ifdef ratiomemc3
uint64_t entry_num = 134217728; // 33,554,432(1/4)
#endif

bool step, unif;
double zipf_theta, workload_ratio;
float get_ratio;
int num_pkt_20;
// #ifdef PRELOAD
int loading_mode = 1;
int server_threads = 0;
int kv_len = 8;
// #endif

uint32_t get_threshold;

// file descriptor for dump or read workload to/from file
// FILE *fp[NUM_QUEUE];

/********************************************************
*
* 类型:struct rte_ether_addr/网卡MAC地址常量
* 内容:
        Src 10.176.64.35    enp3s0f1     98:03:9b:8f:b0:11
        Dst 10.176.64.41    enp3s0f0np0  98:03:9b:8f:b1:c8
  详情:
        10.176.64.36  对应网卡enp2s0f0的MAC地址为04:3f:72:dc:26:24
                      对应网卡enp2s0f1的MAC地址为04:3f:72:dc:26:25
        10.176.64.35  enp3s0f0  对应网卡的MAC地址为98:03:9b:8f:b0:10
                      enp3s0f1  对应网卡的MAC地址为98:03:9b:8f:b0:11
        10.176.64.40  对应网卡b3:00.0/enp3s0f0的MAC地址为98:03:9b:8f:b5:c0
        10.176.64.41  对应网卡enp3s0f0np0的MAC地址为98:03:9b:8f:b1:c8
        10.176.64.41  对应网卡enp3s0f1np1的MAC地址为98:03:9b:8f:b1:c9
* 添加者:bwb
* 时间1:2022-11-24
* 时间2:2023-03-09
*
********************************************************/

rte_ether_addr S_Addr = 
    {{0x98, 0x03, 0x9b, 0x8f, 0xb0, 0x11}};
    // {{0x98, 0x03, 0x9b, 0x8f, 0xb0, 0x10}};
rte_ether_addr D_Addr = 
    {{0x98, 0x03, 0x9b, 0x8f, 0xb1, 0xc8}};
    // {{0x98, 0x03, 0x9b, 0x8f, 0xb5, 0xc0}};

/*********************
*
* 类型:IP常量
* 内容:SRC 10.176.64.36     DST 10.176.64.35    暂时用不到
* 添加者:bwb
* 时间:2022-11-24
*
**********************/
#define IP_SRC_ADDR ((10U << 24) | (176U << 16) | (64U << 8) | 35U)
#define IP_DST_ADDR ((10U << 24) | (176U << 16) | (64U << 8) | 41U)//改网卡
// #define IP_DST_ADDR ((10U << 24) | (176U << 16) | (64U << 8) | 40U)

// 14(ethernet header) + 20(IP header) + 8(UDP header)
const uint32_t kHeaderLen = 42;
const uint32_t kResCounterLen = 8;
#define ETHERNET_HEADER_LEN 14
// IP pkt header
#define IP_DEFTTL 64 /* from RFC 1340. */
#define IP_VERSION 0x40
#define IP_HDRLEN 0x05 /* default IP header length == five 32-bits words. */
#define IP_VHL_DEF (IP_VERSION | IP_HDRLEN)

struct rte_mempool *recv_mbuf_pool[NUM_QUEUE];
struct rte_mempool *send_mbuf_pool;//内存池
#define NUM_MAX_CORE 36
struct benchmark_core_statistics {
    uint64_t tx;
    uint64_t rx;
    uint64_t dropped;
    uint64_t err_ending;
    uint64_t set_succ;
    uint64_t get_succ;
    uint64_t set_fail;
    uint64_t get_fail;
    int enable;
} __rte_cache_aligned;
//保存了每个core整体情况
struct benchmark_core_statistics core_statistics[NUM_MAX_CORE];
#ifdef _DUMP_PKT
FILE *fp[NUM_MAX_CORE];
#endif

#define TIMER_MILLISECOND 2000000ULL /* around 1ms at 2 Ghz */
#define MAX_TIMER_PERIOD 86400       /* 1 day max */
static int64_t timer_period =
    5 * TIMER_MILLISECOND * 1000; /* default period is 5 seconds */

typedef struct context_s {
    unsigned int core_id;
    unsigned int queue_id;
} context_t;
std::vector<std::thread> workers;


uint8_t *mtod(struct rte_mbuf *mbuf) { /* for using this macro in gdb */
    return (uint8_t *)rte_pktmbuf_mtod(mbuf, uint8_t *);
}

static void check_pkt_content(struct rte_mbuf *pkt);
static void tx_preload(struct rte_mbuf **tx_mbufs);
// static int tx_pkt_load(char *, struct zipf_gen_state *, uint64_t *,uint32_t);
static int tx_pkt_load(char *, struct zipf_gen_state *,  std::uniform_int_distribution<uint64_t>, uint64_t *,uint32_t);

static void rx_pkt_process(struct rte_mbuf **, uint16_t);
static void print_stats(void);
void generate_udp_flow(uint16_t port_id, uint16_t rx_q, uint16_t src_port,
                       uint16_t src_mask, uint16_t dst_port,
                       uint16_t dst_mask);

static void print_pkt_header(struct rte_mbuf *pkt);
static inline void show_mac(const char *info, struct rte_ether_addr *addr);
static int pkt_filter(const struct rte_mbuf *pkt);
#ifdef _DUMP_PKT
static inline void pkt_content_dump(struct rte_mbuf *pkt);
void show_pkt(struct rte_mbuf *pkt);
#endif
void discover_servers(DirectoryClient* dir_client) {
  std::vector<std::string> server_list;
  while (true) {
    server_list = dir_client->get_server_list();
    if (server_list.size() >= 1) break;

    printf("warning: too few servers (%zu < %u); retrying...\n",
           server_list.size(), 1);
    sleep(1);
  }
}

void *rx_loop(context_t *context) {//接收函数 循环运行 
/*
To do:
1. receive requests from 40, and then parse the requests
2. set specific response to the buffer

*/
    printf("rx启动的core id = %d\n",context->core_id);
    uint32_t core_id = context->core_id;
    uint32_t queue_id = context->queue_id;
    uint16_t i, port = 0;
    struct rte_mbuf *rx_buf[20];

    unsigned long mask = 1 << core_id;
    if (sched_setaffinity(0, sizeof(unsigned long), (cpu_set_t *)&mask) < 0) {
        //参数1代表是本进程来绑定,参数2是mask字段的大小,参数3是运行进程的CPU
        printf("core id = %d\n", core_id);
        assert(0);
    }

    uint64_t prev_tsc = 0, diff_tsc, cur_tsc, timer_tsc = 0;
    core_statistics[core_id].enable = 1;
    int idx = 0;
    while (1) {
        idx &= 20;
        //定时输出对应统计信息。每5秒打印一次?
        if (queue_id == 0) {
            cur_tsc = rte_rdtsc();
            //rte_rdtsc() 返回的是自开机始CPU的周期数。
            diff_tsc = cur_tsc - prev_tsc;
            if (timer_period > 0) {
                timer_tsc += diff_tsc;
                if (unlikely(timer_tsc >= (uint64_t)timer_period)) {//timer_period=5s
                    /*
                    unlikely()等价于if (value),但value为假的可能性比较大
                    */
                    print_stats();
                    timer_tsc = 0;
                }
            }
            prev_tsc = cur_tsc;
        }
        const uint16_t nb_rx =
            rte_eth_rx_burst(port, queue_id, rx_buf + idx, BURST_SIZE);
            //从以太网设备的接收队列中检索输入数据包的突发。
            //检索到的数据包存储在rte_mbuf结构中，其指针在rx_pkts数组中提供。
            //返回值：实际检索到的数据包数，即有效提供给rx_pkts数组的rte_mbuf结构的指针数。
        // printf("core id:%d\tnb_rx:%d\n", core_id,nb_rx);

        core_statistics[core_id].rx += nb_rx;

        if (nb_rx != 0) {
            // printf("收到的core id = %d, total number = %lu\n",core_id, core_statistics[core_id].rx);

            // // ToDo: analyze requests respond quality
            // rx_pkt_process(rx_buf, nb_rx);
            
            /******************
            *
            * 类型:
            * 内容:怎么重复使用？
            * 添加者:bwb
            * 时间:2023-xx-xx
            *
            ******************/
            // for (i = 0; i < nb_rx; i++) {
            //     rte_pktmbuf_free(rx_buf[i]);
            //     //释放一个mbuf，释放过程即把mbuf归还到rte_mempool中.
            // }
        }
        idx++;
    }
}

void *tx_loop(context_t *context) {//发送数据包的主要函数
/*
将response组装并发送给client
To do:
1. each tx thread has a ring buffer
2. each thread has a loop to get response from the buffer and combine them into packets
3. send the packets to seatination
*/  
    printf("tx启动的core id = %d\n",context->core_id);

    const static uint32_t SEND_ROUND = 20;
    uint32_t core_id = context->core_id;
    uint32_t queue_id = context->queue_id;
    uint16_t i, port = 0;
    uint16_t pktlen = ETHERNET_MAX_FRAME_LEN;//TODO:类似于server，包填充完成后再填写data_len等属性(client之前一直是预先算好直接填)
    struct rte_mbuf *tx_bufs_pt[PKG_GEN_COUNT * SEND_ROUND];//用来存储packet?????
    // printf("core id = %d\tqueue_id id = %d\n", core_id, queue_id);
    unsigned long mask = 1 << core_id;
    if (sched_setaffinity(0, sizeof(unsigned long), (cpu_set_t *)&mask) < 0) {
        //int sched_setaffinity(pid_t pid, size_t cpusetsize, const cpu_set_t *mask);
        printf("core id = %d\n", core_id);
        assert(0);
    }

#ifdef PRELOAD
    /* update packet length for 100% SET operations in PRELOAD */
    // uint16_t pktlen = 1508;  // which is length_packet[0]
    pktlen = 1484;  // 100%SET,42+2+32×45?????仍然有问题需要解释！！

#endif
    /* for 1GB hash table, 512MB signature, 32bits, total is 128M = 2^29/2^2
     * = 2^27 load 80% of the hash table
     */
    const uint64_t total_cnt = (uint64_t)TOTAL_CNT;
    // uint64_t preload_cnt = PRELOAD_CNT;//目前为64M
    uint64_t preload_cnt = entry_num;//112 * 1048576;//已参考mica设置为192M(暂时改为112M)，值过大则会需要很长的生成时间
    uint64_t zipf_count = 0;
    if (tag_mask > 256) {
        #if defined(micaP100)
            zipf_count = entry_num; //8线程是否考虑这里 * 2？因为跑这个只是为了测吞吐量，不关心命中率……
        #elif defined(micaP30)
            zipf_count = entry_num; //5-95的get范围不变
        #elif defined(micaP50)
            zipf_count = entry_num;
        #elif defined(micaP120)
            zipf_count = (uint64_t)(entry_num * 1.2);
        #elif defined(micaP150)
            zipf_count = (uint64_t)(entry_num * 1.5);
        #elif defined(micaP200)
            zipf_count = entry_num * 2;
        #elif defined(micaP300)
            zipf_count = entry_num * 3;
        #elif defined(micaP400)
            zipf_count = entry_num * 4;
        #elif defined(micaP700)
            zipf_count = entry_num * 7;
        #elif defined(micaP1000)
            zipf_count = entry_num * 10;
        #endif
    } else {
        #if defined(protoP100)
            zipf_count = entry_num; //8线程是否考虑这里 * 2？因为跑这个只是为了测吞吐量，不关心命中率……
        #elif defined(protoP30)
            zipf_count = entry_num; //5-95的get范围不变
        #elif defined(protoP50)
            zipf_count = (uint64_t)(entry_num * 1.2);//for memc3
        #elif defined(protoP120)
            zipf_count = (uint64_t)(entry_num * 1.2);
        #elif defined(protoP150)
            zipf_count = (uint64_t)(entry_num * 1.5);
        #elif defined(protoP200)
            zipf_count = entry_num * 2;
        #elif defined(protoP300)
            zipf_count = entry_num * 3;
        #elif defined(protoP400)
            zipf_count = entry_num * 4;
        #elif defined(protoP700)
            zipf_count = entry_num * 7;
        #elif defined(protoP1000)
            zipf_count = entry_num * 10;
        #endif
    }
        printf("zipf_count: %lu\n", zipf_count);

    #ifdef RANDRANGE100
        zipf_count = entry_num;
        printf("FIXED RANGE 100%% get: %lu\n", zipf_count);
    #endif
    // uint64_t get_key = 1;  // set_key = 1;
    // zipf_count = static_cast<uint64_t>(zipf_count * 2 * (1 - get_ratio));

    struct zipf_gen_state zipf_state;
    mehcached_zipf_init(&zipf_state, zipf_count - 2,
                        (double)zipf_theta, (uint64_t)core_id);
    std::uniform_int_distribution<uint64_t> dist(1, zipf_count - 2);//

    
    /*
    To do: add the code for theta > 1
    */
   /*struct Zipf z;
   ///////NewZipf(&z,randSeed, 1.1, 1, max);
   NewZipf(&z,21, double(zipf_theta), 1, preload_cnt - 2);//seed,theta,min,max  xhj
   */
    struct rte_ether_hdr *ethh;
    struct rte_ipv4_hdr *ip_hdr;
    struct rte_udp_hdr *udph;
    for (int i = 0; i < PKG_GEN_COUNT * SEND_ROUND; i++) {//总共申请20个packet的内存空间，全部按照preload来初始化
        struct rte_mbuf *pkt = (struct rte_mbuf *)rte_pktmbuf_alloc(
            (struct rte_mempool *)send_mbuf_pool);
            //rte_pktmbuf_alloc向rte_mempool申请一个mbuf
        if (pkt == NULL) {
            rte_exit(EXIT_FAILURE,
                     "Cannot alloc storage memory in  port %" PRIu16 "\n",
                     port);
        }
        pkt->data_len = pktlen;
        // pkt->nb_segs = 1;  // nb_segs
        pkt->pkt_len = pkt->data_len;
        // pkt->ol_flags = PKT_TX_IPV4;  // ol_flags
        // pkt->vlan_tci = 0;            // vlan_tci//虚拟网络的标记位
        // pkt->vlan_tci_outer = 0;      // vlan_tci_outer
        pkt->l2_len = sizeof(struct rte_ether_hdr);
        pkt->l3_len = sizeof(struct rte_ipv4_hdr);
 
        ethh = (struct rte_ether_hdr *)rte_pktmbuf_mtod(pkt, unsigned char *);
        //指向mbuf中数据开头的宏。
        ethh->s_addr = S_Addr;
        ethh->d_addr = D_Addr;
        ethh->ether_type = rte_cpu_to_be_16(RTE_ETHER_TYPE_IPV4);//一致

        ip_hdr = (struct rte_ipv4_hdr *)((unsigned char *)ethh +
                                         sizeof(struct rte_ether_hdr));
        ip_hdr->version_ihl = IP_VHL_DEF;//一致
        ip_hdr->type_of_service = 0;//一致
        ip_hdr->fragment_offset = 0;//一致
        ip_hdr->time_to_live = IP_DEFTTL;//一致
        ip_hdr->next_proto_id = IPPROTO_UDP;//一致
        ip_hdr->packet_id = 0;//一致
        ip_hdr->total_length = rte_cpu_to_be_16(pktlen - sizeof(struct rte_ether_hdr));//将16位值从CPU顺序转换为大端and按照line 300更改
        ip_hdr->src_addr = rte_cpu_to_be_32(IP_SRC_ADDR);
        ip_hdr->dst_addr = rte_cpu_to_be_32(IP_DST_ADDR);

        // ip_hdr->hdr_checksum = rte_ipv4_cksum(ip_hdr);
        ip_hdr->hdr_checksum = 0;//一致

        udph = (struct rte_udp_hdr *)((unsigned char *)ip_hdr +
                                      sizeof(struct rte_ipv4_hdr));
        udph->src_port = queue_id;
        udph->dst_port = queue_id;
        udph->dgram_cksum = 0;//从mica新添加
        udph->dgram_len =
            rte_cpu_to_be_16((uint16_t)(pktlen - sizeof(struct rte_ether_hdr) -
                                        sizeof(struct rte_ipv4_hdr)));
        tx_bufs_pt[i] = pkt;//存储这些包
    }

#ifdef PRELOAD
    if (queue_id == 0) tx_preload(tx_bufs_pt);

    /* update `header`s for pktlen changed */
    pktlen = ETHERNET_MAX_FRAME_LEN;//恢复正常set流程的配置，但下面的for循环只进行一次?(因为现在有break，所以暂时不会出错(bushi))已暂时 * SEND_ROUND
    for (i = 0; i < PKG_GEN_COUNT * SEND_ROUND; i++) {
        tx_bufs_pt[i]->data_len = pktlen;
        tx_bufs_pt[i]->pkt_len = pktlen;

        ip_hdr = (struct rte_ipv4_hdr *)((char *)rte_pktmbuf_mtod(tx_bufs_pt[i],
                                                                  char *) +
                                         sizeof(struct rte_ether_hdr));
        ip_hdr->total_length =
            rte_cpu_to_be_16((uint16_t)(pktlen - sizeof(struct rte_ether_hdr)));//与line 269不同???
        ip_hdr->hdr_checksum = rte_ipv4_cksum(ip_hdr);

        udph = (struct rte_udp_hdr *)((unsigned char *)ip_hdr +
                                      sizeof(struct rte_ipv4_hdr));
        udph->dgram_len =
            rte_cpu_to_be_16((uint16_t)(pktlen - sizeof(struct rte_ether_hdr) -
                                        sizeof(struct rte_ipv4_hdr)));
        // udph->dgram_cksum = 0x5;
    }

    while (loading_mode == 1)
        ;
#endif

    /* (legacy) Different receivers use different keys start point */
    // This seems no use. The `get_key` is determined by `mehcached_zipf_next`.
    // get_key = (10000 * queue_id) % preload_cnt;
    // set_key = preload_cnt + queue_id * ((total_cnt - preload_cnt) /
    // NUM_QUEUE);
    static uint64_t seq[NUM_QUEUE];
    seq[queue_id] = 1 ; //考虑加上了tag判断，因此set初始值不作区分

    char *ptr = NULL;
    core_statistics[core_id].enable = 1;
    int T = num_pkt_20;
    int thread_id;

    while (T--) {
    // while (true) {
        
        for (i = 0; i < SEND_ROUND; i++) {//20个mbuf轮流发

            ptr = (char *)((char *)rte_pktmbuf_mtod(tx_bufs_pt[i],char *)
             + kHeaderLen);
                           //rte_pktmbuf_mtod函数获得指向mbuf中数据开头的宏。

            thread_id = tx_pkt_load(ptr, &zipf_state, dist, seq, queue_id);//组装packet
            if (thread_id == -1) break;
            int nb_tx = rte_eth_tx_burst(port, queue_id, tx_bufs_pt + i, 1);
            //rte_eth_tx_burst——物理口发包函数
            total_packet[queue_id]++;
            core_statistics[core_id].tx += nb_tx;
            if(step) break;
            #ifdef GHR
                usleep(600);//delay 600us
            #endif

        }
        if (thread_id == -1) break;
        
        if(step) break;
    }
        sleep(1);
    {
        ptr = (char *)((char *)rte_pktmbuf_mtod(tx_bufs_pt[0],char *) + kHeaderLen);
        
        *(uint16_t *)ptr = MEGA_JOB_FINISH;
        ptr += sizeof(uint16_t);
        *(uint16_t *)ptr = 0xFFFF;
        int nb_tx = rte_eth_tx_burst(port, queue_id, tx_bufs_pt, 1);
        printf("%u: 发送finish成功!\n", queue_id);
        printf("[INFO]total receive[ %d ]: %lu\n", core_id, core_statistics[core_id].rx);

    }

}

void port_init() {
    unsigned nb_ports;
    uint16_t portid = 0;
    /* Initialize the Environment Abstraction Layer (EAL). */
    int t_argc = 12;
    char *t_argv[] = {(char *)"./build/benchmark",
                      (char *)"-c",
                      (char *)"f",//8 4 2 1
                      (char *)"-n",
                      (char *)"1",
                      (char *)"--huge-unlink",
                      (char *)"-w",
                      (char *)"pci@0000:03:00.1",//改网卡，对41是.1，对40是.0
                      (char *)"--file-prefix",
                      (char *)"bwb",
                      (char *)"--socket-mem",
                      (char *)"1000"};//网卡端口
    //02:00.0 Ethernet controller: Mellanox Technologies MT27800 Family [ConnectX-5]
    int ret = rte_eal_init(t_argc, t_argv);
    if (ret < 0) rte_exit(EXIT_FAILURE, "Error with EAL initialization\n");
    printf("rte_eal_init的返回值 = %d\n",ret);
    //assert(0); 
    nb_ports = rte_eth_dev_count_avail();
    printf("WEN: There are %d device(s) available\n", nb_ports);
    
    /* Creates a new mempool in memory to hold the mbufs. */
    char str[10];
    for (int i = 0; i < NUM_QUEUE; i++) {
        sprintf(str, "RX_POOL_%d", i);
        recv_mbuf_pool[i] = rte_pktmbuf_pool_create(
            str, NUM_MBUFS * nb_ports, MBUF_CACHE_SIZE, 0,
            RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());
        if (recv_mbuf_pool[i] == NULL)
            rte_exit(EXIT_FAILURE, "Cannot create mbuf pool\n");
    }//新建了4个接收buf pool
    send_mbuf_pool = rte_pktmbuf_pool_create(
        "SEND_POOL", NUM_MBUFS * nb_ports, MBUF_CACHE_SIZE, 0,
        RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());
    if (send_mbuf_pool == NULL)
        rte_exit(EXIT_FAILURE, "Cannot create mbuf pool\n");
    //新建了1个发送buf pool
    // printf("Creates a new mempool in memory to hold the mbufs.Done\n");

    /* Initialize all ports. */
    const uint16_t rx_rings = NUM_QUEUE, tx_rings = NUM_QUEUE;//收发队列
    uint16_t nb_rxd = RX_RING_SIZE;
    uint16_t nb_txd = TX_RING_SIZE;
    uint16_t q;
    int retval;
    uint16_t port = 0;
    struct rte_eth_conf port_conf;//按照mica里面的进行修改
    struct rte_eth_txconf tx_conf;
    struct rte_eth_rxconf rx_conf;
    memset(&port_conf, 0, sizeof(rte_eth_conf));
    memset(&tx_conf, 0, sizeof(rte_eth_conf));
    memset(&rx_conf, 0, sizeof(rte_eth_conf));

    port_conf.rxmode.mq_mode = ETH_MQ_RX_NONE;
    port_conf.rxmode.max_rx_pkt_len = RTE_ETHER_MAX_LEN;
    port_conf.rxmode.split_hdr_size = 0;
    port_conf.rxmode.offloads = (DEV_RX_OFFLOAD_VLAN_FILTER | 
                                 DEV_RX_OFFLOAD_VLAN_STRIP);
    port_conf.txmode.mq_mode = ETH_MQ_TX_NONE;
    port_conf.txmode.offloads = (DEV_TX_OFFLOAD_VLAN_INSERT |
                                 DEV_TX_OFFLOAD_IPV4_CKSUM  |
                                 DEV_TX_OFFLOAD_UDP_CKSUM   |
                                 DEV_TX_OFFLOAD_TCP_CKSUM   |
                                 DEV_TX_OFFLOAD_SCTP_CKSUM  |
                                 DEV_TX_OFFLOAD_TCP_TSO);
    port_conf.fdir_conf.mode = RTE_FDIR_MODE_PERFECT;
    port_conf.fdir_conf.pballoc = RTE_FDIR_PBALLOC_64K;
    port_conf.fdir_conf.status = RTE_FDIR_NO_REPORT_STATUS;
    port_conf.fdir_conf.mask.dst_port_mask = 0xffff;
    port_conf.fdir_conf.drop_queue = 0;
    

    struct rte_eth_dev_info dev_info;
    retval = rte_eth_dev_info_get(port, &dev_info);
    if (retval < 0) {
        rte_exit(EXIT_FAILURE,
                 "Error during getting device (port %u) info: %s\n",
                  port, strerror(-retval));
    }
  	port_conf.txmode.offloads &= dev_info.tx_offload_capa;
    rx_conf = dev_info.default_rxconf;
    rx_conf.offloads = port_conf.rxmode.offloads;
    tx_conf = dev_info.default_txconf;
    tx_conf.offloads = port_conf.txmode.offloads;

    // rx_conf.rx_thresh.pthresh = 8;
    // rx_conf.rx_thresh.hthresh = 0;
    // rx_conf.rx_thresh.wthresh = 0;
    // rx_conf.rx_free_thresh = 0;
    // rx_conf.rx_drop_en = 0;
    // rx_conf.offloads = DEV_RX_OFFLOAD_CHECKSUM;
    // tx_conf.tx_thresh.pthresh = 32;
    // tx_conf.tx_thresh.hthresh = 0;
    // tx_conf.tx_thresh.wthresh = 0;
    // tx_conf.tx_free_thresh = 0;
    // tx_conf.tx_rs_thresh = 0;
        // printf("Initialize all ports.Done\n");

    /* Configure the Ethernet device. 配置以太网设备*/
    retval = rte_eth_dev_configure(port, rx_rings, tx_rings, &port_conf);
    if (retval < 0) {
        rte_exit(EXIT_FAILURE, "Cannot configure device: err=%d, port=%u\n",
                 retval, (unsigned)port);
    }
    /*检查Rx和Tx描述符的数量是否满足以太网设备信息中的描述符限制，否则将其调整为边界*/
    retval = rte_eth_dev_adjust_nb_rx_tx_desc(port, &nb_rxd, &nb_txd);//返回了两个1024
        // printf("Configure the Ethernet device.Done\n");
    // printf("rx: %u, tx: %u\n", nb_rxd, nb_txd);
    // printf("socket_id:%d\n", rte_eth_dev_socket_id(port));

    if (retval < 0) {
        rte_exit(EXIT_FAILURE, "Cannot configure adjust desc: port=%u\n",
                 (unsigned)port);
    }
    /* Allocate and set up RX queue(s) per Ethernet port. */
    for (q = 0; q < rx_rings; q++) {
        retval = rte_eth_rx_queue_setup(static_cast<uint8_t>(port),
                                        q, 512/* nb_rxd */,
                                        rte_eth_dev_socket_id(port),
                                        &rx_conf/* NULL */, recv_mbuf_pool[q]);
        /*分配并设置以太网设备的接收队列。*/
        if (retval < 0) {
            rte_exit(EXIT_FAILURE, "rte_eth_rx_queue_setup:err=%d, port=%u\n",
                     retval, (unsigned)port);
        }
    }

    /* Allocate and set up TX queue(s) per Ethernet port. */
    for (q = 0; q < tx_rings; q++) {
        retval = rte_eth_tx_queue_setup(static_cast<uint8_t>(port),
                                        q, 512/* nb_txd */,
                                        rte_eth_dev_socket_id(port),
                                        &tx_conf);
        /*分配并设置以太网设备的传输队列。*/
        if (retval < 0) {
            rte_exit(EXIT_FAILURE, "rte_eth_tx_queue_setup:err=%d, port=%u\n",
                     retval, (unsigned)port);
        }
    }
    // retval = rte_eth_dev_mac_addr_add(static_cast<uint8_t>(port), &S_Addr, 0);
    // if (retval < 0) {
    //   rte_exit(EXIT_FAILURE, "failed to register MAC address for port = %d\n",
    //            port);
    // }
    printf("starting port %" PRIu16 "...\n", port);

    // rte_eth_promiscuous_enable(port);//混杂模式开启

    retval = rte_eth_dev_start(port);
    if (retval < 0) {
      rte_exit(EXIT_FAILURE, "rte_eth_dev_start:err=%d, port=%u\n",
                retval, (unsigned)port);
    }

    struct rte_eth_link link;
    while (true) {
        printf("querying port %" PRIu16 "...\n", port);

        rte_eth_link_get(static_cast<uint8_t>(port), &link);

        if (!link.link_status) {
            printf("warning: port %" PRIu16 ": link down; retrying...\n", port);
            sleep(1);
            continue;
        }
        if (link.link_speed / 1000 < 10) {
            printf("warning: port %" PRIu16 ": low speed (current: %" PRIu32
                    " Gbps, minimum: %" PRIu32 " Gbps); retrying...\n",
                    port, link.link_speed / 1000, 10);
            sleep(1);
            continue;
        }
        break;
    }

    printf("port %" PRIu16 ": %" PRIu32 " Gbps (%s)\n", port,
           link.link_speed / 1000,
           (link.link_duplex == ETH_LINK_FULL_DUPLEX) ? ("full-duplex")
                                                      : ("half-duplex"));

    printf("Adding UDP filter for each endpoint.\n");

    for (uint16_t eid = 0; eid < NUM_QUEUE; eid++) {
      uint16_t queue_id = eid;
      uint16_t udp_port = eid;

    //   printf("port_id, queue_id, udp_port = %d %d %d\n", port, queue_id,
            //  udp_port);
      generate_udp_flow(port, queue_id, 0, EMPTY_MASK, udp_port, FULL_MASK);

    }

    

    //bwb: ↑ ↑ ↑ 相当于port_init()部分
}

int main(int argc, char *argv[]) {
    int ch;
    int run_time = 1, tag_power = 8;
    get_ratio = 0.5;
    zipf_theta = 0.99;
    workload_ratio = 100.0;
    kkey_len = 8;
    kval_len = 8;
    step = false;
    unif = false;
    num_pkt_20 = 5;
    while ((ch = getopt(argc, argv, "hsun:t:r:l:w:z")) != -1) {
        switch (ch) {
        case 'h':
            printf(" Usage: %s\t[-s only running one cycle (no arguments)]\n"
                "\t\t\t\t[-n <packet num * 20>]\n"
                // "\t\t\t\t[-t <running time(s)>]\n"
                "\t\t\t\t[-t <tag power>]\n"
                "\t\t\t\t[-r <ratio of GET>]\n"
                "\t\t\t\t[-l <KV length>(8:16:32:40:48:56:64:1024)]\n"
                "\t\t\t\t[-z <zipf theta> [0,1] ]\n", argv[0]);
            exit(0);
        case 's':
            step = true;
            break;
        case 'u':
            unif = true;
            break;
        case 'n':
            num_pkt_20 = atoi(optarg);
            break;
        case 't':
            // run_time = atoi(optarg);
            tag_power = atoi(optarg);
            break;
        case 'r':
            get_ratio = atof(optarg);
            break;
        case 'l':
            kv_len = atoi(optarg);
            if ((kv_len!=8)&&(kv_len!=16)&&(kv_len!=24)&&(kv_len!=32)&&
                (kv_len!=40)&&(kv_len!=48)&&(kv_len!=56)&&(kv_len!=64)&&
                (kv_len!=128)&&(kv_len!=256)&&(kv_len!=512)&&(kv_len!=1024)) { 
                printf("[ERROR] illegal kv length error!!\n");
                return 0;
            }
            break;
        case 'w':
            workload_ratio = atof(optarg);
            break;
        case 'z':
            // zipf_theta = atof(optarg);
            unif = false;
            break;
        default:
            printf("Error: unknown option: %c\n", (char)optopt);
            break;
        }
    }
    printf("[INFO] 发送Value Size为:%d Bytes\n",kv_len);
    printf("[INFO] workload_ratio为:%.2lf %%\n",workload_ratio);
    entry_num = (uint64_t)((double)entry_num * workload_ratio / 100);
    //memcached 的最大key是255个字符（255B）,val是1MB
    switch (kv_len) {
        case 8:
            kval_len = 8;
            break;
        case 16:
            kval_len = 16;
            break;
        case 24:
            kval_len = 24;
            break;
        case 32:
            kval_len = 32;
            break;
        case 40:
            kval_len = 40;
            break;
        case 48:
            kval_len = 48;
            break;
        case 56:
            kval_len = 56;
            break;
        case 64:
            kval_len = 64;
            break;
        case 128:
            kval_len = 128;
            break;
        case 256:
            kval_len = 256;
            break;
        case 512:
            kval_len = 512;
            break;
        case 1024:
            kval_len = 1024;
            break;
        default:
            printf("[ERROR] illegal kv length error!!\n");
            return 0;
    }

    set_len = kkey_len + kkey_hash_len + kval_len + 8;
    get_len = kkey_len + kkey_hash_len + 6;
    get_threshold = (uint32_t)(get_ratio * (double)((uint32_t)-1));
    if (unif) {
        printf("Distribution is !!!!!uniform!!!!!!\n");
    } else {
        printf("Distribution is !!!!!zipf!!!!!!!!!\n");
    }
    #ifdef DynamicGet
        printf("Dynamic Get!!!!!!!\n");
        #ifdef sw
            printf("Sliding Window Get!!!!!!!\n");
        #else
            printf("Sliding right-bound Get!!!!!!!\n");
        #endif
    #else
            printf("fixed Get!!!!!!!\n");
    #endif


    printf("get_ratio: %.2f\n", get_ratio);
    printf("zipf_theta: %.2f\n", zipf_theta);
    printf("tag power: %d\n", tag_power);
    tag_mask = ((uint64_t)1 << tag_power) - 1;

for (int i = 0; i< NUM_QUEUE; i++){
    total_set[i] = 0;
    total_get[i] = 0;
    total_packet[i] = 0;
}
    port_init();
    // pthread_t tid;
    // pthread_attr_t attr;
    // pthread_attr_init(&attr);
    // pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);//线程分离
    // printf("Allocate and set up TX queue(s) per Ethernet port.Done\n");

    DirectoryClient* dir_client = new DirectoryClient();

    // discover_servers(dir_client);

    context_t *context[NUM_QUEUE];//结构体中包含coreId和queueId
    context_t *context_rx[NUM_QUEUE];//结构体中包含coreId和queueId
    // for (int i = 0; i < rx_rings; i++) {
    //     //创建对应的线程，来执行tx_loop和rx_loop函数
    //     context = (context_t *)malloc(sizeof(context_t));
    //     context->core_id = (i << 1) + 1;//奇数 2i+1
    //     context->queue_id = i;
    //     if (pthread_create(&tid, &attr, (void *)rx_loop, (void *)context) != 0)
    //         perror("pthread_create error!!\n");
    // }

    // for (int i = 0; i < NUM_QUEUE; i++) {
    //     context_rx[i] = new context_t;
    //     context_rx[i]->core_id = i << 1;
    //     context_rx[i]->queue_id = i;
    //     workers.emplace_back(rx_loop, context_rx[i]);
    // }

    for (int i = 0; i < NUM_QUEUE; i++) {
        context[i] = new context_t;
        context[i]->core_id = i << 1;
        context[i]->queue_id = i;
        workers.emplace_back(tx_loop, context[i]);
    }
    
    /******************
    *
    * 类型:新增功能
    * 内容:实现自动根据发包量结束程序，而非手动指定sleep时间
    * 添加者:bwb
    * 时间:2023-04-13
    *
    ******************/
    for (auto &t : workers) 
        t.join();
    
    for (int i = 0; i< NUM_QUEUE; i++) printf("[INFO]total pkt[ %d ]: %ld\n", i, total_packet[i]);
    for (int i = 0; i< NUM_QUEUE; i++) printf("[INFO]total set[ %d ]: %ld\n", i, total_set[i]);
    for (int i = 0; i< NUM_QUEUE; i++) printf("[INFO]total get[ %d ]: %ld\n", i, total_get[i]);
    // for (int i = 0; i< NUM_QUEUE; i+=2) printf("[INFO]total get[ %d ]: %lu\n", i, core_statistics[i].rx);

    for (int i = 0; i < NUM_QUEUE; i++) {
        delete context[i];
    }
    return 0;
}

static void tx_preload(struct rte_mbuf **tx_mbufs) {
    printf("Going to insert %u keys, LOAD_FACTOR is %.2f\n", PRELOAD_CNT,
           LOAD_FACTOR);

    int i;
    uint64_t key_hash,set_key = 1;
    uint16_t payload_len,count;
    char *ptr = NULL;
    uint64_t preload_cnt = PRELOAD_CNT;
    struct rte_ether_hdr *ethh;
    struct rte_ipv4_hdr *ip_hdr;
    struct rte_udp_hdr *udph;
    struct rte_mbuf **m_table = tx_mbufs;
    /* preload the keys */
    // while (set_key < NUM_DEFINED_GET * 0.01 * total_cnt) {
    while (set_key < preload_cnt) {
        /* construct a send buffer */
        for (i = 0; i < PKG_GEN_COUNT; i++) {
            /* skip the packet header */
            ptr = (char *)((char *)rte_pktmbuf_mtod(m_table[i], char *) +
                           kHeaderLen);
            /* basic length = header len + ending mark len */
            payload_len = kHeaderLen + MEGA_END_MARK_LEN;
            count = 0;
            /* construct a packet */
            while (payload_len + set_len <= ETHERNET_MAX_FRAME_LEN) {
                count++;
                *(uint16_t *)ptr = MEGA_JOB_SET;
                ptr += sizeof(uint16_t); /* 2 bytes job type */
                *(uint16_t *)ptr = kkey_len;
                ptr += sizeof(uint16_t); /* 2 bytes key length */
                *(uint16_t *)ptr = kkey_hash_len;//2 byte
                ptr += sizeof(uint16_t);
                *(uint16_t *)ptr = kval_len;//2 byte
                ptr += sizeof(uint16_t);
                
                key_hash =XXH64(&set_key,kkey_len,1);

                *(uint64_t *)(ptr) = set_key;
                ptr += kkey_len;
                *(uint64_t *)(ptr) = key_hash;
                ptr += kkey_hash_len;
                *(uint64_t *)(ptr) = set_key + 1;
                ptr += kval_len;

                payload_len += set_len;

                set_key++;
                if (set_key >= preload_cnt) {
                    break;
                }
            }
            printf("PRE_SET:%u\n",count);
            assert(payload_len <= ETHERNET_MAX_FRAME_LEN);
            // Update pkt_len because there is a `bread` in the while loop
            m_table[i]->data_len = payload_len;
            m_table[i]->pkt_len = payload_len;

            ip_hdr = (struct rte_ipv4_hdr *)((char *)rte_pktmbuf_mtod(
                                                 m_table[i], char *) +
                                             sizeof(struct rte_ether_hdr));
            ip_hdr->total_length = rte_cpu_to_be_16(
                (uint16_t)(payload_len - sizeof(struct rte_ether_hdr)));
            ip_hdr->hdr_checksum = rte_ipv4_cksum(ip_hdr);

            udph = (struct rte_udp_hdr *)((unsigned char *)ip_hdr +
                                          sizeof(struct rte_ipv4_hdr));
            udph->dgram_len = rte_cpu_to_be_16(
                (uint16_t)(payload_len - sizeof(struct rte_ether_hdr) -
                           sizeof(struct rte_ipv4_hdr)));
            // udph->dgram_cksum = 0xf;
            /* write the ending mark */
            *(uint16_t *)ptr = 0xFFFF;
            // check_pkt_content(m_table[i]);
#ifdef _DUMP_PKT_SEND
            // show_pkt(m_table[i]);
            rte_pktmbuf_dump(fp[sched_getcpu()], m_table[i],
                             m_table[i]->pkt_len);
            // fprintf(fp[sched_getcpu()], "payload_len: %u  ", payload_len);
            // pkt_content_dump(m_table[i]);
#endif
            /* reduce insert speed */
            int k = 20000;
            while (k > 0) k--;
        }

        rte_eth_tx_burst(0, (uint16_t)0, m_table, PKG_GEN_COUNT);
        // if (set_key > 5000) sleep(100);
        // assert(0);
    }

    printf(
        " ==========================     Hash table has been loaded     "
        "========================== \n");

    loading_mode = 0;
}

static void check_pkt_content(struct rte_mbuf *pkt) {
    uint16_t pktlen = pkt->data_len - MEGA_END_MARK_LEN;
    char *ptr =
        (char *)((char *)rte_pktmbuf_mtod(pkt, char *) + kHeaderLen);
    for (int k = 0; k < pktlen; k += 24) {
        assert(*(uint64_t *)(ptr + 8) == (*(uint64_t *)(ptr + 16)) - 1);
    }
}

#ifdef _DUMP_PKT
static inline void pkt_content_dump(struct rte_mbuf *pkt) {
    int cnt = 0;
    uint16_t pktlen = pkt->data_len - kHeaderLen;
    uint16_t *ptr = (uint16_t *)((uint8_t *)rte_pktmbuf_mtod(pkt, uint8_t *) +
                                 kHeaderLen);
    fprintf(fp[sched_getcpu()], "pkt_len: %d\n", pktlen);
    for (uint16_t i = 0; i < pktlen - 2; i += 2) {
        fprintf(fp[sched_getcpu()], "%04x  ", *ptr);
        ptr++;
        if ((++cnt) % 10 == 0) fprintf(fp[sched_getcpu()], "\n");
    }
    fprintf(fp[sched_getcpu()], "END_MARK: %04x \n", *ptr);
    fprintf(fp[sched_getcpu()], "\n");
}

void show_pkt(struct rte_mbuf *pkt) {
    int pktlen = pkt->data_len - kHeaderLen;
    uint8_t *ptr = (uint8_t *)((uint8_t *)rte_pktmbuf_mtod(pkt, uint8_t *) +
                               kHeaderLen);
    // fprintf(fp[sched_getcpu()], "pkt_len: %d\n", pktlen);
    while (*(uint16_t *)ptr != 0xFFFF) {
        uint32_t key_len = *(uint16_t *)(ptr + PROTOCOL_TYPE_LEN);
        if (*(uint16_t *)ptr == MEGA_JOB_GET) {
            fprintf(
                fp[sched_getcpu()], "GET\t%lu\n",
                *(uint64_t *)(ptr + PROTOCOL_TYPE_LEN + PROTOCOL_KEYLEN_LEN));
            ptr += PROTOCOL_TYPE_LEN + PROTOCOL_KEYLEN_LEN + key_len;
        } else if (*(uint16_t *)ptr == MEGA_JOB_SET) {
            uint32_t val_len =
                *(uint16_t *)(ptr + PROTOCOL_TYPE_LEN + PROTOCOL_KEYLEN_LEN);
            fprintf(fp[sched_getcpu()], "SET\t%lu\t%lu\n",
                    *(uint64_t *)(ptr + PROTOCOL_HEADER_LEN),
                    *(uint64_t *)(ptr + PROTOCOL_HEADER_LEN + key_len));
            ptr += PROTOCOL_TYPE_LEN + PROTOCOL_KEYLEN_LEN +
                   PROTOCOL_VALLEN_LEN + key_len + val_len;
        }
        // fprintf(fp[sched_getcpu()], "\n");
    }
    fprintf(fp[sched_getcpu()], "END_MARK: %04x \n", *(uint16_t *)ptr);
    fprintf(fp[sched_getcpu()], "\n");
    fflush(fp[sched_getcpu()]);
}
#endif

static int tx_pkt_load(char *ptr, struct zipf_gen_state *zipf_state, std::uniform_int_distribution<uint64_t> dist,
                        uint64_t *seq,uint32_t t_id) {

    //将请求组装到一个packet中
    //参数1指向mempool的开始，参数三开始定义为1
    uint16_t *begin_ptr = (uint16_t *)ptr;
    bool set1024 = false, get1024 = false;
    int get1024count = 0, set1024count = 0, get_thres = 3, set_thres = 1;
    uint16_t tag/* , set_count = 0, get_count = 0 */;
    uint16_t payload_len = kHeaderLen + MEGA_END_MARK_LEN;
    uint64_t key_hash, get_key64, set_key64, set_val64, t_entry_num, get_limit, set_limit;

    switch (kv_len) {
        case 8://双26是50%做法，3-57是5-95做法（这种情况下的set包是32B，get包是22B）
            #ifdef ratio595
            get_thres = 57;//26/57
            set_thres = 3;//26/3
            #else
            // #ifdef ratio100set
            // get_thres = 0;//26/57
            // set_thres = 35;//26/3
            // #else
            get_thres = 26;//26/57
            set_thres = 26;//26/3
            #endif
            // #endif
            break;
        case 16:
            get_thres = 23;
            set_thres = 23;
            break;
        case 24:
            get_thres = 20;
            set_thres = 20;
            break;
        case 32:
            get_thres = 18;
            set_thres = 18;
            break;
        case 40:
            get_thres = 16;
            set_thres = 16;
            break;
        case 48:
            get_thres = 15;
            set_thres = 15;
            break;
        case 56:
            get_thres = 14;
            set_thres = 14;
            break;
        case 64:
            get_thres = 13;
            set_thres = 13;
            break;
        case 128:
            get_thres = 8;
            set_thres = 8;
            break;
        case 256:
            get_thres = 4;
            set_thres = 4;
            break;
        case 512:
            get_thres = 2;
            set_thres = 2;
            break;
        case 1024:
            break;
        default:
            printf("[ERROR] illegal kv length error!!\n");
            break;
    }

    if (tag_mask > 256) {
        #if defined(micaP100)
            t_entry_num = entry_num / 4;//100%
        #elif defined(micaP30)
            if (unif) {
                // t_entry_num = entry_num / 18;//7 57.% hit用
                t_entry_num = entry_num / 12;//tput用
            } else {
                // t_entry_num = entry_num / 4;//zipf全写没问题
                t_entry_num = entry_num / 3;//zipf全写没问题
            }
        #elif defined(micaP50)
            t_entry_num = entry_num / 8;//50%
            // t_entry_num = (uint64_t)(entry_num * 0.15);//60%
        #elif defined(micaP120)
            t_entry_num = (uint64_t)(entry_num * 0.3);//120%
        #elif defined(micaP150)
            t_entry_num = entry_num / 4 + entry_num / 8;//150%
        #elif defined(micaP200)
            t_entry_num = entry_num / 2;//200%
        #elif defined(micaP300)
            t_entry_num = entry_num / 2 + entry_num / 4;//300%
        #elif defined(micaP400)
            t_entry_num = entry_num;//400%
        #elif defined(micaP700)
            t_entry_num = entry_num + entry_num / 2 + entry_num / 4;//400%
        #elif defined(micaP1000)
            t_entry_num = entry_num * 2 + entry_num / 2;//1000%
        #endif
    } else {
        #if defined(protoP100)
            t_entry_num = entry_num / 4;//100%
        #elif defined(protoP30)
            if (unif) {
                // t_entry_num = entry_num / 18;//13 30.77%
                t_entry_num = entry_num / 3;//13 30.77%
                // t_entry_num = entry_num / 5;//13 30.77%
            } else {
                // t_entry_num = entry_num / 5;//zipf全写（看着点）没问题
                t_entry_num = entry_num / 2 + entry_num / 4;//zipf全写（看着点）没问题
            }
        #elif defined(protoP50)
            // t_entry_num = entry_num / 8;//50%
            t_entry_num = (uint64_t)(entry_num * 0.15);//60%
        #elif defined(protoP120)
            t_entry_num = (uint64_t)(entry_num * 0.3);//120%
        #elif defined(protoP150)
            t_entry_num = entry_num / 4 + entry_num / 8;//150%
        #elif defined(protoP200)
            t_entry_num = entry_num / 2;//200%
        #elif defined(protoP300)
            t_entry_num = entry_num / 2 + entry_num / 4;//300%
        #elif defined(protoP400)
            t_entry_num = entry_num;//400%
        #elif defined(protoP700)
            t_entry_num = entry_num + entry_num / 2 + entry_num / 4;
        #elif defined(protoP1000)
            t_entry_num = entry_num * 2 + entry_num / 2;//1000%
        #endif
    }
    //固定set量，根据get ratio调控get的发送数量上限
    get_limit = static_cast<uint64_t>(t_entry_num * get_ratio / (1 - get_ratio));
    set_limit = t_entry_num;
    
    // std::uniform_int_distribution<uint64_t> ortype(0, (uint32_t)-1);
    ::util::Rand op_type_rand(static_cast<uint64_t>(t_id) + 1000);
    // ::util::Rand op_type_rand(1000);
    char* get_key = (char *)malloc(kkey_len);
    char* set_key = (char *)malloc(kkey_len);
    memset(get_key, 0, kkey_len);
    memset(set_key, 0, kkey_len);
    //在一个包内，get请求是先type，然后keylen，然后key
    //set的话，先是type，然后klen，valueLen，然后key和value
    if (kv_len == 1024) {  
        uint32_t op_r1024 = op_type_rand.next_u32();
        bool is_get = op_r1024 <= get_threshold;
        if (total_get[t_id] >= get_limit && total_set[t_id] < set_limit) {
            is_get = false;
        } else if (total_set[t_id] >= set_limit && total_get[t_id] < get_limit) {
            is_get = true;
        } else if (total_set[t_id] >= set_limit && total_get[t_id] >= get_limit) {
            return -1;
        }
        //手动执行1set+1get
        if (is_get) {
            while (true) {
                if (unif) {
                    get_key64 = dist(mt);
                } else {
                    get_key64 = (uint64_t)mehcached_zipf_next(zipf_state) + 1;
                }

                memcpy(get_key, &get_key64, 8);
                key_hash = CityHash64(get_key, 8);
                tag = calc_tag(key_hash);
                if (!(tag % NUM_QUEUE == t_id)) continue;
                break;
            }

            total_get[t_id]++;

            *(uint16_t *)ptr = MEGA_JOB_GET;//operation type  2 Byte
            ptr += 2;
            *(uint16_t *)ptr = 8;
            ptr += 2;
            *(uint16_t *)ptr = 8;
            ptr += 2;
            *(uint64_t *)ptr = get_key64;
            ptr += 8;
            *(uint64_t *)ptr = key_hash;
            ptr += 8;

// ***************************************************************************

            while (true) {

                #ifdef SetSeq
                    set_key64 = *(uint64_t *)(seq + t_id);
                #else
                    if (unif) {
                        set_key64 = dist(mt);
                    } else {
                        set_key64 = (uint64_t)mehcached_zipf_next(zipf_state) + 1;
                    }
                #endif
                set_val64 = set_key64 + 1;

                memcpy(set_key, &set_key64, 8);
                key_hash = CityHash64(set_key, 8);
                tag = calc_tag(key_hash);
                if (!(tag % NUM_QUEUE == t_id)) {
                    #ifdef SetSeq
                        *(uint64_t *)(seq + t_id) = set_val64;
                    #endif
                    continue;
                }
                break;
            }

            total_set[t_id]++;

            *(uint16_t *)ptr = MEGA_JOB_SET; 
            ptr += 2;         
            *(uint16_t *)ptr = 8;
            ptr += 2;
            *(uint16_t *)ptr = 8;
            ptr += 2;
            *(uint16_t *)ptr = 1024;
            ptr += 2;
            *(uint64_t *)ptr = set_key64;
            ptr += 8;
            *(uint64_t *)ptr = key_hash;
            ptr += 8;

            memset(ptr, 0, 1024);//补0至满足位数
            memcpy(ptr, &set_val64, 8);

            ptr += 1024;
            
            #ifdef SetSeq
                *(uint64_t *)(seq + t_id) = set_val64;
            #endif


        } else {
            while (true) {

                #ifdef SetSeq
                    set_key64 = *(uint64_t *)(seq + t_id);
                #else
                    if (unif) {
                        set_key64 = dist(mt);
                    } else {
                        set_key64 = (uint64_t)mehcached_zipf_next(zipf_state) + 1;
                    }
                #endif
                set_val64 = set_key64 + 1;

                memcpy(set_key, &set_key64, 8);
                key_hash = CityHash64(set_key, 8);
                tag = calc_tag(key_hash);
                if (!(tag % NUM_QUEUE == t_id)) {
                    #ifdef SetSeq
                        *(uint64_t *)(seq + t_id) = set_val64;
                    #endif
                    continue;
                }
                break;
            }

            total_set[t_id]++;

            *(uint16_t *)ptr = MEGA_JOB_SET; 
            ptr += 2;         
            *(uint16_t *)ptr = 8;
            ptr += 2;
            *(uint16_t *)ptr = 8;
            ptr += 2;
            *(uint16_t *)ptr = 1024;
            ptr += 2;
            *(uint64_t *)ptr = set_key64;
            ptr += 8;
            *(uint64_t *)ptr = key_hash;
            ptr += 8;

            memset(ptr, 0, 1024);//补0至满足位数
            memcpy(ptr, &set_val64, 8);

            ptr += 1024;
            
            #ifdef SetSeq
                *(uint64_t *)(seq + t_id) = set_val64;
            #endif

// ***************************************************************************
            while (true) {
                if (unif) {
                    get_key64 = dist(mt);
                } else {
                    get_key64 = (uint64_t)mehcached_zipf_next(zipf_state) + 1;
                }

                memcpy(get_key, &get_key64, 8);
                key_hash = CityHash64(get_key, 8);
                tag = calc_tag(key_hash);
                if (!(tag % NUM_QUEUE == t_id)) continue;
                break;
            }

            total_get[t_id]++;

            *(uint16_t *)ptr = MEGA_JOB_GET;//operation type  2 Byte
            ptr += 2;
            *(uint16_t *)ptr = 8;
            ptr += 2;
            *(uint16_t *)ptr = 8;
            ptr += 2;
            *(uint64_t *)ptr = get_key64;
            ptr += 8;
            *(uint64_t *)ptr = key_hash;
            ptr += 8;

        }
        
        *(uint16_t *)ptr = 0xFFFF;
        // printf("set:%u\tget:%u\tlen:%u\n", set_count, get_count, payload_len);
        // printf("payload_len:%u, end: %u, begin: %u\n", payload_len, *(uint16_t *)ptr, *begin_ptr);
        // while((uint16_t *)ptr >= begin_ptr) {
        //     printf("ptr: %u\n", *begin_ptr);
        //     begin_ptr++;
        // }
        return t_id;

    }

    while(payload_len + get_len <= ETHERNET_MAX_FRAME_LEN) {
        uint32_t op_r = op_type_rand.next_u32();//mehcached_rand(&k)
        bool is_get = op_r <= get_threshold;
        if (total_set[t_id] >= set_limit) return -1;
        
        if (total_get[t_id] >= get_limit && total_set[t_id] < set_limit) {
            is_get = false;
        } else if (total_set[t_id] >= set_limit && total_get[t_id] < get_limit) {
            is_get = true;
        } else if (total_set[t_id] >= set_limit && total_get[t_id] >= get_limit) {
            return -1;
        }
        // if (kv_len >= 64 && set1024 && !get1024) is_get = true;
        // if (kv_len >= 64 && !set1024 && get1024) is_get = false;
        if (set1024 && !get1024) is_get = true;
        if (!set1024 && get1024) is_get = false;
        
        /******************
        *
        * 类型:
        * 内容:只set不get
        * 添加者:bwb
        * 时间:2023-12-12
        *
        ******************/
        // get1024 = true;
        // is_get = false;

        if(is_get){

            if (unif) {
                #ifdef DynamicGet
                uint64_t idx = 1;
                while (total_set[0] + total_set[1] + total_set[2] + total_set[3] > idx * (entry_num / 10)) idx++;
                idx -= 1;
                switch (idx) {
                    case 0:
                        get_key64 = dist1(mt);
                        break;
                    case 1:
                        get_key64 = dist1(mt);
                        break;
                    case 2:
                        get_key64 = dist2(mt);
                        break;
                    case 3:
                        get_key64 = dist3(mt);
                        break;
                    case 4:
                        get_key64 = dist4(mt);
                        break;
                    case 5:
                        get_key64 = dist5(mt);
                        break;
                    case 6:
                        get_key64 = dist6(mt);
                        break;
                    case 7:
                        get_key64 = dist7(mt);
                        break;
                    case 8:
                        get_key64 = dist8(mt);
                        break;
                    case 9:
                        get_key64 = dist9(mt);
                        break;
                    case 10:
                        get_key64 = dist10(mt);
                        break;
                    case 11:
                        get_key64 = dist11(mt);
                        break;
                    case 12:
                        get_key64 = dist12(mt);
                        break;
                    case 13:
                        get_key64 = dist13(mt);
                        break;
                    case 14:
                        get_key64 = dist14(mt);
                        break;
                    case 15:
                        get_key64 = dist15(mt);
                        break;
                    default:
                        printf("error dynamic get\n");
                        assert(0);
                        break;
                }
                #else
                    get_key64 = dist(mt);
                #endif
            } else {
                get_key64 = (uint64_t)mehcached_zipf_next(zipf_state) + 1;
            }

            memcpy(get_key, &get_key64, 8);
            // key_hash =XXH64(get_key,kkey_len,1);
            key_hash = CityHash64(get_key, kkey_len);
            tag = calc_tag(key_hash);
            
            /******************
            *
            * 类型:修改
            * 内容:注释了下一行以改单线程
            * 添加者:bwb
            * 时间:2023-04-12
            *
            ******************/
            if (!(tag % NUM_QUEUE == t_id)) continue;

            total_get[t_id]++;

            *(uint16_t *)ptr = MEGA_JOB_GET;//operation type  2 Byte
            ptr += 2;
            *(uint16_t *)ptr = kkey_len;//2 Byte
            ptr += 2;
            *(uint16_t *)ptr = kkey_hash_len;//2 Byte
            ptr += 2;
            *(uint64_t *)ptr = get_key64;
            ptr += kkey_len;//8
            *(uint64_t *)ptr = key_hash;
            ptr += kkey_hash_len;//8

            // if (kv_len == 16) {
            //     *(uint64_t *)(ptr) = get_key64;
            // } else {
            //     memcpy(ptr, get_key, kkey_len);
            // }

            // printf("tid:%d\tget:%ld\n", t_id, get_key64);
            payload_len += get_len;
            get1024count++;
            // if (kv_len >= 64 && get1024count == get_thres) get1024 = true;
            if (get1024count == get_thres) get1024 = true;
        } else {
            if(/* unlikely( */ETHERNET_MAX_FRAME_LEN - payload_len < set_len/* ) */) break;
            #ifdef SetSeq
                set_key64 = *(uint64_t *)(seq + t_id);
                // if (t_id == 0) printf("key : %ld\n", set_key64);

            #else
                if (unif) {
                    set_key64 = dist(mt);
                } else {
                    set_key64 = (uint64_t)mehcached_zipf_next(zipf_state) + 1;
                }
            #endif
            set_val64 = set_key64 + 1;
            memcpy(set_key, &set_key64, 8);
            // key_hash = XXH64(set_key, kkey_len, 1);
            key_hash = CityHash64(set_key, kkey_len);
            tag = calc_tag(key_hash);
            /******************
            *
            * 类型:修改
            * 内容:改单线程要注释以下4行
            * 添加者:bwb
            * 时间:2023-04-12
            *
            ******************/
            if (!(tag % NUM_QUEUE == t_id)) {
                #ifdef SetSeq
                    *(uint64_t *)(seq + t_id) = set_val64;
                #endif
                continue;
            }
            total_set[t_id]++;

            *(uint16_t *)ptr = MEGA_JOB_SET; //#define MEGA_JOB_SET 0x3  2 byte
            ptr += 2;         //向后移动2 byte 即操作类型 2 byte
            *(uint16_t *)ptr = kkey_len;      // 2 byte
            ptr += 2;
            *(uint16_t *)ptr = kkey_hash_len; // 2 byte
            ptr += 2;
            *(uint16_t *)ptr = kval_len; // 2 byte
            ptr += 2;
            
            *(uint64_t *)ptr = set_key64;//赋set key
            ptr += kkey_len;//向后移动key_len
            *(uint64_t *)ptr = key_hash;
            ptr += kkey_hash_len;//向后移动keyhash_len

            // if (kv_len == 16) {
            //     *(uint64_t *)(ptr) = set_key64;//16+16+16+16=64,前面正好用了64位，set的key从2开始递增
            //     ptr += kkey_len + kkey_hash_len;//向后移动KEY_LEN+kkey_hash_len
            //     *(uint64_t *)(ptr) = set_val64;//value是key+1
            // } else if (kv_len == 8) {//基本不用了
            //     *(uint32_t *)(ptr) = (uint32_t)set_key64;
            //     ptr += kkey_len + kkey_hash_len;
            //     *(uint32_t *)(ptr) = (uint32_t)set_val64;
            // } else {
            //     memcpy(ptr, set_key, kkey_len);
            //     ptr += kkey_len + kkey_hash_len;
            //     memcpy(ptr, &set_val64, 8);
            //     memset(ptr + 8, 0, kval_len - 8);//补0至满足位数
            // }

            if (kv_len == 8) {//实际上是value length = 8
                *(uint64_t *)ptr = set_val64;
            } else {
                memset(ptr, 0, kval_len);//补0至满足位数
                memcpy(ptr, &set_val64, 8);
            }
            ptr += kval_len;//向后移动value_len

            payload_len += set_len;
            
            #ifdef SetSeq
                *(uint64_t *)(seq + t_id) = set_val64;
            #endif
            set1024count++;
            // if (kv_len >= 64 && set1024count == set_thres) set1024 = true;
            if (set1024count == set_thres) set1024 = true;

            // printf("tid:%d\ttag:%d\tsetk:%10ld\tsetv:%10ld\n", t_id, tag, set_key64, set_val64);
        }
        // if (kv_len >= 64 && set1024 && get1024) break;
        if (set1024 && get1024) break;
    }
    // *(uint64_t *)(seq + t_id) = set_val64;//更新start_set_key字段
    /* pkt ending flag */
    *(uint16_t *)ptr = 0xFFFF;
    // printf("set:%u\tget:%u\tlen:%u\n", set_count, get_count, payload_len);
    // printf("payload_len:%u, end: %u, begin: %u\n", payload_len, *(uint16_t *)ptr, *begin_ptr);
    // while((uint16_t *)ptr >= begin_ptr) {
    //     printf("ptr: %u\n", *begin_ptr);
    //     begin_ptr++;
    // }
    return t_id;
}

static void rx_pkt_process(struct rte_mbuf **recv_mbufs, uint16_t n_pkts) {
    //对收到的包进行分析和处理，并将统计结果累加到对应的结构体中（每个核单独统计的）
    int core_id = (sched_getcpu() - 1) >> 1;
    char *ptr = NULL;
    for (uint16_t k = 0; k < n_pkts; k++) {
        if (pkt_filter(recv_mbufs[k])) continue;
#ifdef _DUMP_PKT_RECV
        print_pkt_header(recv_mbufs[k]);
        pkt_content_dump(recv_mbufs[k]);
#endif
        ptr = (char *)rte_pktmbuf_mtod(recv_mbufs[k], char *) +
              (recv_mbufs[k]->pkt_len - MEGA_END_MARK_LEN);
              //rte_pktmbuf_mtod:指向mbuf中数据开头的宏。
        if (*(uint16_t *)ptr != MEGA_PKT_END) {
            // print_pkt_header(recv_mbufs[k]);
            // pkt_content_dump(recv_mbufs[k]);
            core_statistics[core_id].err_ending++;
        }

        uint16_t key_len;
        uint32_t val_len;
        // printf("sched_getcpu() = %d  通过活动线程得到的core_id = %d\n",sched_getcpu(),core_id);

        // if (core_id == 0) {//原为core_id == 1
        // printf("收到第k个包,k = %d\n",k);
            char *w_ptr = (char *)rte_pktmbuf_mtod(recv_mbufs[k], char *) +
                          kHeaderLen + kResCounterLen;

            /*
             * The condition for jump out the while loop cannot be w_ptr == ptr.
             * Because the pkt may be paded to 64 bytes.
             */
            while (*(uint16_t *)w_ptr != MEGA_PKT_END) {
                if (*(uint16_t *)w_ptr == GET_SUCC) {
                    core_statistics[core_id].get_succ++;
                    key_len = *(uint16_t *)(w_ptr + PROTOCOL_TYPE_LEN);
                    val_len = *(uint32_t *)(w_ptr + PROTOCOL_TYPE_LEN +
                                            PROTOCOL_KEYLEN_LEN);
                    w_ptr += PROTOCOL_TYPE_LEN + PROTOCOL_KEYLEN_LEN +
                             PROTOCOL_VALLEN_LEN + key_len + val_len;
                } else if (*(uint16_t *)w_ptr == GET_FAIL) {
                    core_statistics[core_id].get_fail++;
                    w_ptr += SET_RESPOND_LEN;
                } else if (*(uint16_t *)w_ptr == SET_SUCC) {
                    core_statistics[core_id].set_succ++;
                    w_ptr += SET_RESPOND_LEN;
                } else if (*(uint16_t *)w_ptr == SET_FAIL) {
                    core_statistics[core_id].set_fail++;
                    w_ptr += SET_RESPOND_LEN;
                } else if (*(uint16_t *)w_ptr == GET_THREAD) {
                    w_ptr += SET_RESPOND_LEN;
                    server_threads = *(uint16_t *)w_ptr;
                    printf("成功解析Thread = %d\n", server_threads);

                } else {
                    core_statistics[core_id].err_ending++;
                    break;
                }
            }
        // }
    }
}

/* Print out statistics on packets dropped */
static void print_stats(void) {
    return;
    static uint64_t total_err_ending = 0;
    uint64_t total_packets_dropped, total_packets_tx, total_packets_rx;
    unsigned core_id, queue_id;

    total_packets_dropped = 0;
    total_packets_tx = 0;
    total_packets_rx = 0;

    const char clr[] = {27, '[', '2', 'J', '\0'};
    const char topLeft[] = {27, '[', '1', ';', '1', 'H', '\0'};

    /* Clear screen and move to top left */
    printf("%s%s", clr, topLeft);

    printf("\nPort statistics ====================================");

    for (core_id = 0; core_id < NUM_MAX_CORE; core_id++) {
        if (core_statistics[core_id].enable == 0) continue;
        printf(
            "\nStatistics for core %d ------------------------------"
            "    Packets sent: %11" PRIu64 "    Packets received: %11" PRIu64
            "    Packets dropped: %11" PRIu64,
            core_id, core_statistics[core_id].tx, core_statistics[core_id].rx,
            core_statistics[core_id].dropped);

        total_packets_dropped += core_statistics[core_id].dropped;
        total_packets_tx += core_statistics[core_id].tx;
        total_packets_rx += core_statistics[core_id].rx;
        total_err_ending += core_statistics[core_id].err_ending;

        //xhj 注释掉下面四行然后看流量
        /*
        core_statistics[core_id].err_ending = 0;
        core_statistics[core_id].dropped = 0;
        core_statistics[core_id].tx = 0;
        core_statistics[core_id].rx = 0;
        */
    }

    printf(
        "\nAggregate statistics ==============================="
        "\nTotal packets sent: %18" PRIu64
        "\nTotal packets received: %14" PRIu64
        "\nTotal packets dropped: %15" PRIu64
        "\nTotal packets err_end: %15" PRIu64,
        total_packets_tx, total_packets_rx, total_packets_dropped,
        total_err_ending);
    printf("\n====================================================\n");

    int sample_core_id = 3;
    for (sample_core_id = 0; sample_core_id < 4; sample_core_id++) {
        printf(
            "\nSample statistics [Core %d] ========================="
            "\nTotal packets set_succ on Core: %15" PRIu64//输出uint64
            "\nTotal packets set_fail on Core: %15" PRIu64
            "\nTotal packets get_succ on Core: %15" PRIu64
            "\nTotal packets get_fail on Core: %15" PRIu64,
            sample_core_id, core_statistics[sample_core_id].set_succ,
            core_statistics[sample_core_id].set_fail,
            core_statistics[sample_core_id].get_succ,
            core_statistics[sample_core_id].get_fail);
        printf("\n====================================================\n");
        core_statistics[sample_core_id].set_succ = 0;
        core_statistics[sample_core_id].set_fail = 0;
        core_statistics[sample_core_id].get_succ = 0;
        core_statistics[sample_core_id].get_fail = 0;
    }
}

static void print_pkt_header(struct rte_mbuf *pkt) {
    struct rte_ether_hdr *ethh;
    struct rte_ipv4_hdr *ip_hdr;
    struct rte_udp_hdr *udph;

    ethh = (struct rte_ether_hdr *)rte_pktmbuf_mtod(pkt, char *);
    show_mac("src MAC : ", &(ethh->s_addr));
    show_mac("dst MAC : ", &(ethh->d_addr));

    ip_hdr =
        (struct rte_ipv4_hdr *)((char *)ethh + sizeof(struct rte_ether_hdr));
    udph = (struct rte_udp_hdr *)((char *)ip_hdr + sizeof(struct rte_ipv4_hdr));
    printf("UDP port : src(%d) dst(%d)\n", udph->src_port, udph->dst_port);
}

static inline void show_mac(const char *info, struct rte_ether_addr *addr) {
#ifdef _DUMP_PKT
    fprintf(fp[sched_getcpu()],
#else
    printf(
#endif
            "%s %02" PRIx8 " %02" PRIx8 " %02" PRIx8 " %02" PRIx8 " %02" PRIx8
            " %02" PRIx8 "\n",
            info, (addr->addr_bytes)[0], (addr->addr_bytes)[1],
            (addr->addr_bytes)[2], (addr->addr_bytes)[3], (addr->addr_bytes)[4],
            (addr->addr_bytes)[5]);
}

static int pkt_filter(const struct rte_mbuf *pkt) {//检查包的网卡MAC地址与本机是否对应
    assert(pkt);
    struct rte_udp_hdr *udph =
        (struct rte_udp_hdr *)((uint8_t *)rte_pktmbuf_mtod(pkt, uint8_t *) +
                               34);
    // if (udph->dgram_cksum != 0xf0f0) return 1;
    struct rte_ether_hdr *ethh =
        (struct rte_ether_hdr *)rte_pktmbuf_mtod(pkt, unsigned char *);
    for (int i = 0; i < 6; i++) {
        if ((ethh->d_addr.addr_bytes[i] != S_Addr.addr_bytes[i]) ||
            (ethh->s_addr.addr_bytes[i] != D_Addr.addr_bytes[i])) {
            return 1;
        }
    }
    return 0;
} 

void generate_udp_flow(uint16_t port_id, uint16_t rx_q, uint16_t src_port,
                       uint16_t src_mask, uint16_t dst_port,
                       uint16_t dst_mask) {
  struct rte_flow_attr attr;
  struct rte_flow_item pattern[MAX_PATTERN_NUM];
  struct rte_flow_action action[MAX_ACTION_NUM];
  struct rte_flow_action_queue queue = {.index = rx_q};
  struct rte_flow_item_udp udp_spec;
  struct rte_flow_item_udp udp_mask;
  struct rte_flow* flow = NULL;

  struct rte_flow_error _error;
  struct rte_flow_error* error = &_error;

  int res;

  memset(pattern, 0, sizeof(pattern));
  memset(action, 0, sizeof(action));

  /*
   * set the rule attribute.
   * in this case only ingress packets will be checked.
   */
  memset(&attr, 0, sizeof(struct rte_flow_attr));
  attr.ingress = 1;

  /*
   * create the action sequence.
   * one action only,  move packet to queue
   */
  action[0].type = RTE_FLOW_ACTION_TYPE_QUEUE;
  action[0].conf = &queue;
  action[1].type = RTE_FLOW_ACTION_TYPE_END;

  /*
   * set the first level of the pattern (ETH).
   * since in this example we just want to get the
   * ipv4 we set this level to allow all.
   */
  // pattern[0].type = RTE_FLOW_ITEM_TYPE_ETH;

  /* 只有这部分和分流 IP 的有所区别 8< */

  /*
   * allow all IPV4 packets.
   */
  pattern[0].type = RTE_FLOW_ITEM_TYPE_IPV4;

  /*
   * setting the second level of the pattern (UDP).
   */
  memset(&udp_spec, 0, sizeof(struct rte_flow_item_udp));
  udp_spec.hdr.src_port = src_port;//rte_cpu_to_be_16(src_port);
  udp_spec.hdr.dst_port = dst_port;//rte_cpu_to_be_16(dst_port);

  memset(&udp_mask, 0, sizeof(struct rte_flow_item_udp));
  udp_mask.hdr.src_port = src_mask;//rte_cpu_to_be_16(src_mask);
  udp_mask.hdr.dst_port = dst_mask;//rte_cpu_to_be_16(dst_mask);

  pattern[1].type = RTE_FLOW_ITEM_TYPE_UDP;
  pattern[1].spec = &udp_spec;
  pattern[1].mask = &udp_mask;

  /* >8 区别部分结束 */

  /* the final level must be always type end */
  pattern[2].type = RTE_FLOW_ITEM_TYPE_END;

  res = rte_flow_validate(port_id, &attr, pattern, action, error);

  if (res) {
    printf("Flow can't be validated, res = %d\n", res);
  }

  if (!res) flow = rte_flow_create(port_id, &attr, pattern, action, error);

  if (!flow) {
    fprintf(stderr,
            "error: failed to add perfect filter entry on port %" PRIu16
            " (err=%s)\n",
            port_id, error->message ? error->message : "(no stated reason)");
    assert(false);
    return;
  }
//   printf("Flow is created, src_port:%u, dst_port:%u\n", src_port, dst_port);

  return;
}

/* void *GetThread(context_t *context) {
    printf("启动的core id = %d\n",context->core_id);
    uint32_t core_id = context->core_id;
    uint32_t queue_id = context->queue_id;
    uint16_t i, port = 0;
    uint16_t pktlen = 46;
    struct rte_mbuf *tx_bufs[1];

    unsigned long mask = 1 << core_id;
    if (sched_setaffinity(0, sizeof(unsigned long), (cpu_set_t *)&mask) < 0) {
        //参数1代表是本进程来绑定,参数2是mask字段的大小,参数3是运行进程的CPU
        printf("core id = %d\n", core_id);
        assert(0);
    }
    
    struct rte_ether_hdr *ethh;
    struct rte_ipv4_hdr *ip_hdr;
    struct rte_udp_hdr *udph;

    struct rte_mbuf *pkt = (struct rte_mbuf *)rte_pktmbuf_alloc(
        (struct rte_mempool *)send_mbuf_pool);
        //rte_pktmbuf_alloc向rte_mempool申请一个mbuf

    if (pkt == NULL)
        rte_exit(EXIT_FAILURE,
                    "Cannot alloc storage memory in port %" PRIu16 "\n", port);
    pkt->data_len = pktlen;
    pkt->nb_segs = 1;  // nb_segs
    pkt->pkt_len = pkt->data_len;
    pkt->ol_flags = PKT_TX_IPV4;  // ol_flags
    pkt->vlan_tci = 0;            // vlan_tci//虚拟网络的标记位
    pkt->vlan_tci_outer = 0;      // vlan_tci_outer
    pkt->l2_len = sizeof(struct rte_ether_hdr);
    pkt->l3_len = sizeof(struct rte_ipv4_hdr);

    ethh = (struct rte_ether_hdr *)rte_pktmbuf_mtod(pkt, unsigned char *);
    //指向mbuf中数据开头的宏。
    ethh->s_addr = S_Addr;
    ethh->d_addr = D_Addr;
    ethh->ether_type = rte_cpu_to_be_16(RTE_ETHER_TYPE_IPV4);

    ip_hdr = (struct rte_ipv4_hdr *)((unsigned char *)ethh +
                                        sizeof(struct rte_ether_hdr));
    ip_hdr->version_ihl = IP_VHL_DEF;
    ip_hdr->type_of_service = 0;
    ip_hdr->fragment_offset = 0;
    ip_hdr->time_to_live = IP_DEFTTL;
    ip_hdr->next_proto_id = IPPROTO_UDP;
    // ip_hdr->next_proto_id = IPPROTO_IP;
    ip_hdr->packet_id = 0;
    ip_hdr->total_length = rte_cpu_to_be_16(pktlen - sizeof(struct rte_ether_hdr));
    ip_hdr->src_addr = rte_cpu_to_be_32(IP_SRC_ADDR);
    ip_hdr->dst_addr = rte_cpu_to_be_32(IP_DST_ADDR);

    ip_hdr->hdr_checksum = rte_ipv4_cksum(ip_hdr);

    udph = (struct rte_udp_hdr *)((unsigned char *)ip_hdr +
                                    sizeof(struct rte_ipv4_hdr));
    udph->src_port = 3;
    udph->dst_port = 1;

    udph->dgram_len =
        rte_cpu_to_be_16((uint16_t)(pktlen - sizeof(struct rte_ether_hdr) -
                                    sizeof(struct rte_ipv4_hdr)));
    // udph->dgram_cksum = 0;

    tx_bufs[0] = pkt;//存储这些包

    char *ptr = NULL;
    while (1) {

        ptr = (char *)((char *)rte_pktmbuf_mtod(tx_bufs[0],char *) + kHeaderLen);

        *(uint16_t *)ptr = MEGA_JOB_THREAD;
        ptr += sizeof(uint16_t);
        *(uint16_t *)ptr = 0xFFFF;
        int nb_tx = rte_eth_tx_burst(port, 0, tx_bufs, PKG_GEN_COUNT);
        sleep(1);
        if (server_threads) break;
    }
}
 */