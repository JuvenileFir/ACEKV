#include <assert.h>
#include <inttypes.h>
#include <sched.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

#include <rte_byteorder.h>
#include <rte_cycles.h>
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_lcore.h>
#include <rte_mbuf.h>

#include "xxhash.h"
#include "benchmark.h"
#include "zipf.h"//只能产生theta<1的数据分布
// #include "zipfgo.h"//生成theta>1的数据分布

// #define PRELOAD

#define RX_RING_SIZE 1024
#define TX_RING_SIZE 1024

#define NUM_MBUFS 8191
#define MBUF_CACHE_SIZE 250

#define BURST_SIZE (4U)
#define PKG_GEN_COUNT 1

#define GET_RATIO 0
uint64_t total_set[NUM_QUEUE];
uint64_t total_get[NUM_QUEUE];
uint64_t total_packet[NUM_QUEUE];

// #ifdef PRELOAD
int loading_mode = 1;
int server_threads = 0;
// #endif

uint32_t get_threshold = (uint32_t)(GET_RATIO * (double)((uint32_t)-1));

// file descriptor for dump or read workload to/from file
// FILE *fp[NUM_QUEUE];

static struct rte_ether_addr S_Addr = {{0x04, 0x3f, 0x72, 0xdc, 0x26, 0x25}};//enp2s0f0 04:3f:72:dc:26:24   enp2s0f1 04:3f:72:dc:26:25
static struct rte_ether_addr D_Addr = {{0x98, 0x03, 0x9b, 0x8f, 0xb1, 0xc9}};//dest 10.176.64.41对应的网卡enp3s0f1np1的MAC地址 即98:03:9b:8f:b1:c9

// static struct rte_ether_addr S_Addr = {{0x04, 0x3f, 0x72, 0xdc, 0x26, 0x24}};//36
// static struct rte_ether_addr D_Addr = {{0x98, 0x03, 0x9b, 0x8f, 0xb0, 0x10}};//35

#define IP_SRC_ADDR ((10U << 24) | (176U << 16) | (64U << 8) | 36U)
#define IP_DST_ADDR ((10U << 24) | (176U << 16) | (64U << 8) | 35U)//对应IP

// static struct rte_ether_addr S_Addr = {{0x98, 0x03, 0x9b, 0x8f, 0xb0, 0x11}};//source  217 也就是35
// static struct rte_ether_addr D_Addr = {{0x98, 0x03, 0x9b, 0x8f, 0xb5, 0xc0}};//dest 10.141.221.222对应的网卡的MAC地址 即40
// #define IP_SRC_ADDR ((10U << 24) | (141U << 16) | (221U << 8) | 217U)
// #define IP_DST_ADDR ((10U << 24) | (141U << 16) | (221U << 8) | 222U)//对应IP

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

static const struct rte_eth_conf port_conf_default = {//按照mica里面的进行修改
    .rxmode =
        {
            .mq_mode = ETH_MQ_RX_NONE,
            .max_rx_pkt_len = RTE_ETHER_MAX_LEN,
            .offloads = (DEV_RX_OFFLOAD_VLAN_FILTER | DEV_RX_OFFLOAD_VLAN_STRIP),
        },
    .fdir_conf =
        {
            .mode = RTE_FDIR_MODE_PERFECT, 
            .pballoc = RTE_FDIR_PBALLOC_64K, 
            .status = RTE_FDIR_NO_REPORT_STATUS,
            .mask.dst_port_mask = 0xffff,
            .drop_queue = 0,
        },
    .txmode =
        {
            .mq_mode = ETH_MQ_TX_NONE,
        },
};


uint8_t *mtod(struct rte_mbuf *mbuf) { /* for using this macro in gdb */
    return (uint8_t *)rte_pktmbuf_mtod(mbuf, uint8_t *);
}

static void check_pkt_content(struct rte_mbuf *pkt);
static void tx_preload(struct rte_mbuf **tx_mbufs);
static void tx_pkt_load(char *, struct zipf_gen_state *, uint64_t *,uint32_t);
static void rx_pkt_process(struct rte_mbuf **, uint16_t);
static void print_stats(void);

static void print_pkt_header(struct rte_mbuf *pkt);
static inline void show_mac(const char *info, struct rte_ether_addr *addr);
static int pkt_filter(const struct rte_mbuf *pkt);
#ifdef _DUMP_PKT
static inline void pkt_content_dump(struct rte_mbuf *pkt);
void show_pkt(struct rte_mbuf *pkt);
#endif

void *rx_loop(context_t *context) {//接收函数 循环运行 
/*
To do:
1. receive requests from 40, and then parse the requests
2. set specific response to the buffer

*/
    printf("启动的core id = %d\n",context->core_id);
    uint32_t core_id = context->core_id;
    uint32_t queue_id = context->queue_id;
    uint16_t i, port = 0;
    struct rte_mbuf *rx_buf[BURST_SIZE];

    unsigned long mask = 1 << core_id;
    if (sched_setaffinity(0, sizeof(unsigned long), (cpu_set_t *)&mask) < 0) {
        //参数1代表是本进程来绑定,参数2是mask字段的大小,参数3是运行进程的CPU
        printf("core id = %d\n", core_id);
        assert(0);
    }

    uint64_t prev_tsc = 0, diff_tsc, cur_tsc, timer_tsc = 0;
    core_statistics[core_id].enable = 1;
    while (1) {
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
            rte_eth_rx_burst(port, queue_id, rx_buf, BURST_SIZE);
            //从以太网设备的接收队列中检索输入数据包的突发。
            //检索到的数据包存储在rte_mbuf结构中，其指针在rx_pkts数组中提供。
            //返回值：实际检索到的数据包数，即有效提供给rx_pkts数组的rte_mbuf结构的指针数。
        // printf("core id:%d\tnb_rx:%d\n", core_id,nb_rx);

        core_statistics[core_id].rx += nb_rx;

        if (nb_rx != 0) {
            // printf("收到的core id = %d\n",core_id);

            // ToDo: analyze requests respond quality
            rx_pkt_process(rx_buf, nb_rx);
            for (i = 0; i < nb_rx; i++) {
                rte_pktmbuf_free(rx_buf[i]);
                //释放一个mbuf，释放过程即把mbuf归还到rte_mempool中.
            }
        }
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
    const static uint32_t SEND_ROUND = 20;
    uint32_t core_id = context->core_id;
    uint32_t queue_id = context->queue_id;
    uint16_t i, port = 0;
    uint16_t pktlen = ETHERNET_MAX_FRAME_LEN;//TODO:类似于server，包填充完成后再填写data_len等属性(client之前一直是预先算好直接填)
    struct rte_mbuf *tx_bufs_pt[PKG_GEN_COUNT * SEND_ROUND];//用来存储packet?????

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
    uint64_t preload_cnt = PRELOAD_CNT;//目前为64M
    // uint64_t preload_cnt = 192 * 1048576;//已参考mica设置为192M，值过大则会需要很长的生成时间

    uint64_t get_key = 1;  // set_key = 1;

    struct zipf_gen_state zipf_state;
    mehcached_zipf_init(&zipf_state, preload_cnt - 2, (double)ZIPF_THETA,
                        (uint64_t)core_id);
    /*
    To do: add the code for theta > 1
    */
   /*struct Zipf z;
   ///////NewZipf(&z,randSeed, 1.1, 1, max);
   NewZipf(&z,21, double(ZIPF_THETA), 1, preload_cnt - 2);//seed,theta,min,max  xhj
   */
    struct rte_ether_hdr *ethh;
    struct rte_ipv4_hdr *ip_hdr;
    struct rte_udp_hdr *udph;
    for (int i = 0; i < PKG_GEN_COUNT * SEND_ROUND; i++) {//总共申请20个packet的内存空间，全部按照preload来初始化
        struct rte_mbuf *pkt = (struct rte_mbuf *)rte_pktmbuf_alloc(
            (struct rte_mempool *)send_mbuf_pool);
            //rte_pktmbuf_alloc向rte_mempool申请一个mbuf
        if (pkt == NULL)
            rte_exit(EXIT_FAILURE,
                     "Cannot alloc storage memory in  port %" PRIu16 "\n",
                     port);
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
        ip_hdr->total_length = rte_cpu_to_be_16(pktlen - sizeof(struct rte_ether_hdr));//将16位值从CPU顺序转换为大端and按照line 300更改
        ip_hdr->src_addr = rte_cpu_to_be_32(IP_SRC_ADDR);
        ip_hdr->dst_addr = rte_cpu_to_be_32(IP_DST_ADDR);

        ip_hdr->hdr_checksum = rte_ipv4_cksum(ip_hdr);

        udph = (struct rte_udp_hdr *)((unsigned char *)ip_hdr +
                                      sizeof(struct rte_ipv4_hdr));
        // udph->src_port = 123;
        // udph->dst_port = 123;
        udph->dgram_len =
            rte_cpu_to_be_16((uint16_t)(pktlen - sizeof(struct rte_ether_hdr) -
                                        sizeof(struct rte_ipv4_hdr)));
        // udph->dgram_cksum = 0;

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
    static uint64_t set_key[NUM_QUEUE];
     set_key[queue_id] = 1 + queue_id * PER_THREAD_CNT + PRELOAD_CNT; 
    //  set_key[queue_id] = (uint64_t)1000000000 * (queue_id + 1); 

    // printf("queue_id:%d\tset_key_before:%lu\n",queue_id,set_key[queue_id]);

    //  set_key = 1 + queue_id * PER_THREAD_CNT; 

    char *ptr = NULL;
    core_statistics[core_id].enable = 1;
    int T =10;
    while (T) {
        
        for (i = 0; i < SEND_ROUND; i++) {
            // Random UDP port for RSS distributing pkts among RXs
            //用于RX之间RSS分发pkts的随机UDP端口
            /*
            RSS是一项技术，
            可使用预定义的哈希函数在多个RX队列之间分配接收到的数据包。
            它使多核CPU能够处理来自不同内核上不同队列的数据包。
            */
            udph = (struct rte_udp_hdr *)((char *)rte_pktmbuf_mtod(
                                              tx_bufs_pt[i], char *) +
                                          sizeof(struct rte_ether_hdr) +
                                          sizeof(struct rte_ipv4_hdr));
            // udph->src_port = queue_id;
            udph->src_port = queue_id;
            udph->dst_port = 1;
            // udph->src_port = set_key % 65535;
            // udph->dst_port = set_key % 65535;
            // ToDo: Is UPD cksum is needed?
            
            ptr = (char *)((char *)rte_pktmbuf_mtod(tx_bufs_pt[i],char *)
             + kHeaderLen);
                           //rte_pktmbuf_mtod函数获得指向mbuf中数据开头的宏。

            tx_pkt_load(ptr, &zipf_state, set_key, queue_id);//组装packet
#ifdef _DUMP_PKT
            rte_pktmbuf_dump(fp[sched_getcpu()], tx_bufs_pt[i],
                             tx_bufs_pt[i]->pkt_len);
            //将mbuf结构转储到文件中。
            // show_pkt(tx_bufs_pt[i]);
            // pkt_content_dump(tx_bufs_pt[i]);
#endif
            int nb_tx = rte_eth_tx_burst(port, queue_id, tx_bufs_pt + i, 1);
            //rte_eth_tx_burst——物理口发包函数
            total_packet[queue_id]++;
            core_statistics[core_id].tx += nb_tx;
        }

            // break;//xhj 看是否这样server就会少输出一部分信息
        // rte_delay_us_sleep(400);
    }
}

void *GetThread(context_t *context) {
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

int main(int argc, char *argv[]) {

#ifdef _DUMP_PKT
    char filename[50];//已有filename
    for (int i = 0; i < NUM_MAX_CORE; i++) {
        sprintf(filename, "/home/hewen/tmp/mac_%d.txt", i);
        if ((fp[i] = fopen(filename, "wt+")) == NULL) {
            printf("Fail to open file(%s)\n", filename);
            exit(-1);
        }
    }
#endif

#ifdef _DUMP_WORKLOAD_
    char workload_filename[50];//已有workload
    for (int i = 0; i < NUM_QUEUE; i++) {
        sprintf(workload_filename, "/home/hewen/tmp/static_workload_%d.txt", i);
        if ((fp[i] = fopen(workload_filename, "wt+")) == NULL) {
            printf("Fail to open file(%s)\n", workload_filename);
            exit(-1);
        }
    }
#endif

#ifdef _STATIC_WORKLOAD_//static workload
    char workload_filename[50];
    for (int i = 0; i < NUM_QUEUE; i++) {
        sprintf(workload_filename, "/home/hewen/tmp/static_workload_%d.txt", i);
        if ((fp[i] = fopen(workload_filename, "rt")) == NULL) {
            printf("Fail to open file(%s)\n", workload_filename);
            exit(-1);
        }
    }
#endif
for (int i = 0; i< NUM_QUEUE; i++){
    total_set[i] = 0;
    total_get[i] = 0;
    total_packet[i] = 0;
}
    unsigned nb_ports;
    uint16_t portid = 0;
    /* Initialize the Environment Abstraction Layer (EAL). */
    int t_argc = 12;
    char *t_argv[] = {
        "./build/benchmark", "-c", "f", "-n", "1", "--huge-unlink", "-w",
        "pci@0000:02:00.1","--file-prefix", "bwb","--socket-mem", "2048"};//网卡端口
        //02:00.0 Ethernet controller: Mellanox Technologies MT27800 Family [ConnectX-5]
    int ret = rte_eal_init(t_argc, t_argv);
    if (ret < 0) rte_exit(EXIT_FAILURE, "Error with EAL initialization\n");
    printf("rte = %d\n",ret);
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
    //新建了一个发送buf pool
        // printf("Creates a new mempool in memory to hold the mbufs.Done\n");

    /* Initialize all ports. */
    const uint16_t rx_rings = NUM_QUEUE, tx_rings = NUM_QUEUE;//收发队列
    uint16_t nb_rxd = RX_RING_SIZE;
    uint16_t nb_txd = TX_RING_SIZE;
    uint16_t q;
    int retval, port = 0;
    struct rte_eth_conf port_conf = port_conf_default;
    struct rte_eth_dev_info dev_info;
    struct rte_eth_txconf tx_conf;
    struct rte_eth_rxconf rx_conf;
    retval = rte_eth_dev_info_get(port, &dev_info);
    if (retval < 0) {
        rte_exit(EXIT_FAILURE,
                 "Error during getting device (port %u) info: %s\n", port,
                 strerror(-retval));
    }
    tx_conf = dev_info.default_txconf;
    tx_conf.offloads = port_conf.txmode.offloads;
    // rx_conf.offloads = DEV_RX_OFFLOAD_CHECKSUM;
        // printf("Initialize all ports.Done\n");

    /* Configure the Ethernet device. 配置以太网设备*/
    retval = rte_eth_dev_configure(port, rx_rings, tx_rings, &port_conf);
    if (retval < 0)
        rte_exit(EXIT_FAILURE, "Cannot configure device: err=%d, port=%u\n",
                 retval, (unsigned)port);
    retval = rte_eth_dev_adjust_nb_rx_tx_desc(port, &nb_rxd, &nb_txd);
        // printf("Configure the Ethernet device.Done\n");

    /*检查Rx和Tx描述符的数量是否满足以太网设备信息中的描述符限制，否则将其调整为边界*/
    if (retval < 0)
        rte_exit(EXIT_FAILURE, "Cannot configure adjust desc: port=%u\n",
                 (unsigned)port);

    /* Allocate and set up RX queue(s) per Ethernet port. */
    for (q = 0; q < rx_rings; q++) {
        retval =
            rte_eth_rx_queue_setup(port, q, nb_rxd, rte_eth_dev_socket_id(port),
                                   NULL, recv_mbuf_pool[q]);
        /*分配并设置以太网设备的接收队列。*/
        if (retval < 0)
            rte_exit(EXIT_FAILURE, "rte_eth_rx_queue_setup:err=%d, port=%u\n",
                     retval, (unsigned)port);
    }

    /* Allocate and set up TX queue(s) per Ethernet port. */
    for (q = 0; q < tx_rings; q++) {
        retval = rte_eth_tx_queue_setup(port, q, nb_txd,
                                        rte_eth_dev_socket_id(port), &tx_conf);
        /*分配并设置以太网设备的传输队列。*/
        if (retval < 0)
            rte_exit(EXIT_FAILURE, "rte_eth_tx_queue_setup:err=%d, port=%u\n",
                     retval, (unsigned)port);
    }

    retval = rte_eth_dev_start(port);
    if (retval < 0)
        rte_exit(EXIT_FAILURE, "rte_eth_dev_start:err=%d, port=%u\n", retval,
                 (unsigned)port);
    rte_eth_promiscuous_enable(port);
    //bwb: ↑ ↑ ↑ 相当于port_init()部分
    pthread_t tid;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);//线程分离
        // printf("Allocate and set up TX queue(s) per Ethernet port.Done\n");

    context_t *context;//结构体中包含coreId和queueId
    // for (int i = 0; i < 1; i++) {
/*     for (int i = 0; i < rx_rings; i++) {
        //创建对应的线程，来执行tx_loop和rx_loop函数
        context = (context_t *)malloc(sizeof(context_t));
        context->core_id = (i << 1) + 1;//奇数 2i+1
        context->queue_id = i;
        if (pthread_create(&tid, &attr, (void *)rx_loop, (void *)context) != 0)
            perror("pthread_create error!!\n");
    }

    context = (context_t *)malloc(sizeof(context_t));
    context->core_id = 20;
    context->queue_id = 10;
    if (pthread_create(&tid, &attr, (void *)GetThread, (void *)context) != 0)
    perror("pthread_create error!!\n"); */

    for (int i = 0; i < rx_rings; i++) {
        //创建对应的线程，来执行tx_loop和rx_loop函数
        if (i >= 0) {
            context = (context_t *)malloc(sizeof(context_t));
            context->core_id = i << 1;//偶数 2i
            context->queue_id = i;
            if (pthread_create(&tid, &attr, (void *)tx_loop, (void *)context) !=
                0)
                perror("pthread_create error!!\n");
        }
    }
    int T = 1;
    while (T--) {
        sleep(1);
    }
    for (int i = 0; i< NUM_QUEUE; i++) printf("[INFO]total pkt[ %d ]: %ld\n", i, total_packet[i]);
    for (int i = 0; i< NUM_QUEUE; i++) printf("[INFO]total set[ %d ]: %ld\n", i, total_set[i]);
    for (int i = 0; i< NUM_QUEUE; i++) printf("[INFO]total get[ %d ]: %ld\n", i, total_get[i]);



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
            while (payload_len + SET_LEN <= ETHERNET_MAX_FRAME_LEN) {
                count++;
                *(uint16_t *)ptr = MEGA_JOB_SET;
                ptr += sizeof(uint16_t); /* 2 bytes job type */
                *(uint16_t *)ptr = KEY_LEN;
                ptr += sizeof(uint16_t); /* 2 bytes key length */
                *(uint16_t *)ptr = KEY_HASH_LEN;//2 byte
                ptr += sizeof(uint16_t);
                *(uint16_t *)ptr = VALUE_LEN;//2 byte
                ptr += sizeof(uint16_t);
                
                key_hash =XXH64(&set_key,KEY_LEN,1);

                *(uint64_t *)(ptr) = set_key;
                ptr += KEY_LEN;
                *(uint64_t *)(ptr) = key_hash;
                ptr += KEY_HASH_LEN;
                *(uint64_t *)(ptr) = set_key + 1;
                ptr += VALUE_LEN;

                payload_len += SET_LEN;

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

static void tx_pkt_load(char *ptr, struct zipf_gen_state *zipf_state,
                        uint64_t *start_set_key,uint32_t t_id) {

    //将请求组装到一个packet中
    //tx_pkt_load(ptr, &zipf_state, &set_key);
    //参数1指向mempool的开始，参数三开始定义为1
    uint16_t set_count = 0, get_count = 0;
    uint16_t payload_len = kHeaderLen + MEGA_END_MARK_LEN;
    uint64_t key_hash, get_key64, set_val64, k = (uint64_t)(t_id + 1000);
    uint64_t set_key64 = *(uint64_t *)(start_set_key + t_id);

    char* get_key = (char *)malloc(KEY_LEN);
    char* set_key = (char *)malloc(KEY_LEN);
    memset(get_key, 0, KEY_LEN);
    memset(set_key, 0, KEY_LEN);
    
    //在一个包内，get请求是先type，然后keylen，然后key
    //set的话，先是type，然后klen，valueLen，然后key和value
    while(payload_len + GET_LEN <= ETHERNET_MAX_FRAME_LEN){
			uint32_t op_r = mehcached_rand(&k);
			bool is_get = op_r <= get_threshold;
			
			if(likely(is_get)){
				total_get[t_id]++;

				get_count++;
#ifndef _STATIC_WORKLOAD_
			// get_key64 = (uint64_t)mehcached_zipf_next(zipf_state) + 1;
			get_key64 = set_key64;

			memcpy(get_key, &get_key64, 8);
			key_hash =XXH64(get_key,KEY_LEN,1);
			// printf("GET\nkey:%zu,hash:%zu\n",get_key,key_hash);

			// get_key = set_key;//??? xhj  why not zipf distribution？
			/*There is no preload period, therefore the set_key must equals to the get_key.
			So, the preload period should put all the set keys to the server, 
			and then set new key and get key with zipf distribution.

			To do:
			1.modify the client and the server,
				then test whether the server can receive the packet,
				and test whether the client can receive the right response
			2.modify the function of set and get as the golang code 
			3.retest the whole data and train the model to get a new ML model
			4.write and plot the results
			Finish !!
			*/
			// assert(get_key >= 1 && get_key <= PRELOAD_CNT);
#else
			fscanf(fp[sched_getcpu() >> 1], "%lu ", &get_key);
#endif

#ifdef _DUMP_WORKLOAD_
			fprintf(fp[sched_getcpu() >> 1], "%lu ", get_key);
#endif

			*(uint16_t *)ptr = MEGA_JOB_GET;//operation type
			ptr += sizeof(uint16_t);
			*(uint16_t *)ptr = KEY_LEN;
			ptr += sizeof(uint16_t);
			*(uint16_t *)ptr = KEY_HASH_LEN;//2 byte
			ptr += sizeof(uint16_t);

#if defined(KV_LEN_8)
			*(uint32_t *)(ptr) = (uint32_t)get_key64;
	
#elif defined(KV_LEN_16) || defined(KV_LEN_24) || defined(KV_LEN_32) || defined(KV_LEN_40)
			*(uint64_t *)(ptr) = get_key64;

#elif defined(KV_LEN_48) || defined(KV_LEN_56) || defined(KV_LEN_64) || defined(KV_LEN_1024)
			memcpy(ptr, get_key, KEY_LEN);

#endif
			ptr += KEY_LEN;
			*(uint64_t *)(ptr) = key_hash;
			ptr += KEY_HASH_LEN;//+8字节，即64位

			payload_len += GET_LEN;

			}
			else{
				if(unlikely((ETHERNET_MAX_FRAME_LEN - payload_len) < SET_LEN)) break;
				total_set[t_id]++;
				set_count++;
				set_key64++;
				set_val64 = (set_key64 + 1) % TOTAL_CNT;
				if (set_key64 >= (uint64_t)(PER_THREAD_CNT * (t_id + 1) + PRELOAD_CNT)) {
					set_key64 = (uint64_t)(PER_THREAD_CNT * t_id + 1 + PRELOAD_CNT); //如果超过该线程分配的数据范围，则返回数据范围首位置
                    printf("new round\n");
                }

				memcpy(set_key, &set_key64, 8);
				key_hash = XXH64(set_key, KEY_LEN, 1);

				*(uint16_t *)ptr = MEGA_JOB_SET; //#define MEGA_JOB_SET 0x3
				ptr += sizeof(uint16_t);         //向后移动2 byte 即操作类型2 byte
				*(uint16_t *)ptr = KEY_LEN;      // 2 byte
				ptr += sizeof(uint16_t);
				*(uint16_t *)ptr = KEY_HASH_LEN; // 2 byte
				ptr += sizeof(uint16_t);
				*(uint16_t *)ptr = VALUE_LEN; // 2 byte
				ptr += sizeof(uint16_t);

				*(uint64_t *)(ptr + KEY_LEN) = key_hash;

#if defined(KV_LEN_8)
				*(uint32_t *)(ptr) = (uint32_t)set_key64;
				ptr += KEY_LEN + KEY_HASH_LEN;
				*(uint32_t *)(ptr) = (uint32_t)set_val64;

#elif defined(KV_LEN_16)
				*(uint64_t *)(ptr) = set_key64;//16+16+16+16=64,前面正好用了64位，set的key是从2开始逐渐递增的
				ptr += KEY_LEN + KEY_HASH_LEN;//向后移动KEY_LEN+KEY_HASH_LEN
				*(uint64_t *)(ptr) = set_val64;//value是key+1
#else
		#if defined(KV_LEN_24) || defined(KV_LEN_32) || defined(KV_LEN_40)
				*(uint64_t *)(ptr) = set_key64;
		#elif defined(KV_LEN_48) || defined(KV_LEN_56) || defined(KV_LEN_64) || defined(KV_LEN_1024)
				memcpy(ptr, set_key, KEY_LEN);
		#endif
				ptr += KEY_LEN + KEY_HASH_LEN;
				memcpy(ptr, &set_val64, 8);
				memset(ptr + 8, 0, VALUE_LEN - 8);
#endif
				ptr += VALUE_LEN;//向后移动value_len
				payload_len += SET_LEN;
			}
    }
    *(uint64_t *)(start_set_key + t_id) = set_key64;//更新start_set_key字段
    /* pkt ending flag */
    *(uint16_t *)ptr = 0xFFFF;
    // printf("set:%u\tget:%u\tlen:%u\n", set_count, get_count, payload_len);
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
#ifdef _DUMP_PKT
    fprintf(fp[sched_getcpu()],
#else
    printf(
#endif
            "UDP port : src(%d) dst(%d)\n", udph->src_port, udph->dst_port);
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
