#include "communication.h"
#include "memcached.h"
#include "memc3_util.h"
#include "stats.h"
#include "slabs.h"
#include "assoc_chain.h"
#include "assoc_cuckoo.h"
#include "items.h"
#include "hash.h"

#define RUN_MEMC3
// #include "directory/directory_client.h"

// typedef client::directory::DirectoryClient DirectoryClient;

struct rte_mempool *recv_mbuf_pool[THREAD_NUM];

struct rte_mempool *send_mbuf_pool;
struct rte_ether_addr s_addr = {{0x98, 0x03, 0x9b, 0x8f, 0xb1, 0xc8}};//41的03:00.0

typedef struct context_s {
  unsigned int core_id;
  unsigned int queue_id;
} context_t;

std::vector<std::thread> workers;
Piekv *m_piekv;
struct stats stats;
struct settings settings;
time_t process_started;     /* when the process was started */

uint32_t keyver_array[keyver_count];
unsigned int hashpower;
unsigned long  int hashsize;
extern unsigned int hash_items;

ub4 hashmask;
ub4 tagpower;
ub4 tagmask;
struct slab_rebalance slab_rebal;
volatile int slab_rebalance_signal;
void assoc2_init(/* const  */int hashpower_init);


void sigint_handler(int sig) {
  printf("run finish job......\n");
  while(*reinterpret_cast<uint32_t*>(m_piekv->thread_is_running_)) {
    for (int i = 0; i < THREAD_NUM; i++)
      // m_piekv->thread_is_running_[i] = 0;
      __sync_bool_compare_and_swap((volatile uint8_t *)&(m_piekv->thread_is_running_[i]), 1U, 0U);

  }
    __sync_bool_compare_and_swap((volatile uint32_t *)&(m_piekv->is_running_), 1U, 0U);

  // for (auto &t : workers) t.join();
  printf("\n");
  printf("valid count:           %5zu\n\n", m_piekv->all_table_stats_->count);
  printf("Set request:           %5zu\n\n", m_piekv->all_table_stats_->set_request);
  printf("Set success:           %5zu\n\n", m_piekv->all_table_stats_->set_success);
  printf("Get request:           %5zu\n\n", m_piekv->all_table_stats_->get_request);
  printf("Get found:             %5zu\n\n", m_piekv->all_table_stats_->get_found);
  printf("Get Hit Rate:          %5lf%%\n\n", m_piekv->all_table_stats_->get_found * 100. / m_piekv->all_table_stats_->get_request);
  printf("index load factor:    %.4lf %%\n", 100.0 * hash_items / bkt_size / hashsize/* stat->count * 100.0 / kSlotNum */);
  printf("index load factor:    %5lf%%\n\n", m_piekv->all_table_stats_->count * 100. / (double)(((ub4) 1 << settings.hashpower_init) * bkt_size)/* 117440512 */);//hashsize * num_bucket
  printf("slabs load factor:     %5lf%%\n\n", m_piekv->all_table_stats_->count * 64 * 100. / settings.maxbytes);

  // print_table_stats(&mytable);
  printf("[INFO] Everything works fine.\n");
  fflush(stdout);
  // TODO:free all shm_free_all();
    delete m_piekv;

  exit(EXIT_SUCCESS);
}

void generate_udp_flow(uint16_t port_id, uint16_t rx_q, uint16_t src_port,
                       uint16_t src_mask, uint16_t dst_port,
                       uint16_t dst_mask) {
  struct rte_flow_attr attr;
  struct rte_flow_item pattern[2];//MAX_PATTERN_NUM
  struct rte_flow_action action[MAX_ACTION_NUM];
  struct rte_flow* flow = NULL;
  struct rte_flow_action_queue queue;// = {.index = rx_q };
  queue.index = rx_q;
  struct rte_flow_item_udp udp_spec;
  struct rte_flow_item_udp udp_mask;

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

  // /* 只有这部分和分流 IP 的有所区别 8< */

  /*
   * allow all IPV4 packets.
   */
  pattern[0].type = RTE_FLOW_ITEM_TYPE_IPV4;

  /*
   * setting the second level of the pattern (UDP).
   */
  memset(&udp_spec, 0, sizeof(struct rte_flow_item_udp));
  memset(&udp_mask, 0, sizeof(struct rte_flow_item_udp));
  udp_spec.hdr.src_port = src_port;
  udp_spec.hdr.dst_port = dst_port;

  udp_mask.hdr.src_port = src_mask;//rte_cpu_to_be_16(src_mask);
  udp_mask.hdr.dst_port = dst_mask;//rte_cpu_to_be_16(dst_mask);

  pattern[1].type = RTE_FLOW_ITEM_TYPE_UDP;
  pattern[1].spec = &udp_spec;
  pattern[1].mask = &udp_mask;

  // /* >8 区别部分结束 */

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
  } else {
    printf("Flow is created with src_port = %u, dst_port = %u\n", src_port, dst_port);
  }

  return;
}

void settings_init(size_t maxbytes_, int hashsize) {
    printf("maxbytes:%zu\n", maxbytes_);
    settings.use_cas = true;
    settings.access = 0700;
    settings.port = 11212;//11211
    settings.udpport = 11212;//11211
    /* By default this string should be NULL for getaddrinfo() */
    settings.inter = NULL;
    
    /******************
    *
    * 类型:
    * 内容:改出2/3的slab大小
    * 添加者:bwb
    * 时间:2023-12-22
    *
    ******************/
    settings.maxbytes = maxbytes_ * 2 / 3;//(size_t)1 << 33; //2048 * 1024 * 1024 - 1; /* default is 64MB */ //1G可以，2G就报alloc错？
    settings.maxconns = 1024;         /* to limit connections-related memory to about 5MB */
    settings.verbose = 0;
    settings.oldest_live = 0;
    settings.evict_to_free = 1;       /* push old items out of cache when memory runs out */
    settings.socketpath = NULL;       /* by default, not using a unix socket */
    settings.factor = 1.25;
    settings.chunk_size = 48;         /* space for a modest key and value */
    settings.num_threads = THREAD_NUM;         /* N workers */
    settings.num_threads_per_udp = 0;
    settings.prefix_delimiter = ':';
    settings.detail_enabled = 0;
    settings.reqs_per_event = 20;
    settings.backlog = 1024;
    settings.binding_protocol = negotiating_prot;
    settings.item_size_max = 1024 * 1024; /* The famous 1MB upper limit. */
    settings.maxconns_fast = false;
    settings.hashpower_init = hashsize; //default 0 /25
    settings.slab_reassign = false;
    settings.slab_automove = false;
}

void stats_init(void) {
    stats.curr_items = stats.total_items = stats.curr_conns = stats.total_conns = stats.conn_structs = 0;
    stats.get_cmds = stats.set_cmds = stats.get_hits = stats.get_misses = stats.evictions = stats.reclaimed = 0;
    stats.touch_cmds = stats.touch_misses = stats.touch_hits = stats.rejected_conns = 0;
    stats.curr_bytes = stats.listen_disabled_num = 0;
    stats.hash_power_level = stats.hash_bytes = stats.hash_is_expanding = 0;
    stats.expired_unfetched = stats.evicted_unfetched = 0;
    stats.slabs_moved = 0;
    stats.accepting_conns = true; /* assuming we start in this state. */
    stats.slab_reassign_running = false;

    /* make the time we started always be 2 seconds before we really
       did, so time(0) - time.started is never zero.  if so, things
       like 'settings.oldest_live' which act as booleans as well as
       values are now false in boolean context... */
    process_started = time(0) - 2;
    stats_prefix_init();
}

void port_init() {
  unsigned nb_ports;
  /* Initialize the Environment Abstraction Layer (EAL). */
  int t_argc = 8;
  char *t_argv[] = {(char *)"./build/benchmark",
                    (char *)"-c",
                    (char *)"f",
                    (char *)"-n",
                    (char *)"1",
                    (char *)"--huge-unlink",
                    (char *)"-w",
                    (char *)"pci@0000:03:00.0"};
  
  int ret = rte_eal_init(t_argc, t_argv);

  if (ret < 0) rte_exit(EXIT_FAILURE, "Error with EAL initialization\n");

  nb_ports = rte_eth_dev_count_avail();
  printf("WEN: There are %d device(s) available\n", nb_ports);

  /* Creates a new mempool in memory to hold the mbufs. */
  char str[15];
  for (uint32_t i = 0; i < THREAD_NUM; i++) {
    sprintf(str, "RX_POOL_%d", i);
    recv_mbuf_pool[i] = rte_pktmbuf_pool_create(str, NUM_MBUFS * nb_ports, MBUF_CACHE_SIZE, 0,
                                                RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());
    if (recv_mbuf_pool[i] == NULL) rte_exit(EXIT_FAILURE, "Cannot create mbuf pool\n");
  }
  send_mbuf_pool = rte_pktmbuf_pool_create("SEND_POOL", NUM_MBUFS * nb_ports, MBUF_CACHE_SIZE, 0,
                                           RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());
  if (send_mbuf_pool == NULL) rte_exit(EXIT_FAILURE, "Cannot create mbuf pool\n");

  /* Initialize all ports. */
  const uint16_t rx_rings = THREAD_NUM, tx_rings = THREAD_NUM;
  uint16_t nb_rxd = RX_RING_SIZE;
  uint16_t nb_txd = TX_RING_SIZE;
  uint16_t q;
  int retval;
  uint16_t port = 0;
  struct rte_eth_conf port_conf;
  struct rte_eth_txconf tx_conf;
  struct rte_eth_rxconf rx_conf;
  memset(&port_conf, 0, sizeof(rte_eth_conf));
  memset(&tx_conf, 0, sizeof(rte_eth_conf));
  memset(&rx_conf, 0, sizeof(rte_eth_conf));
  // port_conf.rxmode.mq_mode = ETH_MQ_RX_NONE;//ETH_MQ_RX_RSS
  // // port_conf.rxmode.mq_mode = ETH_MQ_RX_RSS;
  // port_conf.rxmode.max_rx_pkt_len = RTE_ETHER_MAX_LEN;
  // port_conf.rxmode.offloads = (DEV_RX_OFFLOAD_VLAN_FILTER | 
  //                              DEV_RX_OFFLOAD_VLAN_STRIP);//DEV_RX_OFFLOAD_IPV4_CKSUM;
  // // port_conf.rx_adv_conf.rss_conf.rss_key = NULL;
  // // port_conf.rx_adv_conf.rss_conf.rss_hf = ETH_RSS_IP | ETH_RSS_UDP;
  // // 根据包的那些字段进行hash，从而将不同的hash结果分发到不同的队列上
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
  tx_conf = dev_info.default_txconf;
  tx_conf.offloads = port_conf.txmode.offloads;

  rx_conf = dev_info.default_rxconf;
  rx_conf.offloads = port_conf.rxmode.offloads;

  // tx_conf = dev_info.default_txconf;
  // rx_conf.rx_thresh.pthresh = 8;
  // rx_conf.rx_thresh.hthresh = 0;
  // rx_conf.rx_thresh.wthresh = 0;
  // rx_conf.rx_free_thresh = 0;
  // rx_conf.rx_drop_en = 0;
  
  // tx_conf.tx_thresh.pthresh = 32;
  // tx_conf.tx_thresh.hthresh = 0;
  // tx_conf.tx_thresh.wthresh = 0;
  // tx_conf.tx_free_thresh = 0;
  // tx_conf.tx_rs_thresh = 0;
  
  /* Configure the Ethernet device. */
  retval = rte_eth_dev_configure(port, rx_rings, tx_rings, &port_conf);
  if (retval < 0) rte_exit(EXIT_FAILURE, "Cannot configure device: err=%d, port=%u\n", retval, (unsigned)port);
  retval = rte_eth_dev_adjust_nb_rx_tx_desc(port, &nb_rxd, &nb_txd);
  if (retval < 0) rte_exit(EXIT_FAILURE, "Cannot configure adjust desc: port=%u\n", (unsigned)port);

  /* Allocate and set up RX queue(s) per Ethernet port. */
  for (q = 0; q < rx_rings; q++) {
    retval = rte_eth_rx_queue_setup(port, q, 512/* nb_rxd */,
                                    rte_eth_dev_socket_id(port),
                                    &rx_conf/* NULL */, recv_mbuf_pool[q]);
    if (retval < 0) rte_exit(EXIT_FAILURE, "rte_eth_rx_queue_setup:err=%d, port=%u\n", retval, (unsigned)port);
  }

  /* Allocate and set up TX queue(s) per Ethernet port. */
  for (q = 0; q < tx_rings; q++) {
    retval = rte_eth_tx_queue_setup(port, q, 512/* nb_txd */,
                                    rte_eth_dev_socket_id(port),
                                    &tx_conf);
    if (retval < 0) rte_exit(EXIT_FAILURE, "rte_eth_tx_queue_setup:err=%d, port=%u\n", retval, (unsigned)port);
  }
  // retval = rte_eth_dev_mac_addr_add(static_cast<uint8_t>(port), &s_addr, 0);
  // if (retval < 0) {
  //   rte_exit(EXIT_FAILURE, "failed to register MAC address for port = %d\n",
  //            port);
  // }
  printf("starting port %" PRIu16 "...\n", port);

  // rte_eth_promiscuous_enable(port);

  retval = rte_eth_dev_start(port);
  if (retval < 0) rte_exit(EXIT_FAILURE, "rte_eth_dev_start:err=%d, port=%u\n", retval, (unsigned)port);
  
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
  for (uint16_t eid = 0; eid < 4; eid++) {
    uint16_t queue_id = eid;
    uint16_t udp_port = eid;

    printf("port_id/queue_id/udp_port = %d %d %d\n", port, queue_id, udp_port);
    //当前flow筛选规则为：无论源udp_port为多少，目的udp_port是多少，就对应到指定编号的queue中
    generate_udp_flow(port, queue_id, 0, EMPTY_MASK, udp_port, FULL_MASK);
  }
}

void print_piekv() {
  printf("piekv run: %d",m_piekv->is_running_);
}

int main(int argc, char *argv[]) {
    int ch;
    int table_power = 23, log_power = 30;
    size_t flow_mode = 0, init_table_size = 1, init_log_size = 1;//pages: default half of 240
    while ((ch = getopt(argc, argv, "hcf:l:t:")) != -1) {
        switch (ch) {
        case 'h':
            printf(
                " Usage: %s [-l <log_power>] [-t <table_power>]"
                "[-f <flow mode>] [-n <flow pages>]\n",
                argv[0]);
            exit(0);
            break;
        case 'f':
            flow_mode = atoi(optarg);
            break;
        case 'c':
            set_core_affinity = 0;
            break;
        case 'l':
            log_power = atoi(optarg);
            break;
        case 't':
            table_power = atoi(optarg);
            break;
        default:
            printf("Error: unknown option: %c\n", (char)optopt);
            break;
        }
    }
    printf("[INFO] maxbytes Size为:%d GB\n",1 << (log_power - 30));
    #ifdef RUN_MEMC3
      printf("[INFO] 运行memc3\n");
    #else
      printf("[INFO] 运行proto\n");
    #endif
    printf("[INFO] table_power 参数值为 %d\n", table_power);
    init_table_size = init_table_size << 30; //原来是30
    init_log_size = init_log_size << log_power;
    std::signal(SIGINT, sigint_handler);

    port_init();
  
    // std::string server_info;
    // generate_server_info(server_info);

    m_piekv = new Piekv(init_table_size, init_log_size);
    //创建对象
    // m_piekv->dir_client_ = new DirectoryClient();
    //注册etcd服务
    // m_piekv->dir_client_->register_server(server_info.c_str());
    //创建refresh_etcd线程
    // workers.push_back(std::thread(&Piekv::directory_proc, m_piekv));
    // printf("sizeof item:%ld\n",sizeof(item));
    settings_init(init_log_size, table_power);

    stats_init();
    assoc2_init(settings.hashpower_init);
    // conn_init();
    // preallocate = true;
    printf("alloc maxbytes:%zu\n",settings.maxbytes);

    slabs_init(settings.maxbytes, settings.factor, true);
    thread_init(settings.num_threads);
    print_memc3_settings();//这里就是打印那些带+号的各种配置信息

    // thread_init(settings.num_threads, main_base);

    RTWorker *m_rtworkers[THREAD_NUM];

    printf(" == [STAT] Workers Start (%d threads in total) == \n", THREAD_NUM); 
    #ifdef SINGLE_THREAD
      for (size_t id = 0; id < 1; id++) {
    #else
      for (size_t id = 0; id < THREAD_NUM; id++) {
    #endif
      m_rtworkers[id] = new RTWorker(m_piekv, id, send_mbuf_pool);
      workers.push_back(std::thread(&RTWorker::worker_proc,m_rtworkers[id]));
    }

    if (flow_mode == 2) workers.push_back(std::thread(&Piekv::print_trigger, m_piekv, settings.maxbytes));

    while(*reinterpret_cast<uint32_t*>(m_piekv->thread_is_running_));//之所以不直接用thread.join()是因为还有print_trigger线程需要关闭
    printf("Finish the job......\n");
    __sync_bool_compare_and_swap((volatile uint32_t *)&(m_piekv->is_running_), 1U, 0U);//用信号量强行关闭print_trigger线程
    for (auto &t : workers)
      t.join();
    
    printf("\n");
    printf("valid count:           %5zu\n\n", m_piekv->all_table_stats_->count);
    printf("Set request:           %5zu\n\n", m_piekv->all_table_stats_->set_request);
    printf("Set success:           %5zu\n\n", m_piekv->all_table_stats_->set_success);
    printf("Get request:           %5zu\n\n", m_piekv->all_table_stats_->get_request);
    printf("Get found:             %5zu\n\n", m_piekv->all_table_stats_->get_found);
    
    printf("Get Hit Rate:          %5lf%%\n\n", m_piekv->all_table_stats_->get_found * 100. / m_piekv->all_table_stats_->get_request);
    // printf("indexs load factor:     %5lf%%\n\n", 1.0 * hash_items / 7 / hashsize/* m_piekv->all_table_stats_->count * 100. / 117440512 */);//hashsize * num_bucket
    // printf("indexs Load Factor:     %5lf %%\n\n", m_piekv->hashtable_->calc_load_factor());
    printf("slabs load factor:     %5lf%%\n\n", m_piekv->all_table_stats_->count * 64 * 100. / settings.maxbytes);
    printf("cuckoo_depth:          %5zu\n\n", cuckoo_depth);
    printf("cuckoo_count:          %5zu\n\n", cuckoo_count);
  

    // dir_client->unregister_server();
    // m_piekv->dir_client_->unregister_server();

    printf("[INFO] Everything works fine.\n");fflush(stdout);

    delete m_piekv;
    return EXIT_SUCCESS; 

}