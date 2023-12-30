#pragma once
#ifndef COMMUNICATION_H_
#define COMMUNICATION_H_

// #include <cstdint>
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_cycles.h>
#include <rte_lcore.h>
#include <rte_mbuf.h>
#include <rte_byteorder.h>
#include <rte_ip.h>
#include <rte_ether.h>
#include <rte_flow.h>
#include "piekv.h"
#include "memcached.h"


#define SET_THREAD_NUM 10
#define GET_THREAD_NUM 18
#define MAX_CHECKER 36
#define MAX_THREAD_NUM 72

/* Following protocol speicific parameters should be same with MEGA */
#define MEGA_PKT_END 0xffff
#define MEGA_JOB_GET 0x2
#define MEGA_JOB_SET 0x3
#define MEGA_JOB_THREAD 0x4
#define MEGA_JOB_FINISH 0xf
// #define MEGA_JOB_DEL 0x4 // TODO: DEL is to be implemented
const uint32_t kEndMarkLen = 2;
const uint32_t kTypeLen = 2;
const uint32_t kKeylenLen = 2;
const uint32_t khashlenLen = 2;
const uint32_t kVallenLen = 4;
const uint32_t kHeaderLen = 8;

#define GET_RESPOND_LEN 2U
#define SET_RESPOND_LEN 2U
const uint32_t kMaxSetReturn = 2;

const uint32_t kResCounterLen = 8;
#define SET_SUCC 0x13
#define SET_FAIL 0x23
#define GET_SUCC 0x12
#define GET_FAIL 0x22
#define GET_THREAD 0x2f

// #define THREAD_NUM THREAD_NUM
#define RX_RING_SIZE 1024
#define TX_RING_SIZE 1024

#define NUM_MBUFS 8191
#define MBUF_CACHE_SIZE 250

#define BURST_SIZE (4U)
#define PKG_GEN_COUNT 1

#define MAX_PATTERN_NUM 3
#define MAX_ACTION_NUM 2
#define FULL_MASK 0xffff /* full mask */
#define EMPTY_MASK 0x0   /* empty mask */

// static struct rte_ether_addr S_Addr = {{0x98, 0x03, 0x9b, 0x8f, 0xb0, 0x11}};//{0x98, 0x03, 0x9b, 0x8f, 0xb1, 0xc9}
// static struct rte_ether_addr D_Addr = {{0x98, 0x03, 0x9b, 0x8f, 0xb5, 0xc0}};//{0x04, 0x3f, 0x72, 0xdc, 0x26, 0x24}

// #define IP_SRC_ADDR ((192U << 24) | (168U << 16) | (1U << 8) | 101U)
// #define IP_DST_ADDR ((192U << 24) | (168U << 16) | (1U << 8) | 103U)

#define IP_SRC_ADDR ((10U << 24) | (176U << 16) | (64U << 8) | 41U)
#define IP_DST_ADDR ((10U << 24) | (176U << 16) | (64U << 8) | 35U)

// struct rte_ether_addr S_Addr = {{0x98, 0x03, 0x9b, 0x8f, 0xb1, 0xc9}};//41的03:00.1
// struct rte_ether_addr D_Addr = {{0x04, 0x3f, 0x72, 0xdc, 0x26, 0x25}};//36的02:00.1


// 14(ethernet header) + 20(IP header) + 8(UDP header)
const uint32_t kEIUHeaderLen = 42;
const uint32_t kEthernetHeaderLen = 14;
const uint32_t kMaxFrameLen = 1514;
const uint32_t kMinFrameLen = 64;
// IP pkt header
#define IP_DEFTTL 64 /* from RFC 1340. */
#define IP_VERSION 0x40
#define IP_HDRLEN 0x05 /* default IP header length == five 32-bits words. */
#define IP_VHL_DEF (IP_VERSION | IP_HDRLEN)


#define NUM_MAX_CORE 36
// const size_t page_size = 1048576 * 2;  // page size = 2 MiB

const size_t num_mem_blocks = 240;   // SHM_MAX_PAGES;


struct benchmark_core_statistics {
  uint64_t tx;
  uint64_t rx;
  uint64_t dropped;
  uint64_t err_ending;
  int enable;
} __rte_cache_aligned;


static size_t set_core_affinity = 1;


typedef struct RxGet_Packet
{
    uint16_t operation_type;
    uint16_t key_len;
    uint16_t key_hash_len;
}RxGet_Packet;


typedef struct RxSet_Packet
{
    uint16_t operation_type;
    uint16_t key_len;
    uint16_t key_hash_len;
    uint16_t val_len;
}RxSet_Packet;


typedef struct TxGet_Packet
{
    uint16_t result;
    uint16_t key_len;
    uint32_t val_len;
}TxGet_Packet;


typedef struct RT_Counter
{
     uint16_t get_succ = 0;
     uint16_t set_succ = 0;
     uint16_t get_fail = 0;
     uint16_t set_fail = 0;
}RT_Counter;




class RTWorker
{
private:
    /* data */
    // communication
    struct rte_mbuf *tx_bufs_pt[PKG_GEN_COUNT];
    struct rte_mbuf *rx_buf[BURST_SIZE];
   
    uint32_t pktlen = kEIUHeaderLen, pkt_id = 0;
    uint32_t max_get_return = 64;

    RT_Counter rt_counter_;

    size_t t_id_;
    int core_id;
    uint8_t *ptr = NULL;
    uint16_t port = 0;
    struct rte_ether_hdr *ethh;
    struct rte_ipv4_hdr *ip_hdr;
    struct rte_udp_hdr *udph;
    struct rte_mbuf *pkt;
    uint16_t nb_rx, nb_tx;
    uint8_t *tx_ptr;

    Piekv *piekv_;

public:
    RTWorker(Piekv *piekv, size_t t_id, struct rte_mempool *send_mbuf_pool);
    ~RTWorker();
    void parse_get();
    void parse_set();
    void worker_proc();

    void complement_pkt(struct rte_mbuf *pkt, uint8_t *ptr, int pktlen);
    void check_pkt_end(struct rte_mbuf *pkt);
    bool pkt_filter(const struct rte_mbuf *pkt);
    bool process_bin_get(uint8_t *key_, size_t nkey);
    enum set_ret process_bin_update(uint8_t *key_, size_t nkey, uint16_t vlen);

    void send_packet();
};


#endif
