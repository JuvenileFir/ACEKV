#include "communication.h"

extern Piekv* m_piekv;
static struct benchmark_core_statistics core_statistics[THREAD_NUM];//core_statistics[NUM_MAX_CORE]
struct rte_ether_addr S_Addr = {{0x98, 0x03, 0x9b, 0x8f, 0xb1, 0xc8}};//41的03:00.0
struct rte_ether_addr D_Addr = {{0x98, 0x03, 0x9b, 0x8f, 0xb0, 0x11}};//35的03:00.1
RTWorker::RTWorker(Piekv *piekv, size_t t_id, struct rte_mempool *send_mbuf_pool)
{
    piekv_ = piekv;
    t_id_ = t_id;
    core_id = t_id;

    if (set_core_affinity)
    {
        cpu_set_t mask;
        CPU_ZERO(&mask);
        CPU_SET(core_id, &mask);
        if (pthread_setaffinity_np(pthread_self(), sizeof(mask), &mask) < 0)
            fprintf(stderr, "[Error] set thread affinity failed\n");
    }

    core_statistics[core_id].enable = 1;

    pkt = (struct rte_mbuf *)rte_pktmbuf_alloc((struct rte_mempool *)send_mbuf_pool);
    
    if (unlikely(pkt == NULL))
        rte_exit(EXIT_FAILURE, "Cannot alloc storage memory in  port %" PRIu16 "\n", port);
    
    pkt->nb_segs = 1; // nb_segs
    pkt->ol_flags = PKT_TX_IPV4; // ol_flags
    pkt->vlan_tci = 0;           // vlan_tci
    pkt->vlan_tci_outer = 0;     // vlan_tci_outer
    pkt->l2_len = sizeof(struct rte_ether_hdr);
    pkt->l3_len = sizeof(struct rte_ipv4_hdr);

    ethh = (struct rte_ether_hdr *)rte_pktmbuf_mtod(pkt, unsigned char *);
    ethh->s_addr = S_Addr;
    ethh->d_addr = D_Addr;
    ethh->ether_type = rte_cpu_to_be_16(RTE_ETHER_TYPE_IPV4);

    ip_hdr = (struct rte_ipv4_hdr *)((unsigned char *)ethh + sizeof(struct rte_ether_hdr));
    ip_hdr->version_ihl = IP_VHL_DEF;
    ip_hdr->type_of_service = 0;
    ip_hdr->fragment_offset = 0;
    ip_hdr->time_to_live = IP_DEFTTL;
    ip_hdr->next_proto_id = IPPROTO_UDP;
    ip_hdr->packet_id = 0;
    ip_hdr->src_addr = rte_cpu_to_be_32(IP_SRC_ADDR);
    ip_hdr->dst_addr = rte_cpu_to_be_32(IP_DST_ADDR);

    udph = (struct rte_udp_hdr *)((unsigned char *)ip_hdr + sizeof(struct rte_ipv4_hdr));

    tx_bufs_pt[0] = pkt;  //TODO: change loop here
}

RTWorker::~RTWorker()
{
}

void RTWorker::send_packet()
{
    complement_pkt(tx_bufs_pt[pkt_id], tx_ptr, pktlen); 
    //此行补全了上一行中的RESPOND_COUNTERSE和END_MARK
    pkt_id++;
    pktlen = kEIUHeaderLen;
    if (pkt_id == PKG_GEN_COUNT) {
        for (int k = 0; k < PKG_GEN_COUNT; k++) {
            check_pkt_end(tx_bufs_pt[k]);
        }
        nb_tx = rte_eth_tx_burst(port, t_id_, tx_bufs_pt, PKG_GEN_COUNT);
        core_statistics[core_id].tx += nb_tx;
        pkt_id = 0;
    }
    tx_ptr = (uint8_t *)rte_pktmbuf_mtod(tx_bufs_pt[pkt_id], uint8_t *) + kEIUHeaderLen;
    pktlen += 8; // store response counter in IP pkts.//
    tx_ptr += 8;
}

void RTWorker::parse_set(){
  if (pktlen > (kMaxFrameLen - kResCounterLen - kMaxSetReturn - kEndMarkLen)) {
    send_packet();
  }
    RxSet_Packet *rxset_packet = (RxSet_Packet *)ptr;
    uint64_t key_hash = *(uint64_t *)(ptr + sizeof(RxSet_Packet)+ rxset_packet->key_len);
    
    bool ret = piekv_->set(t_id_, key_hash, ptr + sizeof(RxSet_Packet),
                           rxset_packet->key_len, ptr + sizeof(RxSet_Packet) +
                           rxset_packet->key_len + rxset_packet->key_hash_len,
                           rxset_packet->val_len , false);

    if (ret) {
      rt_counter_.set_succ++;
      *(uint16_t *)tx_ptr = SET_SUCC;
      tx_ptr += SET_RESPOND_LEN;
      pktlen += SET_RESPOND_LEN;
      // rte_delay_us_block(2);
      // rte_delay_us_sleep(1);
      // printf("SUCC!!!t_id:%d,set_key:%lu\n",core_id,*(uint64_t *)(ptr + kHeaderLen));

    } else {
      rt_counter_.set_fail++;
      *(uint16_t *)tx_ptr = SET_FAIL;
      tx_ptr += SET_RESPOND_LEN;
      pktlen += SET_RESPOND_LEN;
      // printf("FAIL!!!t_id:%d,set_key:%lu\n",core_id,*(uint64_t *)(ptr + kHeaderLen));
    }
    ptr += sizeof(RxSet_Packet) + rxset_packet->key_len + 
           rxset_packet->key_hash_len + rxset_packet->val_len;//rxset_packet四值相加

}

void RTWorker::parse_get()
{   
	if (pktlen > (kMaxFrameLen - kResCounterLen - max_get_return - kEndMarkLen)) {   // bwb:超过GET返回包安全length，暂停解析，先进入发包流程;其中GET_MAX_RETURN_LEN=16，原为22 ???
        send_packet();
	}
	RxGet_Packet *rxget_packet = (RxGet_Packet *)ptr;
	uint8_t *key = ptr + kTypeLen + kKeylenLen + khashlenLen; 
	uint64_t key_hash = *(uint64_t *)(key + rxget_packet->key_len);
	// perform get operation, ret represents success or not

	bool ret = piekv_->get(t_id_, key_hash, key, rxget_packet->key_len, 
                           tx_ptr + kHeaderLen + rxget_packet->key_len,
                           (uint32_t *)(tx_ptr + kTypeLen + kKeylenLen));
	if (ret) {
			rt_counter_.get_succ += 1;
			TxGet_Packet *txget_packet = (TxGet_Packet *)tx_ptr;
			txget_packet->result = GET_SUCC;
			txget_packet->key_len = rxget_packet->key_len;
			max_get_return = rxget_packet->key_len + txget_packet->val_len;

			tx_ptr += kHeaderLen; 
			memcpy(tx_ptr, key, rxget_packet->key_len);
			tx_ptr += max_get_return;
			pktlen += kHeaderLen + max_get_return;
	} else {
			rt_counter_.get_fail += 1;
			*(uint16_t *)tx_ptr = GET_FAIL;
			tx_ptr += GET_RESPOND_LEN;
			pktlen += GET_RESPOND_LEN;
	}
	ptr += sizeof(RxGet_Packet) + rxget_packet->key_len + rxget_packet->key_hash_len;
}

void RTWorker::complement_pkt(struct rte_mbuf *pkt, uint8_t *ptr, int pktlen)
{
    uint16_t *counter = (uint16_t *)((uint8_t *)rte_pktmbuf_mtod(pkt, uint8_t *) + kEIUHeaderLen);
    // bwb: ↑ ↑ ↑ 不使用传进来的参数指针，而是重新定位data头指针赋值counter
    *counter = rt_counter_.get_succ;
    rt_counter_.get_succ = 0;
    counter += 1;
    *counter = rt_counter_.set_succ;
    rt_counter_.set_succ = 0;
    counter += 1;
    *counter = rt_counter_.get_fail;
    rt_counter_.get_fail = 0;
    counter += 1;
    *counter = rt_counter_.set_fail;
    rt_counter_.set_fail = 0;

    pktlen += kEndMarkLen;
    *(uint16_t *)ptr = MEGA_PKT_END;

    while ((uint32_t)pktlen < kMinFrameLen)
    {
        ptr += kEndMarkLen;
        pktlen += kEndMarkLen;
        *(uint16_t *)ptr = MEGA_PKT_END;
    }
    pkt->data_len = pktlen; // client tx_loop中在初始化阶段即可执行(因为预先算好)
    pkt->pkt_len = pktlen;  // client tx_loop中在初始化阶段即可执行

    ip_hdr = (struct rte_ipv4_hdr *)((uint8_t *)rte_pktmbuf_mtod(pkt, uint8_t *) + sizeof(struct rte_ether_hdr));
    ip_hdr->total_length = rte_cpu_to_be_16((uint16_t)(pktlen - sizeof(struct rte_ether_hdr)));
    ip_hdr->hdr_checksum = rte_ipv4_cksum(ip_hdr);

    udph = (struct rte_udp_hdr *)((char *)ip_hdr + sizeof(struct rte_ipv4_hdr));
    udph->src_port = t_id_;
    udph->dst_port = t_id_;
    // bwb: ↓ ↓ ↓ client tx_loop中在初始化阶段即可执行
    udph->dgram_len =
        rte_cpu_to_be_16((uint16_t)(pktlen - sizeof(struct rte_ether_hdr) - sizeof(struct rte_ipv4_hdr)));
    udph->dgram_cksum = rte_ipv4_udptcp_cksum(ip_hdr, udph);
    
}

void RTWorker::check_pkt_end(struct rte_mbuf *pkt)
{
    int pkt_len = pkt->data_len;//太小了，远远不足1500

    uint16_t *ptr = (uint16_t *)((uint8_t *)rte_pktmbuf_mtod(pkt, uint8_t *) + (pkt_len - 2));
    // assert(*ptr == MEGA_PKT_END);
    if (*ptr != MEGA_PKT_END)
        printf("end:%d\n",*ptr);
}

bool RTWorker::pkt_filter(const struct rte_mbuf *pkt)
{
    assert(pkt);
    struct rte_ether_hdr *ethh = (struct rte_ether_hdr *)rte_pktmbuf_mtod(pkt, unsigned char *);
    for (int i = 0; i < 6; i++)
    {
        if (ethh->d_addr.addr_bytes[i] != S_Addr.addr_bytes[i] ||
            ethh->s_addr.addr_bytes[i] != D_Addr.addr_bytes[i])
        {
            return true;
        }
    }
    return false;
}

void RTWorker::worker_proc() {
    if (set_core_affinity) {
        cpu_set_t mask;
        CPU_ZERO(&mask);
        CPU_SET(core_id, &mask);
        if (pthread_setaffinity_np(pthread_self(), sizeof(mask), &mask) < 0)
            fprintf(stderr, "[Error] set thread affinity failed\n");
    }
    bool flag = false;

    tx_ptr = (uint8_t *)rte_pktmbuf_mtod(tx_bufs_pt[pkt_id], uint8_t *) + kEIUHeaderLen + kResCounterLen; //  + kResCounterLen
    pktlen = kEIUHeaderLen + kResCounterLen;
    while (piekv_->is_running_) {
        nb_rx = rte_eth_rx_burst(port, t_id_, rx_buf, BURST_SIZE);
        core_statistics[core_id].rx += nb_rx;
        // m_piekv->log_->log_segments_[core_id]->table_stats_->rx_pkt_num += nb_rx;
        m_piekv->hashtable_->all_table_stats_->rx_packet += nb_rx;

#ifdef _DUMP_PKT
        auto pkt_content_dump = [&](struct rte_mbuf *pkt)
        {
            int cnt = 0;
            int pktlen = pkt->data_len - kEIUHeaderLen;
            uint16_t *ptr = (uint16_t *)((uint8_t *)rte_pktmbuf_mtod(pkt, uint8_t *) + kEIUHeaderLen);
            fprintf(fp[sched_getcpu()], "pkt_len: %d\n", pktlen);
            for (int i = 0; i < pktlen - 2; i += 2)
            {
                fprintf(fp[sched_getcpu()], "%04x  ", *ptr);
                ptr++;
                if ((++cnt) % 10 == 0)
                    fprintf(fp[sched_getcpu()], "\n");
            }
            fprintf(fp[sched_getcpu()], "END_MARK: %04x \n", *ptr);
            fprintf(fp[sched_getcpu()], "\n");
        };
#endif

        for (int i = 0; i < nb_rx; i++) {
            if (pkt_filter(rx_buf[i]))
            {
                rte_pktmbuf_free(rx_buf[i]);
                // continue;
            }
#ifdef _DUMP_PKT_RECV
            show_pkt(rx_buf[i]);
            pkt_content_dump(rx_buf[i]);
#endif
            ptr = (uint8_t *)((uint8_t *)rte_pktmbuf_mtod(rx_buf[i], uint8_t *)
                              + kEIUHeaderLen);
            udph = (struct rte_udp_hdr *)((char *)rte_pktmbuf_mtod(rx_buf[i], char *) + 
                                          sizeof(struct rte_ether_hdr) +
                                          sizeof(struct rte_ipv4_hdr));
            
            while (*(uint16_t *)ptr != MEGA_PKT_END) {
	        
            
                if (*(uint16_t *)ptr == MEGA_JOB_GET) {
                    parse_get();

                } else if (*(uint16_t *)ptr == MEGA_JOB_SET) {
                    parse_set();

                } else if (*(uint16_t *)ptr == MEGA_JOB_THREAD) {
                    printf("rx thread\tt_id:%ld\n",t_id_);
                    *(uint16_t *)tx_ptr = GET_THREAD;
                    tx_ptr += SET_RESPOND_LEN;
                    *(uint16_t *)tx_ptr = THREAD_NUM;
                    tx_ptr += 2;
                    pktlen += 4;
                    ptr += 2;//rxset_packet四值相加
                    flag = true;

                } else if (*(uint16_t *)ptr == MEGA_JOB_FINISH) {
                    ptr += 2;//移动ptr
                    __sync_bool_compare_and_swap((volatile uint8_t *)&(piekv_->thread_is_running_[core_id]), 1U, 0U);
                    printf("[INFO]End rx_queue_%ld:%ld\n", t_id_, core_statistics[core_id].rx);
                    return;
                } else {

                    rte_pktmbuf_dump(stdout, rx_buf[i], rx_buf[i]->pkt_len);
                    break;
                }

            }


            if (pktlen != kEIUHeaderLen || pkt_id != 0) {
                
                complement_pkt(tx_bufs_pt[pkt_id], tx_ptr, pktlen);
                pkt_id++;
                for (uint32_t k = 0; k < pkt_id; k++) {
                    check_pkt_end(tx_bufs_pt[k]);
                }
                nb_tx = rte_eth_tx_burst(port, t_id_, tx_bufs_pt, pkt_id);
                if(flag) {
                    printf("Tx Numthreads \n");
                    flag = false;
                }

                core_statistics[core_id].tx += nb_tx;
                pkt_id = 0;
                pktlen = kEIUHeaderLen;
                tx_ptr = (uint8_t *)rte_pktmbuf_mtod(tx_bufs_pt[pkt_id], uint8_t *)
                         + kEIUHeaderLen;
                pktlen += 8;
                tx_ptr += 8;
            }
            rte_pktmbuf_free(rx_buf[i]);

        }
    
    }
    printf("[INFO]End rx_queue_%ld:%ld\n", t_id_, core_statistics[core_id].rx);

}
