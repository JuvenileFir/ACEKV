#include <assert.h>
#include <inttypes.h>
#include <sched.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <rte_byteorder.h>
#include <rte_cycles.h>
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_lcore.h>
#include <rte_mbuf.h>

#include "benchmark.h"
#include "zipf.h"//只能产生theta<1的数据分布
#include "zipfgo.h"//生成theta>1的数据分布

// #define PRELOAD

#define RX_RING_SIZE 1024
#define TX_RING_SIZE 1024

#define NUM_MBUFS 8191
#define MBUF_CACHE_SIZE 250

//#define BURST_SIZE (4U)
#define BURST_SIZE (4U)
#define PKG_GEN_COUNT 1

#define BYTEALIGN 2
#define ROUNDUP(x,n) (x+(n-1))&(~(n-1))//利用roundup进行内存对齐
#define PACKETLEN 1508

// #ifdef PRELOAD
int loading_mode = 1;
// #endif

// file descriptor for dump or read workload to/from file
// FILE *fp[NUM_QUEUE];
/*
static struct rte_ether_addr S_Addr = {{0x98, 0x03, 0x9b, 0x8f, 0xb0, 0x10}};//source  216
static struct rte_ether_addr D_Addr = {{0x98, 0x03, 0x9b, 0x8f, 0xb5, 0xc1}};//dest 223
#define IP_SRC_ADDR ((10U << 24) | (141U << 16) | (221U << 8) | 216U)
#define IP_DST_ADDR ((10U << 24) | (141U << 16) | (221U << 8) | 223U)
*/
//test 222 and 217
static struct rte_ether_addr S_Addr = {{0x98, 0x03, 0x9b, 0x8f, 0xb0, 0x11}};
static struct rte_ether_addr D_Addr = {{0x98, 0x03, 0x9b, 0x8f, 0xb5, 0xc0}};
#define IP_SRC_ADDR ((10U << 24) | (141U << 16) | (221U << 8) | 217U)
#define IP_DST_ADDR ((10U << 24) | (141U << 16) | (221U << 8) | 222U)

// 14(ethernet header) + 20(IP header) + 8(UDP header)
#define EIU_HEADER_LEN 42 //sizeof(struct ether_hdr) + sizeof(struct ipv4_hdr) + sizeof(struct udp_hdr)
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
//cache line对齐，避免多个核访问一个数据结构
//保存了每个core整体情况
struct benchmark_core_statistics core_statistics[NUM_MAX_CORE];

//edit by xhj
struct mbuf_table{
    uint16_t len;
    uint16_t pos;
    struct rte_mbuf *m_table[BURST_SIZE];
};
//edit by xhj

#ifdef _DUMP_PKT
FILE *fp[NUM_MAX_CORE];
#endif

#define TIMER_MILLISECOND 2000000ULL /* around 1ms at 2 Ghz */
#define MAX_TIMER_PERIOD 86400       /* 1 day max */
static int64_t timer_period = 5 * TIMER_MILLISECOND * 1000; /* default period is 5 seconds */
// int timer_period = 5 * TIMER_MILLISECOND * 1000; /* default period is 5 seconds */
typedef struct context_s {
    unsigned int core_id;
    unsigned int queue_id;
} context_t;
/*
Structure for request
*/
typedef struct{
    int operation;//0 set 1get
    int state;//0 fail 1succ
    int key_size;
    int value_size;
    char* key;
    char* value;
} Request;
/*
Structure for request
*/
static const struct rte_eth_conf port_conf_default = {
    .rxmode =
        {
            .mq_mode = ETH_MQ_RX_RSS,//开启rss来分流
            .max_rx_pkt_len = RTE_ETHER_MAX_LEN,
            .offloads = DEV_RX_OFFLOAD_IPV4_CKSUM,
        },
    .rx_adv_conf =
        {
            .rss_conf =
                {
                    .rss_key = NULL,
                    .rss_hf = ETH_RSS_IP | ETH_RSS_UDP,//通过l3层tuple和通过l4层 UDP tuple计算rss hash
                },
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
static void tx_pkt_padding(uint16_t port,context_t *context,Request *,struct mbuf_table *);
static void rx_pkt_process_m(uint16_t ,context_t *,struct rte_mbuf **, uint16_t,struct mbuf_table *);

static void print_stats(void);
static void print_pkt_header(struct rte_mbuf *pkt);
static inline void show_mac(const char *info, struct rte_ether_addr *addr);
static int pkt_filter(const struct rte_mbuf *pkt);
#ifdef _DUMP_PKT
static inline void pkt_content_dump(struct rte_mbuf *pkt);
void show_pkt(struct rte_mbuf *pkt);
#endif

void *rx_loop_m(context_t *context) {//接收函数 循环运行 
/*
To do:
1.接收数据包，然后进行解析，并将最终的结果填充到新的packet中然后发送出去
注：同一个packet的请求来自同一个client，所以每一个请求处理完之后，首先判断要填充的字段有没有超过现在的packetSize，
如果超过了，就把当前这个packet发送出去，然后继续填充新的请求。
发送时，要把dest和src进行调换，然后就可以了。
发送时模仿udpecho的逻辑，每个lcore都有一个数组用来存储packet。

*/
    uint32_t core_id = context->core_id;//id是1,2,3,4 
    uint32_t queue_id = context->queue_id;
    uint16_t i, port = 0;
    struct rte_mbuf *rx_buf[BURST_SIZE];//能收取4个packet，udpecho边是32
    //************xhj
    /*
    TO DO:
    1.init the tx_buf:for each packet,try to set the flags
    2.
    */

    struct mbuf_table tx_buf;//使用一个结构体来代表上面的四个元素
    /*
    To do:
    1.因为每个包中的请求都来自同一个client，所以应该将同一个packet的response组合在一起并发送出去。
    有两种方式：
    1.1为每个tenant设定一个队列，有专门的线程(如：主线程轮询)负责从队列中取请求并组装成packet发送出去。
    1.2每个worker thread自己收到packet并解析之后，直接将请求组成packet发送出去。
    两种方式都可以，但考虑到整个实验要跟原有的memcached对比，同时主线程还可能要进行多租户的请求监控，
    因此先让每个worker thread来做这件事，后面再考虑优化的问题。
    2.考虑到latency，每个租户的packet所形成的response即使没把packet填满，也要发送出去。
    (这样可能会造成很大的资源浪费，因为response可能只有一个packet也要单独发送。)
    3.想到这儿，感觉还是用对应的租户队列来保存请求，然后组装成packet发送回去比较靠谱。
    那对应的租户队列中每个元素的结构大致是：
    GET_SUCC key_len val_len key value
    GET_FAIL 
    SET_SUCC 
    SET_FAIL
    !!!
    还需要两个queue，一个抽样只保存key(skew)(without set operation),一个保存操作类型，keySize和valueSize
    stats_thread:用来对上面两个队列进行状态更新 while(1)
    predict_thread:用来根据租户的状态信息计算MOPS。使用libvent实现定时启动
    如果每个租户有一个队列用来存储要返回的值，
    */

    struct rte_ether_hdr *ethh;
    struct rte_ipv4_hdr *ip_hdr;
    struct rte_udp_hdr *udph;

    for (int i = 0; i < BURST_SIZE; i++) {//总共申请多少个packet的内存空间
        struct rte_mbuf *pkt = (struct rte_mbuf *)rte_pktmbuf_alloc(
            (struct rte_mempool *)send_mbuf_pool);
            //rte_pktmbuf_alloc向rte_mempool申请一个mbuf
        if (pkt == NULL)
            rte_exit(EXIT_FAILURE,
                     "Cannot alloc storage memory in  port %" PRIu16 "\n",
                     port);
        pkt->data_len = ETHERNET_MAX_FRAME_LEN;
        pkt->nb_segs = 1;  // nb_segs
        pkt->pkt_len = pkt->data_len;
        pkt->ol_flags = PKT_TX_IPV4;  // ol_flags
        pkt->vlan_tci = 0;            // vlan_tci//虚拟网络的标记位
        pkt->vlan_tci_outer = 0;      // vlan_tci_outer
        pkt->l2_len = sizeof(struct rte_ether_hdr);
        pkt->l3_len = sizeof(struct rte_ipv4_hdr);

        ethh = (struct rte_ether_hdr *)rte_pktmbuf_mtod(pkt, unsigned char *);
        //指向mbuf中数据开头的宏。
        //传递pkt给子函数，而不是直接传递数据部分
        ethh->s_addr = D_Addr;
        ethh->d_addr = S_Addr;//xhj 这里的地址设置要根据接收到的packet来进行设置,所以要放到解析函数里去设置
        //To do:暂时没加入多租户的地址设置环节
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
        ip_hdr->total_length = rte_cpu_to_be_16(ETHERNET_MAX_FRAME_LEN);//将16位值从CPU顺序转换为大端
        ip_hdr->src_addr = rte_cpu_to_be_32(IP_DST_ADDR);
        ip_hdr->dst_addr = rte_cpu_to_be_32(IP_SRC_ADDR);//xhj 这里的地址设置要根据接收到的packet来进行设置,所以要放到解析函数里去设置
        //To do:暂时没加入多租户的地址设置环节
        ip_hdr->hdr_checksum = rte_ipv4_cksum(ip_hdr);

        udph = (struct rte_udp_hdr *)((unsigned char *)ip_hdr +
                                      sizeof(struct rte_ipv4_hdr));
        // udph->src_port = 123;
        // udph->dst_port = 123;
        udph->dgram_len =
            rte_cpu_to_be_16((uint16_t)(ETHERNET_MAX_FRAME_LEN - sizeof(struct rte_ether_hdr) -
                                        sizeof(struct rte_ipv4_hdr)));
        // udph->dgram_cksum = 0;

        //tx_bufs[i] = pkt;//存储这些包

        tx_buf.m_table[i] = pkt;
    }//上面一整个循环就是先将设定好格式但没有设定好内容的要发送的packet填充到数组中
    //init the packets for tx

    unsigned long mask = 1 << core_id;//mask是要绑定的核
    if (sched_setaffinity(0, sizeof(unsigned long), (cpu_set_t *)&mask) < 0) {
        //字段1代表是本进程来绑定,2是mask字段的大小,3是运行进程的CPU
        printf("core id = %d\n", core_id);
        assert(0);
    }

    uint64_t prev_tsc = 0, diff_tsc, cur_tsc, timer_tsc = 0;
    core_statistics[core_id].enable = 1;
    while (1) {
        //定时输出对应统计信息
        if (queue_id == 0) {//只让一个线程来输出统计信息
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
        core_statistics[core_id].rx += nb_rx;
        if (nb_rx != 0) {
           // printf("xhj__________received %u packets!!\n",nb_rx);
            // ToDo: 解析包的结构，返回对应的response，然后组装并进行发送。
            /*
            int curr_pkt = -1;//记录tx_buf正在填充的packet
            int flag = -1;//当前packet是否已经填充满，0没填充完，1填充好了
            */
            rx_pkt_process_m(port, context, rx_buf, nb_rx, &tx_buf);//将整个packet数组进行处理
            for (i = 0; i < nb_rx; i++) {
                rte_pktmbuf_free(rx_buf[i]);
                //释放一个mbuf，释放过程即把mbuf归还到rte_mempool中.
            }
            //这里应该要把剩下的packet也发送出去
            char *ptr = NULL;
            if((tx_buf.len = 0)||(tx_buf.pos != 0)){
                //padding and send out
                ptr = (char *)((char *)rte_pktmbuf_mtod(
                               tx_buf.m_table[tx_buf.len],
                               char *) +
                           EIU_HEADER_LEN + tx_buf.pos);
                int pkt_space_left = PACKETLEN - EIU_HEADER_LEN - 2 - tx_buf.pos;
                for(int i=0;i<pkt_space_left;i++){
                    *(uint8_t *)ptr = 0xFF;//xhj
                    ptr += sizeof(uint8_t);
                }
                *(uint16_t *)ptr = 0xFFFF;
                int nb_tx = rte_eth_tx_burst(port, queue_id,
                                tx_buf.m_table,
                                tx_buf.len + 1);
                core_statistics[core_id].tx += nb_tx;
                tx_buf.len = 0;
                tx_buf.pos = 0;
            }
        }
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
    char workload_filename[50];//已有workload文件
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

    unsigned nb_ports;
    uint16_t portid = 0;
    /* Initialize the Environment Abstraction Layer (EAL). */
    int t_argc = 12;
 char *t_argv[] = {
        "./build/benchmark", "-c", "0xf0", "-n", "2", "--huge-unlink", "-w",
        "pci@0000:03:00.1","--file-prefix", "xhj", "--socket-mem", "400"};
    int ret = rte_eal_init(t_argc, t_argv);
    if (ret < 0) rte_exit(EXIT_FAILURE, "Error with EAL initialization\n");

    nb_ports = rte_eth_dev_count_avail();
    printf("WEN: There are %d device(s) available\n", nb_ports);

    /* Creates a new mempool in memory to hold the mbufs. */
    char str[10];
    for (int i = 0; i < NUM_QUEUE; i++) {//4个接收内存池
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
        /*
        To do:
        1.是否将生成的response packet放到这个pool中?
        */
    //新建了一个发送buf pool
    /* Initialize all ports. */
    const uint16_t rx_rings = NUM_QUEUE, tx_rings = NUM_QUEUE;//收发队列
    uint16_t nb_rxd = RX_RING_SIZE;
    uint16_t nb_txd = TX_RING_SIZE;
    u#i#i#i#i#iint16_t q;
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

    /* Configure the Ethernet device. 配置以太网设备*/
    retval = rte_eth_dev_configure(port, rx_rings, tx_rings, &port_conf);
    if (retval < 0)
        rte_exit(EXIT_FAILURE, "Cannot configure device: err=%d, port=%u\n",
                 retval, (unsigned)port);
    retval = rte_eth_dev_adjust_nb_rx_tx_desc(port, &nb_rxd, &nb_txd);
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

    pthread_t tid;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);//线程分离

    context_t *context;//结构体中包含coreId和queueId
    for (int i = 0; i < rx_rings; i++) {
        //创建对应的线程，来执行tx_loop和rx_loop函数
        context = (context_t *)malloc(sizeof(context_t));
        context->core_id = i;
        context->queue_id = i;
        if (pthread_create(&tid, &attr, (void *)rx_loop_m, (void *)context) != 0)
        /*TO do:
        直接修改rx_loop函数，收到包以后就进行修改，然后直接发出去！
        */
            perror("pthread_create error!!\n");
    }
    while (1) {
        sleep(10);
    }
    /*
    To do:
    1.write a while loop to build up packets from tenants and send them to the client
    !! Imitate the code of stats_thread from memcached.
    */
    return 0;
}

static void check_pkt_content(struct rte_mbuf *pkt) {
    uint16_t pktlen = pkt->data_len - MEGA_END_MARK_LEN;
    char *ptr =
        (char *)((char *)rte_pktmbuf_mtod(pkt, char *) + EIU_HEADER_LEN);
    for (int k = 0; k < pktlen; k += 24) {
        assert(*(uint64_t *)(ptr + 8) == (*(uint64_t *)(ptr + 16)) - 1);
    }
}

#ifdef _DUMP_PKT
static inline void pkt_content_dump(struct rte_mbuf *pkt) {
    int cnt = 0;
    uint16_t pktlen = pkt->data_len - EIU_HEADER_LEN;
    uint16_t *ptr = (uint16_t *)((uint8_t *)rte_pktmbuf_mtod(pkt, uint8_t *) +
                                 EIU_HEADER_LEN);
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
    int pktlen = pkt->data_len - EIU_HEADER_LEN;
    uint8_t *ptr = (uint8_t *)((uint8_t *)rte_pktmbuf_mtod(pkt, uint8_t *) +
                               EIU_HEADER_LEN);
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

typedef struct token_get {
    size_t key_length;//无符号整型
    char *key;
} get_token_t;//保存get keylen及key
typedef struct token_set {
    size_t key_length;//无符号整型
    size_t value_length;
    char *key;
    char *value;
} set_token_t;
static inline void process_get_command(Request *get_request) {
        char *key;
        size_t nkey;
        char *fake_value = (char *)malloc(11 * sizeof(char));
        strcpy(fake_value,"perfectxhj");
        key = get_request->key;
        nkey = get_request->key_size;
        if(nkey > 0){
            printf("%s\n",key);
        }
        else{
            printf("key length <=0 !\n");
        }
        get_request->state = 1;
        get_request->value_size = 10;
        get_request->value = fake_value;
        /*
        To do:
        1.从memcached中进行对应的get操作
        2.将get结果写入packet
        3.将结果(成功或者失败)返回
        */
        return;
    }
    //直接让get操作函数输出这个key就可以。
    static inline void process_set_command(Request *set_request) {
        char *key;//key
        size_t nkey;//key size
        char *value;//value
        size_t nvalue;//value size

        key = set_request->key;
        nkey = set_request->key_size;
        nvalue = set_request->value_size;
        value = set_request->value;

        if(nkey > 0){
            printf("%s\n",key);
            if(nvalue > 0){
                printf("%s\n",value);
            }
        }
        /*
        To do:
        1.从memcached中进行对应的set操作
        2.将set结果写入packet
        3.将结果(成功或者失败)返回
        */
        set_request->state = 1;
        return;
    }
static void tx_pkt_padding(uint16_t port, context_t *context, Request *request,struct mbuf_table *tx_buf){
    uint32_t core_id = context->core_id;
    uint32_t queue_id = context->queue_id;
    int pkt_space_left = ETHERNET_MAX_FRAME_LEN - EIU_HEADER_LEN - 2 - tx_buf->pos;
    int key_len_roundup = ROUNDUP(request->key_size,BYTEALIGN);
    int val_len_roundup = ROUNDUP(request->value_size,BYTEALIGN);
    int request_len = 0;
    if(request->operation == 1 && request->state == 1){
        request_len = 8 + key_len_roundup + val_len_roundup;     
    }else{//only need two byte padding
        request_len = 2;
    }
    if(request_len > pkt_space_left && tx_buf->pos == 0){
            printf("request length is larger than the qhole packet!\n");
            abort();
            /*
            To do :
            Use multiple packets to send one request.
            */
    }
    else if(request_len > pkt_space_left && tx_buf->pos != 0){
        //填充ff 并编辑下一个包，如果数组已满，发包出去
        char *ptr = NULL;

        ptr = (char *)((char *)rte_pktmbuf_mtod(
                        tx_buf->m_table[tx_buf->len],
                        char *) +
                    EIU_HEADER_LEN + tx_buf->pos);
        //将剩下的部分全部用FF填充
        for(int i = 0;i < pkt_space_left;i++){
            *(uint8_t *)ptr = 0xFF;
            ptr += sizeof(uint8_t);
        }
        *(uint16_t *)ptr = 0xFFFF;//当前这个packet已经填充好了
        if(unlikely(tx_buf->len + 1 == BURST_SIZE)){
            //发送packet数组，并将标志位置为0
            int nb_tx = rte_eth_tx_burst(port, queue_id,
                                tx_buf->m_table,
                                tx_buf->len + 1);
            core_statistics[core_id].tx += nb_tx;
            tx_buf->len = 0;
            tx_buf->pos = 0;
        }
        else{
            tx_buf->len++;
            tx_buf->pos = 0;
        }
    }
    else{//进行kv填充
        if(tx_buf->pos == 0){//init a new packet
            struct rte_ether_hdr *ethh;
            struct rte_ipv4_hdr *ip_hdr;
            struct rte_udp_hdr *udph;
            //假如当前是一个全新的packet
            udph = (struct rte_udp_hdr *)((char *)rte_pktmbuf_mtod(
                                              tx_buf->m_table[tx_buf->len], char *) +
                                          sizeof(struct rte_ether_hdr) +
                                          sizeof(struct rte_ipv4_hdr));
            udph->src_port = rand();
            udph->dst_port = rand();//这里应该是将要发送的包的udp地址设为随机，实现rss
        }
        char *ptr = NULL;

        ptr = (char *)((char *)rte_pktmbuf_mtod(
                                tx_buf->m_table[tx_buf->len],
                                char *) +
                            EIU_HEADER_LEN + tx_buf->pos);
        if(request->operation == 0){
            if(request->state == 1){//getSucc 2 key_len 2 value_len 4 key  8的倍数 value 8的倍数
                *(uint16_t *)ptr = SET_SUCC;
                ptr += sizeof(uint16_t);
                tx_buf->pos += sizeof(uint16_t);
            }
            else{
                *(uint16_t *)ptr = SET_FAIL;
                ptr += sizeof(uint16_t);
                tx_buf->pos += sizeof(uint16_t);
            }
        }
        else{//get operation
            if(request->state == 1){
                *(uint16_t *)ptr = GET_SUCC;
                ptr += sizeof(uint16_t);
                *(uint16_t *)ptr = request->key_size;//2 byte
                ptr += sizeof(uint16_t);
                *(uint32_t *)ptr = request->value_size;//4 byte
                ptr += sizeof(uint32_t);
                //key and value
                /*
                To do:
                padding the key and value 
                */
                memcpy(ptr,request->key,request->key_size);
                ptr += sizeof(char)*request->key_size;
                if(key_len_roundup != request->key_size){
                    for(int i =0;i<key_len_roundup - request->key_size;i++){
                        *(uint8_t *)ptr = 0x00;
                        ptr += sizeof(uint8_t);
                    }
                }
                memcpy(ptr,request->value,request->value_size);
                ptr += sizeof(char)*request->value_size;
                if(val_len_roundup != request->value_size){
                    for(int i =0;i<val_len_roundup - request->value_size;i++){
                        *(uint8_t *)ptr = 0x00;
                        ptr += sizeof(uint8_t);
                    }
                }
                tx_buf->pos += sizeof(uint64_t) + key_len_roundup + val_len_roundup;
            }
            else{
                *(uint16_t *)ptr = GET_FAIL;
                ptr += sizeof(uint16_t);
                tx_buf->pos += sizeof(uint16_t);
            }
        }
    }
    return;
}
/*
 uint32_t queue_id = context->queue_id;
    uint16_t i, port = 0;
*/
static void rx_pkt_process_m(uint16_t port, context_t *context, struct rte_mbuf **recv_mbufs, uint16_t n_pkts, struct mbuf_table *tx_buf) {//xhj

    //对收到的一整个数组的包进行分析和处理，并将统计结果累加到对应的结构体中（每个核单独统计的）
    //要改为分析和处理每个packet，然后生成对应的回应信息，填充到包里，并在包满时将这个包存到发送数组里，由发送数组进行发送。

    int core_id = (sched_getcpu() - 1) >> 1;//原来是1,3,5奇数
    //成功后，sched_getcpu()返回一个非负的CPU号。如果错误，则返回-1并将errno设置为指示错误。
    char *ptr = NULL;
    for (uint16_t k = 0; k < n_pkts; k++) {
        //针对一个packet
        if (pkt_filter(recv_mbufs[k])) continue;
#ifdef _DUMP_PKT_RECV
        print_pkt_header(recv_mbufs[k]);
        pkt_content_dump(recv_mbufs[k]);
#endif
        ptr = (char *)rte_pktmbuf_mtod(recv_mbufs[k], char *) +
              (recv_mbufs[k]->pkt_len - MEGA_END_MARK_LEN);//MEGA_END_MARK_LEN=2 是OxFFFF
              //rte_pktmbuf_mtod:指向mbuf中数据开头的宏。
        if (*(uint16_t *)ptr != MEGA_PKT_END) {
            core_statistics[core_id].err_ending++;
            //最后两位不是FFFF就代表包出错
            //关键发回包的时候没办法packet大小正好合适，所以在向client发包的时候，肯定有空的位要用FF进行填充。
        }

        uint16_t key_len;
        uint32_t val_len;
        // char *key;
        // char *value;
        if (core_id == 1) {//???? 直接在代码调试的时候来看为什么这么写
            char *w_ptr = (char *)rte_pktmbuf_mtod(recv_mbufs[k], char *) +
                          EIU_HEADER_LEN;
            /*
             * The condition for jump out the while loop cannot be w_ptr == ptr.
             * Because the pkt may be paded to 64 bytes.
             */
            while(*(uint16_t *)w_ptr != MEGA_PKT_END){//当前处理的是同一个租户的packet
                int retval = -1;
                if(*(uint16_t *)w_ptr == MEGA_JOB_SET){//set请求
                    w_ptr += sizeof(uint16_t);//向后移动2 byte 即操作类型2 byte
                    key_len = *(uint16_t *)w_ptr;//2 byte
                    w_ptr += sizeof(uint16_t);
                    val_len = *(uint32_t *)w_ptr;//4 byte
                    w_ptr += sizeof(uint32_t);
                    char *key = (char*)malloc(key_len*sizeof(char));
                    memset(key,'\0',sizeof(key));
                    char *value = (char*)malloc(val_len*sizeof(char));
                    memset(value,'\0',sizeof(value));
                    strncpy(key,w_ptr, sizeof(key));
                    w_ptr += key_len;
                    strncpy(value,w_ptr, sizeof(value));
                    w_ptr += val_len;

                    Request set_request;
                    set_request.key = key;
                    set_request.key_size = key_len;
                    set_request.value_size = val_len;
                    set_request.value = value;
                    set_request.operation = 0;
                    set_request.state = 0;
                    process_set_command(&set_request);
                    if(set_request.state == 1){
                        core_statistics[core_id].set_succ++;
                    }
                    else{
                        core_statistics[core_id].set_fail++;
                    }
                    tx_pkt_padding(port, context, &set_request,tx_buf);
                    //TO do:将返回值添加到packet里面,但是还需要知道他的ip以及端口号
                    //所以在处理这个packet时，就要跟对应的tenant信息相对应
                    //填充packet时直接从tenant结构体中获取信息进行填充
                }
                else if(*(uint16_t *)w_ptr == MEGA_JOB_GET){
                     w_ptr += sizeof(uint16_t);//向后移动2 byte 即操作类型2 byte
                     key_len = *(uint16_t *)w_ptr;//2 byte
                     w_ptr += sizeof(uint16_t);
                     char *key = (char*)malloc((key_len + 1)*sizeof(char));//malloc以后一定要free
                     memset(key,'\0',sizeof(key));
                     strncpy(key,w_ptr, sizeof(key));
                     w_ptr += key_len;

                     Request get_request;
                     get_request.state = 0;
                     get_request.operation = 1;
                     get_request.key = key;
                     get_request.key_size = key_len;
                     process_get_command(&get_request);
                    if(get_request.state == 1){
                        //set成功，将请求写入packet
                        core_statistics[core_id].get_succ++;
                    }
                    else{
                        core_statistics[core_id].get_fail++;
                    }
                    tx_pkt_padding(port, context, &get_request,tx_buf);
                }
                else{
                    core_statistics[core_id].err_ending++;
                    break;
                }
            }//finish parsing a packet
            /*
            To do:
            1.将当前位置的packet后面填充FFFF，并将packet位置向下移动
            */
        }
    }
}

/* Print out statistics on packets dropped */
static void print_stats(void) {
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

        core_statistics[core_id].err_ending = 0;
        core_statistics[core_id].dropped = 0;
        core_statistics[core_id].tx = 0;
        core_statistics[core_id].rx = 0;
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
    fprintf(fp[sched_getcpu()];
#else
    printf("UDPport : src(%d) dst(%d)\n", udph->src_port, udph->dst_port);
#endif
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

static int pkt_filter(const struct rte_mbuf *pkt) {//检查包的地址
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
            return 1;//c/s的地址对应不上就返回1
        }
    }
    return 0;
}
