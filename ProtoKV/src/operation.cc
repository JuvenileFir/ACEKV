#include "piekv.h"
#include "hash.h"
#define HASHLOAD

struct timeval t0, t_init;   

MemPool *kMemPool;
bool allow_mutation = false;
Piekv::Piekv(size_t init_table_size, size_t init_log_size){
    is_running_ = 1;
    max_tput_ = 0;
    for (int i = 0; i < THREAD_NUM; i++)
      thread_is_running_[i] = 1;
    /******************
    *
    * 类型:记录
    * 内容:stop_entry_gc_项根据原注释的意思是用于在H2L是停止垃圾收集？
    *      该项继承自FlexibleKV，PieKV中只有初始化。暂时保留该项。
    * 添加者:bwb
    * 时间:2023-02-24
    *
    ******************/
    stop_entry_gc_ = 0;

    // kMemPool = new MemPool(init_block_size, init_mem_block_number);  // TODO: move this line to main function later, it shouldn't be here
    kMemPool = new MemPool(init_table_size, init_log_size);  
    mempool_ = kMemPool;
    // log_ = new Log(mempool_, init_log_block_number);
    log_ = new Log(mempool_);

    hashtable_ = new HashTable(mempool_);

}

Piekv::~Piekv() {
    printf("deleting piekv......\n");
    delete hashtable_;
    delete log_;
    delete mempool_;
}

bucketStatus Piekv::try_find_insert_bucket(Bucket *bucket_, uint32_t *slot,
                                           const uint16_t tag, const uint8_t *key,
                                           uint32_t keylength) {
	LogSegment *seg = log_->log_segments_[tag % THREAD_NUM];
	bool evict_flag = hashtable_->all_table_stats_->count > kIndexThreshold;//1则检测dangle
	bool rand_flag = hashtable_->all_table_stats_->count < kIndexThreshold2;//1则真随机踢
	bool exist_empty = false;
	bool exist_invalid = false;
	uint32_t cmp_counter = 256, temp_mincnt = 0, temp_invalid = 0;
	*slot = ITEMS_PER_BUCKET;
	for (uint32_t i = 0; i < ITEMS_PER_BUCKET; ++i) {
		if (is_slot_empty(bucket_->item_vec[i])) {
			if (!exist_empty) {
				exist_empty = true;
				*slot = i;//仅定位第一个空位
			}
		} else {
			if (TAG(bucket_->item_vec[i]) == tag) {
				LogItem *item = mempool_->locate_item(SEG(bucket_->item_vec[i]), ITEM_OFFSET(bucket_->item_vec[i]));
				// printf("[INFO]log_key_len:%d\n", ITEMKEY_LENGTH(item->kv_length_vec));
				// printf("key_len:%d\n",keylength);

				if (key_eq(item->data, ITEMKEY_LENGTH(item->kv_length_vec), key, keylength)) {
				// printf("[INFO]key比较相同!!\n");
					
					*slot = i;
					return key_duplicated;//发现有key重复（val不作判断，返回false）
				}
			}

      if (!rand_flag) {
        if (evict_flag && !exist_invalid && 
            !seg->is_valid(ITEM_OFFSET(bucket_->item_vec[i]))) {
          exist_invalid = true;
          temp_invalid = i;//记录失效key
        }

        if (COUNTER(bucket_->item_vec[i]) < cmp_counter) {
          cmp_counter = COUNTER(bucket_->item_vec[i]);
          temp_mincnt = i;//记录counter最小的slot
        }
        // printf("[INFO]非空slot判断流程结束\n");
      }
		}
	}
	// printf("\n");

	//下面的if-else结构可以再参考其他情况进行调整
	if (!exist_empty) {
		if (rand_flag) {
			return full_random;
		} else if (exist_invalid) {
			*slot = temp_invalid;
			return slot_expire;
		} else {
			*slot = temp_mincnt;
			return full_min_count;
		}
	} else {
		return slot_empty;
	}
}

/* bucketStatus Piekv::try_find_insert_bucket(Bucket *bucket_, uint32_t *slot,
                                           const uint16_t tag, const uint8_t *key,
                                           uint32_t keylength) {
	for (uint32_t i = 0; i < 7; ++i) {
		*slot = i;
	}
	return slot_empty;
} */

tablePosition Piekv::cuckoo_insert(uint64_t keyhash, uint16_t tag, twoBucket tb,
                                   const uint8_t *key, size_t keylength) {
  uint32_t slot1, slot2;
  // Bucket * ptr = hashtable_->get_bucket(0);/待成功运行后再替换
  bucketStatus bs1 = try_find_insert_bucket(hashtable_->get_bucket(tb.b1),
                                            &slot1, tag, key, keylength);
  bucketStatus bs2 = try_find_insert_bucket(hashtable_->get_bucket(tb.b2),
                                            &slot2, tag, key, keylength);

	// 下面两if用于判断key重复，对应1和2两种情况
  if (bs1 == key_duplicated) {
    write_unlock_bucket(hashtable_->get_bucket(tb.b2));
    return (tablePosition){tb.b1, slot1, failure_key_duplicated};
  }
  if (bs2 == key_duplicated) {
    write_unlock_bucket(hashtable_->get_bucket(tb.b1));
    return (tablePosition){tb.b2, slot2, failure_key_duplicated};
  }
  
  //下面两if用于直接空位插入，对应3-0
  if (bs1 == slot_empty) {
    write_unlock_bucket(hashtable_->get_bucket(tb.b2));
    return (tablePosition){tb.b1, slot1, ok};
  }
  if (bs2 == slot_empty) {
    write_unlock_bucket(hashtable_->get_bucket(tb.b1));
    return (tablePosition){tb.b2, slot2, ok};
  }

	//下面两if用于覆盖失效entry，对应3-1
  if (bs1 == slot_expire) {
    write_unlock_bucket(hashtable_->get_bucket(tb.b2));
    return (tablePosition){tb.b1, slot1, expire_evicted};
  }
  if (bs2 == slot_expire) {
    write_unlock_bucket(hashtable_->get_bucket(tb.b1));
    return (tablePosition){tb.b2, slot2, expire_evicted};
  }

  if (bs1 == full_random || bs2 == full_random) {
    // uint32_t rdm = random();
    if ((random() & 1U) == 0U) {
      write_unlock_bucket(hashtable_->get_bucket(tb.b1));
      return (tablePosition){tb.b2, static_cast<uint32_t>(tag % 7), rand_overwrite};
    } else {
      write_unlock_bucket(hashtable_->get_bucket(tb.b2));
      return (tablePosition){tb.b1, static_cast<uint32_t>(tag % 7), rand_overwrite};
    }
  }
  
	//没意义，run_cuckoo就应该在这一层搞定
  // if (bs1 == full_min_count && bs2 == full_min_count)
  tablePosition tp;
  if (COUNTER(hashtable_->get_bucket(tb.b1)->item_vec[bs1]) > 
      COUNTER(hashtable_->get_bucket(tb.b2)->item_vec[bs2])) {
    write_unlock_bucket(hashtable_->get_bucket(tb.b1));
    tp = {tb.b2, slot2, failure_table_full};
  } else {
    write_unlock_bucket(hashtable_->get_bucket(tb.b2));
    tp = {tb.b1, slot1, failure_table_full};
  } 

  // /******************
  // *
  // * 类型:隐患(正在解决)
  // * 内容:cuckoo evict用上counter最小的slot
  // * 添加者:bwb
  // * 时间:2023-xx-xx
  // *
  // ******************/
  
	// //以下内容是counter cuckoo evict，其中cold_counter++暂时实现在319行(该函数外面)

  cuckooStatus st = run_cuckoo(hashtable_->get_bucket(0), &tp);

  if (st == ok) {
    assert(is_slot_empty(hashtable_->get_bucket(tp.bucket)->item_vec[tp.slot]));
    
    //  * Since we unlocked the buckets during run_cuckoo, another insert
    //  * could have inserted the same key into either tb.b1 or
    //  * tb.b2, so we check for that before doing the insert.
    //为了防止在run_cuckoo()解锁过程中出现了其他线程的插入导致极端意外情况，所以直接get一下，
		//看有没有插入重复key（相当于忙活半天腾出了位置，结果其他线程又趁机写了一个同key&hash进来)
		//这是有可能的，因为刚进run_cuckoo就把两个初始bkt给解锁了（为了防止迁移过程中死锁）
    
    /******************
    *
    * 类型:
    * 内容:现在不会发生这种情况，而且这种情况需要特殊对待，因此以后再修改
    * 添加者:bwb
    * 时间:2023-03-23
    *
    ******************/
    // tablePosition pos = hashtable_->cuckoo_find(keyhash, tb, key, keylength);
    // if (pos.cuckoostatus == ok) {
    //   pos.cuckoostatus = failure_key_duplicated;
    //   return pos;
    // }
    tp.cuckoostatus = cuckoo_evicted;
    return tp;
  }
  // tp.cuckoostatus = cuckoo_evicted;
  //   return tp;

	// /******************
	// *
	// * 类型:隐患(待补全)
	// * 内容:5路径内没有空位，如何解决？目前是直接返回set错误
	// * 添加者:bwb
	// * 时间:2023-xx-xx
	// *
	// ******************/
  tp.cuckoostatus = failure;
  return tp;//问题所在！！
  // return (tablePosition){0, 0, failure};//问题所在！！

    // tp.cuckoostatus = cuckoo_evicted;
    // return tp;

}

bool Piekv::get(size_t t_id, uint64_t key_hash, const uint8_t *key,
                size_t key_length, uint8_t *out_value,
                uint32_t *in_out_value_length) {

    LogSegment *segmentToGet = log_->log_segments_[t_id];
		segmentToGet->table_stats_->get_count += 1;
    hashtable_->all_table_stats_->get_request += 1;

    // uint32_t block_index = hashtable_->round_hash_->HashToBucket(key_hash);
    // uint32_t bucket_index = key_hash % (hashtable_->bucket_num_);//得到第几个bucket
    // Bucket *bucket = (Bucket *)hashtable_->get_bucket_ptr(bucket_index);

    twoSnapshot *ts1 = (twoSnapshot *)malloc(sizeof(twoBucket));
    twoBucket *tb = (twoBucket *)malloc(sizeof(twoBucket));

    while (1) {
        // const Bucket* located_bucket;
        #ifdef HASHLOAD
        uint32_t hashload = jenkins_hash(key, key_length, 0);//配重
        #endif
        tablePosition tp = hashtable_->get_table(ts1, tb, key_hash, key, key_length);
        uint64_t item_vec;
        
        if (tp.cuckoostatus == ok){
            item_vec = hashtable_->get_bucket(tp.bucket)->item_vec[tp.slot];
        } else if (tp.cuckoostatus == failure_key_not_found) {
            segmentToGet->table_stats_->get_notfound += 1;
						hashtable_->all_table_stats_->get_notfound += 1;
            return false;
        } else {
            return false;
        }
        
        // uint32_t block_id = PAGE(item_vec);
        uint64_t item_offset = ITEM_OFFSET(item_vec);
        // segmentToGet->get_log(out_value,in_out_value_length,block_id,item_offset);
        
        LogItem *item = segmentToGet->locateItem(item_offset);
        
        size_t key_length = 
            std::min(ITEMKEY_LENGTH(item->kv_length_vec), (uint32_t)MAX_KEY_LENGTH);
        size_t value_length = 
            std::min(ITEMVALUE_LENGTH(item->kv_length_vec), (uint32_t)MAX_VALUE_LENGTH);

        memcpy8(out_value, item->data + ROUNDUP8(key_length), value_length);
        *in_out_value_length = value_length;
        
        if (!is_snapshots_same(*ts1, read_two_buckets_end(
            reinterpret_cast<Bucket*>(hashtable_->ptr_), *tb))) continue;
        segmentToGet->table_stats_->get_found += 1;
				hashtable_->all_table_stats_->get_found += 1;

				/******************
				*
				* 类型:
				* 内容1:aLRU暂时关闭
				* 内容2:aLRU尝试打开
				* 添加者:bwb
				* 时间1:2023-3-13
				* 时间2:2023-4-25
				*
				******************/
        // allow_mutation = 1;
        allow_mutation = 0;
        if (allow_mutation)
          move_to_head(reinterpret_cast<Bucket*>(hashtable_->ptr_), &tp,item,
                       key_length, value_length, item_vec, item_offset, t_id);
        break;
    }
    return true;
}

bool Piekv::set(size_t t_id, uint64_t key_hash, uint8_t *key, uint32_t key_len,
                uint8_t *val, uint32_t val_len, bool overwrite) {
	
	assert(key_len <= MAX_KEY_LENGTH);
	assert(val_len <= MAX_VALUE_LENGTH);

	LogSegment *segmentToSet = log_->log_segments_[t_id];
	segmentToSet->table_stats_->set_count += 1;
  hashtable_->all_table_stats_->set_request += 1;

	// Cbool overwriting = false;
  // printf("t_id:%ld,seg_id:%d\n", t_id, segmentToSet->segment_idx_);
  #ifdef HASHLOAD
    uint32_t hashload = jenkins_hash(key, key_len, 0);//配重
    #endif
	uint16_t tag = calc_tag(key_hash);
	// Bucket *temp_ptr = reinterpret_cast<Bucket*>(hashtable_->ptr_);
	Bucket *temp_ptr = hashtable_->get_bucket(0);//看上去更简练
	twoBucket tb = cal_two_buckets(key_hash);
	lock_two_buckets(temp_ptr, tb);
	tablePosition tp = cuckoo_insert(key_hash, tag, tb, key, key_len);
	uint64_t entry = hashtable_->get_bucket(tp.bucket)->item_vec[tp.slot];
  uint32_t cold_counter = COUNTER(entry);
	// if(tp.cuckoostatus) printf("tp cuckoo status:%d\n",tp.cuckoostatus);
	if (tp.cuckoostatus == failure) {
		segmentToSet->table_stats_->set_fail++;
    write_unlock_bucket(&temp_ptr[tp.bucket]);
		return false;
	}

	if (tp.cuckoostatus == rand_overwrite) {
    segmentToSet->table_stats_->set_inplace++;
		tp.cuckoostatus = inplace;
  }

	if (tp.cuckoostatus == cuckoo_evicted) {
		cold_counter++;
    segmentToSet->table_stats_->set_cuckoo_evicted++;
		tp.cuckoostatus = ok;
	}

  if (tp.cuckoostatus == expire_evicted) {
    segmentToSet->table_stats_->set_expire_evicted++;
		tp.cuckoostatus = ok;
	}

	if (tp.cuckoostatus == failure_key_duplicated) {
		LogItem *item = segmentToSet->locateItem(ITEM_OFFSET(entry));
		size_t key_length = std::min(ITEMKEY_LENGTH(item->kv_length_vec),
																	static_cast<uint32_t>(MAX_KEY_LENGTH));
		size_t old_value_length = std::min(ITEMVALUE_LENGTH(item->kv_length_vec),
																				static_cast<uint32_t>(MAX_VALUE_LENGTH));
		
		//目前val_eq()底层只能比较32B大小的两值，且两者len不等时，直接认为不等。
		//下方判断：如果key重复但val不重复，则走后续正常插入流程。只有双重复才返回“失败”
		if (val_eq(val,val_len,item->data + ROUNDUP8(key_length),old_value_length)) {
			// printf("key:%d\tval%d\n",*key,*val);
			// unlock_two_buckets(temp_ptr, tb);//old unlock
      write_unlock_bucket(&temp_ptr[tp.bucket]);
			segmentToSet->table_stats_->set_fail += 1;
			hashtable_->all_table_stats_->set_fail += 1;
			return false;
		} else {
			tp.cuckoostatus = inplace;
		}
	}

	assert(tp.cuckoostatus == ok || tp.cuckoostatus == inplace);
	uint64_t new_item_size = static_cast<uint64_t>(sizeof(LogItem) +
																								 ROUNDUP8(key_len) + 
																								 ROUNDUP8(val_len));//隐患：如果都要round8()，则不适配4+4
	uint64_t item_offset = segmentToSet->AllocItem(new_item_size);//待完善 0308
  
	/******************
	*
	* 类型:
	* 内容:下方if（）是否保留？
	* 添加者:bwb
	* 时间:2023-xx-xx
	*
	******************/
	// if (((item_offset + 1) & segmentToSet->seg_offset_mask_) == 0) {
	// 	unlock_two_buckets(temp_ptr, tb);
	// 	segmentToSet->table_stats_->set_fail += 1;
	// 	hashtable_->all_table_stats_->set_fail += 1;
	// 	return false;
	// }

	// uint64_t new_tail = segmentToSet->get_tail();//仿照mica添加
	segmentToSet->set_item(item_offset, key_hash, key, (uint32_t)key_len,
												 val, (uint32_t)val_len, VALID);
	
	hashtable_->get_bucket(tp.bucket)->item_vec[tp.slot] = ITEM_VEC(tag, cold_counter, item_offset); 
	
	// unlock_two_buckets(temp_ptr, tb);//old
	write_unlock_bucket(&temp_ptr[tp.bucket]);
  segmentToSet->table_stats_->set_success += 1;
  hashtable_->all_table_stats_->set_success += 1;
	if (tp.cuckoostatus == ok) {
		segmentToSet->table_stats_->count += 1;
		hashtable_->all_table_stats_->count += 1;
	}

	return true;
}

//move_to_head有待进一步适配
void Piekv::move_to_head(const Bucket* ptr, tablePosition* tp,
                         const LogItem* item, size_t key_length,
                         size_t value_length, uint64_t item_vec,
                         uint64_t item_offset, size_t t_id) {
    LogSegment *segmentToGet = log_->log_segments_[t_id];
    uint64_t new_item_size = 
        sizeof(LogItem) + ROUNDUP8(key_length) + ROUNDUP8(value_length);
    uint64_t distance_from_tail = 
        (segmentToGet->tail_ - item_offset) % (segmentToGet->seg_log_size_);
    if (distance_from_tail > segmentToGet->seg_log_size_ / 2) {

    // printf("t_id:%ld\tperform——tail:%ld \toffset:%ld\tdist:%ld\tthres:%ld\n",t_id,segmentToGet->get_tail(),item_offset,distance_from_tail,mth_threshold_);

    write_lock_bucket(ptr);
    
    // pool_->lock();//PieKV不锁pool

    // check if the original item is still there
    if (hashtable_->get_bucket(tp->bucket)->item_vec[tp->slot] == item_vec) {//仅验证
        // uint64_t new_tail = segmentToGet->get_tail();
        
        // *****************
        // *
        // * 类型:
        // * 内容:AllocItem_lru()，27日晚饭时已改出新版本
        // * 添加者:bwb
        // * 时间:2023-02-27
        // *
        // *****************
        uint64_t new_item_offset = segmentToGet->AllocItem_lru(new_item_size);

        if (item_offset < mempool_->max_log_size_) {

            LogItem *new_item = segmentToGet->locateItem(new_item_offset);
            //下面一行拷贝log
            memcpy8((uint8_t*)new_item, (const uint8_t*)item, new_item_size);
            //下面一行更新hash table
            hashtable_->get_bucket(tp->bucket)->item_vec[tp->slot] = ITEM_VEC(TAG(item_vec), COUNTER(item_vec), new_item_offset);
            segmentToGet->table_stats_->move_to_head_performed++;
        } else {
            // failed -- original data become invalid in the pool
            segmentToGet->table_stats_->move_to_head_failed++;
        }

        // we need to hold the lock until we finish writing
        write_unlock_bucket(ptr);
        // cleanup_bucket(new_item_offset, new_tail);
    } else {
        //pool_->unlock();//PieKV不锁pool
        write_unlock_bucket(ptr);
        // failed -- original data become invalid in the table
        segmentToGet->table_stats_->move_to_head_failed++;
    }
  } else {
        // printf("t_id:%ld\tskip——tail:%ld \toffset:%ld\tdist:%ld\tthres:%ld\n",t_id,segmentToGet->get_tail(),item_offset,distance_from_tail,mth_threshold_);
        segmentToGet->table_stats_->move_to_head_skipped++;
        // printf("skipped加1 done:%ld\n",segmentToGet->table_stats_->move_to_head_skipped);
  }
}

// void Piekv::directory_proc() {
//   // printf(" == [STAT] printer started on CORE 32 == \n");
//   // bind memflowing thread to core 32
//   cpu_set_t mask;
//   CPU_ZERO(&mask);
//   CPU_SET(32, &mask);

//   if (pthread_setaffinity_np(pthread_self(), sizeof(mask), &mask) < 0) {
//       fprintf(stderr, "[E] set thread affinity failed\n");
//   }

//   while (is_running_) {
//     dir_client_->refresh_server();
//     sleep(1);
//   }
// }

void Piekv::print_trigger() {
    // printf(" == [STAT] printer started on CORE 36 == \n");
    // bind memflowing thread to core 34
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(36, &mask);
    if (pthread_setaffinity_np(pthread_self(), sizeof(mask), &mask) < 0) {
        fprintf(stderr, "[E] set thread affinity failed\n");
    }
    bool do_print = false;
    uint64_t res[7][2];
    memset(res, 0, sizeof(res));
    gettimeofday(&t_init, NULL);
    AllTableStats* stat = hashtable_->all_table_stats_;
    double time_init = t_init.tv_sec + t_init.tv_usec / 1000000.0, time[2]= {0.0, 0.0}, timediff = 0.;
    while (is_running_) {
        gettimeofday(&t0, NULL);
        time[0] = (t0.tv_sec + t0.tv_usec / 1000000.0);
        timediff = time[0] - time[1];
        
        res[0][0] = stat->count;
        res[1][0] = stat->get_found;
        res[2][0] = stat->set_success;
        res[3][0] = stat->rx_packet;
        // res[4][0] = stat->rx_burst;
        res[5][0] = stat->set_request + stat->get_request;

        if (res[0][0] != res[0][1] || res[1][0] != res[1][1] || res[3][0] != res[3][1] ||
            res[2][0] != res[2][1] /* || res[4][0] != res[4][1] */ || res[5][0] != res[5][1]) {
              do_print = true;
              printf("\n[INFO]Current Time:       %.2lf s\n",time[0] - time_init);   
              printf("\n[P]indexs Load Factor:   %.4lf %%\n", stat->count * 100.0 / kSlotNum);
              printf("\n[P]cirlog Load Factor:   %.4lf %%\n", stat->count * 16 * 100.0 / kMemPool->max_log_size_);
            }
        // if (res[4][0]!=res[4][1])  printf("\n[P]rx burst:        %7.3lf M / s\n",
                                          // (res[4][0] - res[4][1]) / timediff /1000000. );
        if (res[3][0]!=res[3][1])  printf("\n[P]rx packet:       %7.3lf / s\n",
                                          (res[3][0] - res[3][1]) / timediff );
        if (res[5][0]!=res[5][1]) {
          printf("\n[P]Throughput:      %7.3lf ops\n",(res[5][0] - res[5][1]) / timediff );
          max_tput_ = std::max(max_tput_, (res[5][0] - res[5][1]) / timediff);
        }
        if (res[0][0]!=res[0][1])  printf("\n[P]Valid Entries:   %7.3lf / s\n",
                                          (res[0][0] - res[0][1]) / timediff );
        if (res[2][0]!=res[2][1])  printf("\n[P]set success:     %7.3lf / s\n",
                                          (res[2][0] - res[2][1]) / timediff );
        if (res[1][0]!=res[1][1])  printf("\n[P]get found:       %7.3lf / s\n",
                                          (res[1][0] - res[1][1]) / timediff );
        if (do_print) printf("\n----------------------------------\n");

        for (int i = 0; i < 6; i++)  {
          res[i][1] = res[i][0];
          res[i][0] = 0;
        }
        time[1] = time[0];
        do_print = false;
        sleep(1);
    }
}