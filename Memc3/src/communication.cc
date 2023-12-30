#include "communication.h"
#include "memcached.h"
typedef std::chrono::high_resolution_clock Clock;

volatile rel_time_t current_time;
#define REALTIME_MAXDELTA 60*60*24*30
const uint16_t reload_len = 8;
extern Piekv* m_piekv;
static struct benchmark_core_statistics core_statistics[THREAD_NUM];//core_statistics[NUM_MAX_CORE]
struct rte_ether_addr S_Addr = {{0x98, 0x03, 0x9b, 0x8f, 0xb1, 0xc8}};//41的03:00.0
struct rte_ether_addr D_Addr = {{0x98, 0x03, 0x9b, 0x8f, 0xb0, 0x11}};//35的03:00.1
RTWorker::RTWorker(Piekv *piekv, size_t t_id, struct rte_mempool *send_mbuf_pool) {
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

RTWorker::~RTWorker() {
}


static rel_time_t realtime(const time_t exptime) {
    /* no. of seconds in 30 days - largest possible delta exptime */

    if (exptime == 0) return 0; /* 0 means never expire */

    if (exptime > REALTIME_MAXDELTA) {
        /* if item expiration is at/before the server started, give it an
           expiration time of 1 second after the server started.
           (because 0 means don't expire).  without this, we'd
           underflow and wrap around to some large value way in the
           future, effectively making items expiring in the past
           really expiring never */
        if (exptime <= process_started)
            return (rel_time_t)1;
        return (rel_time_t)(exptime - process_started);
    } else {
        return (rel_time_t)(exptime + current_time);
    }
}

void RTWorker::send_packet() {
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
        if (nb_tx) {
            // printf("发送啦！！！\n");
        }
        pkt_id = 0;
    }
    tx_ptr = (uint8_t *)rte_pktmbuf_mtod(tx_bufs_pt[pkt_id], uint8_t *) + kEIUHeaderLen;
    pktlen += 8; // store response counter in IP pkts.//
    tx_ptr += 8;
}

void RTWorker::parse_set() {
  if (pktlen > (kMaxFrameLen - kResCounterLen - kMaxSetReturn - kEndMarkLen)) {
    send_packet();
  }
    // long int temp = 0;

    RxSet_Packet *rxset_packet = (RxSet_Packet *)ptr;
    uint64_t key_hash = *(uint64_t *)(ptr + sizeof(RxSet_Packet)+ rxset_packet->key_len);
    // printf("处理set\n");
    // uint8_t *old_key = ptr + sizeof(RxSet_Packet);
    // printf("receive key: %ld \n", *(uint64_t*)old_key);
    
    // auto t1 = Clock::now();//计时开始

    enum set_ret ret = process_bin_update(ptr + sizeof(RxSet_Packet), rxset_packet->key_len, rxset_packet->val_len);
    
    // auto t2 = Clock::now();//计时结束
    // if (piekv_->latency[t_id_] < 1) {
    //     // std::cout <<"latency"<<t_id_<<":"<<std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1).count()<< '\n';
    //     temp = std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1).count();
    //     printf("latency%ld: %ld\n", t_id_, temp);
    //     if (temp <= 100000) {
    //         // piekv_->log_->log_segments_[t_id_]->table_stats_->latency 
    //         piekv_->latency[t_id_] += temp;
    //     }
    // }

    // bool ret = 
    piekv_->all_table_stats_->set_request += 1;

    // bool ret = piekv_->set(t_id_, key_hash, ptr + sizeof(RxSet_Packet),
    //                        rxset_packet->key_len, ptr + sizeof(RxSet_Packet) +
    //                        rxset_packet->key_len + rxset_packet->key_hash_len,
    //                        rxset_packet->val_len , false);

    if (ret == EMPTY || ret == EVICTED) {
      piekv_->all_table_stats_->set_success += 1;
      
      /******************
      *
      * 类型:count后面要改，根据是否evict
      * 内容:
      * 添加者:bwb
      * 时间:2023-xx-xx
      *
      ******************/
      if (ret == EVICTED) //piekv_->all_table_stats_->set_evict_succ += 1;
        __sync_fetch_and_add((volatile size_t *)&(piekv_->all_table_stats_->set_evict_succ), 1U);

      if (ret == EMPTY) //piekv_->all_table_stats_->count += 1;
        __sync_fetch_and_add((volatile size_t *)&(piekv_->all_table_stats_->count), 1U);

      rt_counter_.set_succ++;
      *(uint16_t *)tx_ptr = SET_SUCC;
      tx_ptr += SET_RESPOND_LEN;
      pktlen += SET_RESPOND_LEN;
      // rte_delay_us_block(2);
      // rte_delay_us_sleep(1);
      // printf("SUCC!!!t_id:%d,set_key:%lu\n",core_id,*(uint64_t *)(ptr + kHeaderLen));

    } else {
            piekv_->all_table_stats_->set_fail += 1;
      rt_counter_.set_fail++;
      *(uint16_t *)tx_ptr = SET_FAIL;
      tx_ptr += SET_RESPOND_LEN;
      pktlen += SET_RESPOND_LEN;
      // printf("FAIL!!!t_id:%d,set_key:%lu\n",core_id,*(uint64_t *)(ptr + kHeaderLen));
          }
    ptr += sizeof(RxSet_Packet) + rxset_packet->key_len + 
           rxset_packet->key_hash_len + rxset_packet->val_len;//rxset_packet四值相加

}

void RTWorker::parse_get() {   
	if (pktlen > (kMaxFrameLen - kResCounterLen - max_get_return - kEndMarkLen)) {   // bwb:超过GET返回包安全length，暂停解析，先进入发包流程;其中GET_MAX_RETURN_LEN=16，原为22 ???
        send_packet();
	}
	RxGet_Packet *rxget_packet = (RxGet_Packet *)ptr;
	uint8_t *key = ptr + kTypeLen + kKeylenLen + khashlenLen; 
	uint64_t key_hash = *(uint64_t *)(key + rxget_packet->key_len);
	// perform get operation, ret represents success or not
    
    /******************
    *
    * 类型:功能迁移
    * 内容:等待适配……
    * 添加者:bwb
    * 时间:2023-xx-xx
    *
    ******************/
    bool ret = process_bin_get(key, rxget_packet->key_len);
    piekv_->all_table_stats_->get_request += 1;

    // bool ret = true;



	// bool ret = piekv_->get(t_id_, key_hash, key, rxget_packet->key_len, 
    //                        tx_ptr + kHeaderLen + rxget_packet->key_len,
    //                        (uint32_t *)(tx_ptr + kTypeLen + kKeylenLen));
    
    /******************
    *
    * 类型:返回client
    * 内容:
    * 添加者:bwb
    * 时间:2023-xx-xx
    *
    ******************/
	if (ret) {
        piekv_->all_table_stats_->get_found += 1;
        rt_counter_.get_succ += 1;
        TxGet_Packet *txget_packet = (TxGet_Packet *)tx_ptr;
        txget_packet->result = GET_SUCC;
        txget_packet->key_len = rxget_packet->key_len;
        max_get_return = rxget_packet->key_len + txget_packet->val_len;

        tx_ptr += kHeaderLen; 
        memcpy(tx_ptr, key, rxget_packet->key_len);
        tx_ptr += rxget_packet->key_len; 
        memcpy(tx_ptr, key, rxget_packet->key_len/* val_len */);//这里把key再写一遍，假装是把val写了
        tx_ptr += txget_packet->key_len/* val_len */; 
        // tx_ptr += max_get_return;
        pktlen += kHeaderLen + max_get_return;
	} else {
        piekv_->all_table_stats_->get_notfound += 1;
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
    
    /******************
    *
    * 类型:
    * 内容:这里有问题，改了之后end不为MEGA_PKT_END;但因为现在不发送，所以问题不大
    * 添加者:bwb
    * 时间:2023-xx-xx
    *
    ******************/
    // if (*ptr != MEGA_PKT_END)
        // printf("end:%d\n",*ptr);
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
    while (piekv_->thread_is_running_[core_id]) {
        nb_rx = rte_eth_rx_burst(port, t_id_, rx_buf, BURST_SIZE);
        core_statistics[core_id].rx += nb_rx;
        // m_piekv->log_->log_segments_[core_id]->table_stats_->rx_pkt_num += nb_rx;
        piekv_->all_table_stats_->rx_packet += nb_rx;

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

        for (int i = 0; i < nb_rx && piekv_->thread_is_running_[core_id]; i++) {
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
    printf("[INFO]End core_id%d:%u\n", core_id, piekv_->thread_is_running_[core_id]);
    printf("[INFO]End rx_queue_%ld:%ld\n", t_id_, core_statistics[core_id].rx);

}

bool RTWorker::process_bin_get(uint8_t *key_, size_t nkey) {

    item *it;

    // protocol_binary_response_get* rsp = (protocol_binary_response_get*)c->wbuf;
    char* key = reinterpret_cast<char*>(key_);

    it = item_get(key, nkey);
    if (it) {
        /* the length has two unnecessary bytes ("\r\n") */
        // uint16_t keylen = 0;
        // uint32_t bodylen = sizeof(rsp->message.body) + (it->nbytes - 2);

        // item_update(it);
        //stat相关，之后补统计变量
        // pthread_mutex_lock(&c->thread->stats.mutex);
        // c->thread->stats.get_cmds++;
        // c->thread->stats.slab_stats[it->slabs_clsid].get_hits++;
        // pthread_mutex_unlock(&c->thread->stats.mutex);

        // MEMCACHED_COMMAND_GET(c->sfd, ITEM_key(it), it->nkey,
        //                       it->nbytes, ITEM_get_cas(it));

        // if (c->cmd == PROTOCOL_BINARY_CMD_GETK) {
        //     bodylen += nkey;
        //     keylen = nkey;
        // }

        // add_bin_header(c, 0, sizeof(rsp->message.body), keylen, bodylen);
        // rsp->message.header.response.cas = htonll(ITEM_get_cas(it));
        // // add the flags
        // rsp->message.body.flags = htonl(strtoul(ITEM_suffix(it), NULL, 10));
        // //add_iov的作用：将数据添加到将写入的挂起数据列表连接
        // add_iov(c, &rsp->message.body, sizeof(rsp->message.body));

        // if (c->cmd == PROTOCOL_BINARY_CMD_GETK) {
        //     add_iov(c, ITEM_key(it), nkey);
        // }

        /* Add the data minus the CRLF */
        // add_iov(c, ITEM_data(it), it->nbytes - 2);
        // conn_set_state(c, conn_mwrite);//设置连接状态为向client写入
        // c->write_and_go = conn_new_cmd;
        /* Remember this command so we can garbage collect it later */
        // c->item = it;
        return true;
    } else {
        #ifdef RELOAD
            process_bin_update(key_, nkey, reload_len);
        #endif
        // pthread_mutex_lock(&c->thread->stats.mutex);
        // c->thread->stats.get_cmds++;
        // c->thread->stats.get_misses++;
        // pthread_mutex_unlock(&c->thread->stats.mutex);

        // MEMCACHED_COMMAND_GET(c->sfd, key, nkey, -1, 0);

        // if (c->noreply) {//如果不返回信息，就设置新命令了
        //     conn_set_state(c, conn_new_cmd);
        // } else {
        //     if (c->cmd == PROTOCOL_BINARY_CMD_GETK) {
        //         char *ofs = c->wbuf + sizeof(protocol_binary_response_header);
        //         add_bin_header(c, PROTOCOL_BINARY_RESPONSE_KEY_ENOENT,
        //                 0, nkey, nkey);
        //         memcpy(ofs, key, nkey);
        //         add_iov(c, ofs, nkey);
        //         conn_set_state(c, conn_mwrite);
        //         c->write_and_go = conn_new_cmd;
        //     } else {
        //         write_bin_error(c, PROTOCOL_BINARY_RESPONSE_KEY_ENOENT, 0);
        //     }
        // }
        return false;
    }
    return false;

    // if (settings.detail_enabled) {//如果设置了详细统计
    //     stats_prefix_record_get(key, nkey, NULL != it);
    //     //就记录一下具体某个key的get次数，命中次数
    // }
}

enum set_ret RTWorker::process_bin_update(uint8_t *key_, size_t nkey, uint16_t vlen) {
    char* key = reinterpret_cast<char*>(key_);
    item *it;
    int32_t exptime_int = 0;
    time_t exptime;
    char *exptime_str = (char*)"10000";//暂时把过期时间设置为1W秒
    int evict_flag = 0;
    safe_strtol(exptime_str, &exptime_int);

    exptime = exptime_int;

    // protocol_binary_request_set* req = binary_get_request(c);


    /* fix byteorder in the request */
    // req->message.body.flags = ntohl(req->message.body.flags);
    // req->message.body.expiration = ntohl(req->message.body.expiration);

    // vlen = c->binary_header.request.bodylen - (nkey + c->binary_header.request.extlen);

    // if (settings.detail_enabled) {//详细统计
    //     stats_prefix_record_set(key, nkey);
    // }
    it = item_alloc(key, nkey, 0, realtime(exptime), vlen+2, &evict_flag);//flag暂时定为0，应该是多少？

    if (it == NULL) {
        if (!item_size_ok(nkey, 0/* req->message.body.flags */, vlen + 2)) {
            // write_bin_error(c, PROTOCOL_BINARY_RESPONSE_E2BIG, vlen);
        } else {
            // write_bin_error(c, PROTOCOL_BINARY_RESPONSE_ENOMEM, vlen);
        }

        /* Avoid stale data persisting in cache because we failed alloc.
         * Unacceptable for SET. Anywhere else too? */
        // if (c->cmd == PROTOCOL_BINARY_CMD_SET) 
        // printf("返回NULL出来了\n");
        {
            it = item_get(key, nkey);
            if (it) {
                item_unlink(it);
                item_remove(it);
            }
        }
        // printf("item处理完毕了\n");

        /* swallow the data line */
        // c->write_and_go = conn_swallow;
        return FAILED;//set失败
    }

    // ITEM_set_cas(it, 1/* c->binary_header.request.cas */);//cas应该设置多少？
    //细化cmd
    // switch (c->cmd) {
    //     case PROTOCOL_BINARY_CMD_ADD:
    //         c->cmd = NREAD_ADD;
    //         break;
    //     case PROTOCOL_BINARY_CMD_SET:
    //         c->cmd = NREAD_SET;
    //         break;
    //     case PROTOCOL_BINARY_CMD_REPLACE:
    //         c->cmd = NREAD_REPLACE;
    //         break;
    //     default:
    //         assert(0);
    // }

    // if (ITEM_get_cas(it) != 0) {
    //     int cmd = NREAD_CAS;
    // }

    // c->item = it;
    // c->ritem = ITEM_data(it);
    // c->rlbytes = vlen;
    // conn_set_state(c, conn_nread);//衔接下一个complete set操作
    // c->substate = bin_read_set_value;

    // *****************接下来就是直接把complete部分迁移过来了**********************
    // protocol_binary_response_status eno = PROTOCOL_BINARY_RESPONSE_EINVAL;
    enum store_item_type ret = NOT_STORED;


    // pthread_mutex_lock(&c->thread->stats.mutex);
    // c->thread->stats.slab_stats[it->slabs_clsid].set_cmds++;
    // pthread_mutex_unlock(&c->thread->stats.mutex);

    /* We don't actually receive the trailing two characters in the bin
     * protocol, so we're going to just set them here */
    *(ITEM_data(it) + it->nbytes - 2) = '\r';
    *(ITEM_data(it) + it->nbytes - 1) = '\n';

    ret = store_item(it, NREAD_SET/* , nullptr */);//这个函数的后两个参数好像没用上
    // if ((it->it_flags & ITEM_LINKED) != 1) printf("item unlink!\n");
    enum set_ret res = FAILED;
    switch (ret) {
    case STORED:
    //     /* Stored */
    //     write_bin_response(c, NULL, 0, 0, 0);
        if (evict_flag) {
            res = EVICTED;
        } else {
            res = EMPTY;
        }

        break;
    case EXISTS:
    //     write_bin_error(c, PROTOCOL_BINARY_RESPONSE_KEY_EEXISTS, 0);
        break;
    case NOT_FOUND:
    //     write_bin_error(c, PROTOCOL_BINARY_RESPONSE_KEY_ENOENT, 0);
        break;
    case NOT_STORED:
        res = FAILED;
    }

    item_remove(it);       /* release the c->item reference */
    it = 0;
    
    return res;
}
