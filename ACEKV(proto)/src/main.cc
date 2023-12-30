#include "communication.h"
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

/* void generate_server_info(std::string server_info_) {
  std::ostringstream oss;
  oss << '{';

  oss << "\"concurrent_read\":";
  oss << "false";

  oss << ", \"concurrent_write\":";
  oss << "false";

  oss << ", \"partitions\":[";
  oss << "[0,0],[1,1],[2,2],[3,3]";
  // oss << ",[4,0],[5,1],[6,2],[7,3]";
  oss << ']';

  oss << ", \"endpoints\":[";
  oss << "[0,0,\"98:03:9b:8f:b1:c8\",\"10.176.64.41\",0],";
  oss << "[1,1,\"98:03:9b:8f:b1:c8\",\"10.176.64.41\",1],";
  oss << "[2,2,\"98:03:9b:8f:b1:c8\",\"10.176.64.41\",2],";
  oss << "[3,3,\"98:03:9b:8f:b1:c8\",\"10.176.64.41\",3]";
  oss << ']';

  oss << '}';

  server_info_ = oss.str();

  if (true) printf("server_info: %s\n", server_info_.c_str());
}
 */
void sigint_handler(int sig) {
  printf("run finish job......\n");
  __sync_bool_compare_and_swap((volatile uint32_t *)&(m_piekv->is_running_), 1U, 0U);
  for (auto &t : workers) t.join();
  printf("\n");
  printf("Total Index Entries:    %10zu\n\n", m_piekv->hashtable_->all_table_stats_->count);
  printf("indexs Load Factor:     %5lf %%\n\n", m_piekv->hashtable_->all_table_stats_->count * 100. / kSlotNum);
  printf("cirlog Load Factor:     %5lf %%\n\n", m_piekv->hashtable_->all_table_stats_->count * 16 * 100.0 / m_piekv->mempool_->max_log_size_);//init_log_size是单线程的2G，一共4个线程。所以把前面乘的40除以4
  printf("Get Hit Rate:           %5lf%%\n\n", m_piekv->hashtable_->all_table_stats_->get_found * 100. / m_piekv->hashtable_->all_table_stats_->get_request);
  printf("Max Throughput:         %5lf\n\n", m_piekv->max_tput_);
  m_piekv->hashtable_->calc_load_factor();

  for (int i = 0; i < THREAD_NUM; i++) {
    m_piekv->log_->log_segments_[i]->print_table_stats();
    printf("\n\n");
  }
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

int main(int argc, char *argv[]){
    int ch;
    int table_power = 30, log_power = 31;//默认31
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
    
    init_table_size = init_table_size << table_power;
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


    RTWorker *m_rtworkers[4];

    printf(" == [STAT] Workers Start (%d threads in total) == \n", THREAD_NUM); 
    for (size_t id = 0; id < 4/* THREAD_NUM */; id++) {
      m_rtworkers[id] = new RTWorker(m_piekv, id, send_mbuf_pool);
      workers.push_back(std::thread(&RTWorker::worker_proc,m_rtworkers[id]));
    }

    if (flow_mode == 2) workers.push_back(std::thread(&Piekv::print_trigger, m_piekv));

    while(*reinterpret_cast<uint32_t*>(m_piekv->thread_is_running_));//之所以不直接用thread.join()是因为还有print_trigger线程需要关闭
    printf("Finish the job......\n");
    __sync_bool_compare_and_swap((volatile uint32_t *)&(m_piekv->is_running_), 1U, 0U);//用信号量强行关闭print_trigger线程
    for (auto &t : workers)
      t.join();
    
    printf("\n");
    printf("Total Index Entries:    %10zu\n\n", m_piekv->hashtable_->all_table_stats_->count);
    printf("indexs Load Factor:     %5lf %%\n\n", m_piekv->hashtable_->all_table_stats_->count * 100. / kSlotNum);
    printf("cirlog Load Factor:     %5lf %%\n\n", m_piekv->hashtable_->all_table_stats_->count * 16 * 100.0 / init_log_size);//init_log_size是单线程的2G，一共4个线程。所以把前面乘的40除以4
    printf("Get Hit Rate:           %5lf%%\n\n", m_piekv->hashtable_->all_table_stats_->get_found * 100. / m_piekv->hashtable_->all_table_stats_->get_request);
    printf("Max Throughput:         %5lf\n\n", m_piekv->max_tput_);
    m_piekv->hashtable_->calc_load_factor();

    for (int i = 0; i < THREAD_NUM; i++) {
      m_piekv->log_->log_segments_[i]->print_table_stats();
      printf("\n\n");
    }
    // dir_client->unregister_server();
    // m_piekv->dir_client_->unregister_server();

    printf("[INFO] Everything works fine.\n");fflush(stdout);

    delete m_piekv;
    return EXIT_SUCCESS; 

}