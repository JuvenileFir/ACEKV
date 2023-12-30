#include "piekv.h"

struct timeval t0, t_latency[4], t_end[4], t_init;   

#ifdef RUN_MICA

#else

#endif
#if defined(size64)
  uint32_t reload_val_len = 64;
#elif defined(size256)
  uint32_t reload_val_len = 256;
#elif defined(size512)
  uint32_t reload_val_len = 512;
#elif defined(size8)
  uint32_t reload_val_len = 8;//8、64、512
  uint8_t temp_val[8]={1,1,1,1,1,1,1,1};
#endif
uint64_t reload_val = 12345678UL;
MemPool *kMemPool;
bool allow_mutation = false;
Piekv::Piekv(size_t init_table_size, size_t init_log_size){
    is_running_ = 1;
    max_tput_ = 0;
    overwrite_trigger_ = false;
    temp_val_ = (uint8_t *)malloc(reload_val_len);
    memset(temp_val_, 0, reload_val_len);//补0至满足位数
    memcpy(temp_val_, &reload_val, 8);
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

//下面这个是protoKV的核心功能实现函数
bucketStatus Piekv::try_find_insert_bucket(Bucket *bucket_, uint32_t *slot,
                                           const uint16_t tag, const uint8_t *key,
                                           uint32_t keylength, int *scan) {
	LogSegment *seg = log_->log_segments_[tag % THREAD_NUM];
	// bool evict_flag = hashtable_->all_table_stats_->count > kIndexThreshold;//1则检测dangle
	bool evict_flag = overwrite_trigger_;//1则检测dangle
	bool rand_flag = hashtable_->all_table_stats_->count < kIndexThreshold2;//1则真随机踢
	bool exist_empty = false;
	bool exist_invalid = false;
  
	uint32_t cmp_counter = 256, temp_mincnt = 0, temp_invalid = 0;
	*slot = slot_num;
	for (uint32_t i = 0; i < slot_num; ++i) {
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
        if (evict_flag && !exist_invalid) {
          (*scan) += 1;//proto扫描次数的统计
          if (!seg->is_valid(ITEM_OFFSET(bucket_->item_vec[i]))) {
            exist_invalid = true;
            temp_invalid = i;//记录失效key
          }
        }
        #ifdef USE_COUNTER //不用counter的话，就不需要做这部分工作
          if (COUNTER(bucket_->item_vec[i]) < cmp_counter) {
            cmp_counter = COUNTER(bucket_->item_vec[i]);
            temp_mincnt = i;//记录counter最小的slot
          }
        #endif
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
//get用的
tablePosition Piekv::get_item_index(const Bucket *bucket_, uint64_t keyhash, uint16_t tag,
                                    const uint8_t *key, size_t keylength, uint32_t bucket_index) {
  uint64_t vector = 0;
  for (uint32_t i = 0; i < slot_num; ++i) {
    vector = bucket_->item_vec[i];
    if (TAG(vector) != tag) continue;
    LogItem *item = mempool_->locate_item(SEG(vector), ITEM_OFFSET(vector));
    if (item->key_hash != keyhash) continue;
    if (key_eq(item->data, ITEMKEY_LENGTH(item->kv_length_vec), key, keylength))
      return (tablePosition){bucket_index, i, ok};//找到了!
	}
  return (tablePosition){bucket_index, slot_num, failure_key_not_found};//没找到
}

//set用的
tablePosition Piekv::find_item_index(const Bucket *bucket_, uint64_t keyhash, uint16_t tag,
                                     const uint8_t *key, size_t keylength, uint32_t bucket_index) {
	bool exist_empty = false;
  uint32_t slot;
  uint64_t vector = 0;
  for (uint32_t i = 0; i < slot_num; ++i) {
    vector = bucket_->item_vec[i];
		if (is_slot_empty(vector)) {
			if (!exist_empty) {
				exist_empty = true;
				slot = i;//仅定位第一个空位
			}
		} else {
			if (TAG(vector) != tag) continue;
      LogItem *item = mempool_->locate_item(SEG(vector), ITEM_OFFSET(vector));
      if (item->key_hash != keyhash) continue;
      if (key_eq(item->data, ITEMKEY_LENGTH(item->kv_length_vec), key, keylength))
        return (tablePosition){bucket_index, i, failure_key_duplicated};//有重复
		}
	}
  if (exist_empty)
    return (tablePosition){bucket_index, slot, ok};//没重复有空位
  // return (tablePosition){bucket_index, get_oldest(bucket_, tag % 4), inplace};//没重复也没空位，LRU覆盖一个
  return (tablePosition){bucket_index, slot_num, failure_table_full};//没重复也没空位，这个按论文说应该LRU覆盖一个，暂时先不管
}

uint32_t Piekv::get_oldest(const Bucket *bucket_, size_t t_id) {
  LogSegment *segment = log_->log_segments_[t_id];
  size_t oldest_item_index = 0;
  uint64_t oldest_item_distance = 0;

  for (size_t i = 0; i < slot_num; i++) {
    uint64_t vector = bucket_->item_vec[i];

    uint64_t distance = segment->distance(ITEM_OFFSET(vector));
    
    /******************
    *
    * 类型:疑问
    * 内容:这里原代码为何写的是if (oldest_item_distance > distance) {}？距离值越取越小？
    * 正常来说，算的的距离最后会转化为非负值，距离值越大，表明越“旧”。因此这里我更改为小于" < " 
    * 添加者:bwb
    * 时间:2023-06-13
    *
    ******************/
    if (oldest_item_distance < distance) {
      oldest_item_distance = distance;
      oldest_item_index = i;
    }
  }
  return oldest_item_index;
}

tablePosition Piekv::cuckoo_insert(uint64_t keyhash, uint16_t tag, twoBucket tb,
                                   const uint8_t *key, size_t keylength,
                                   TableStats *stat, uint16_t* counter) {
  uint32_t slot1, slot2;
  int scan1 = 0, scan2 = 0;

  // Bucket * ptr = hashtable_->get_bucket(0);/待成功运行后再替换
  bucketStatus bs1 = try_find_insert_bucket(hashtable_->get_bucket(tb.b1),
                                            &slot1, tag, key, keylength, &scan1);
  stat->expire_scan += scan1;

  bucketStatus bs2 = try_find_insert_bucket(hashtable_->get_bucket(tb.b2),
                                            &slot2, tag, key, keylength, &scan2);
  stat->expire_scan += scan2;

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
  #ifdef USE_COUNTER
  //仅单个为空的话再补个1
      if ((bs1 == slot_empty && bs2 != slot_empty) ||
        (bs1 != slot_empty && bs2 == slot_empty) ) 
        (*counter)++;
  #endif

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
  //拿tag取余作为随机数
  if (bs1 == full_random || bs2 == full_random) {
    // uint32_t rdm = random();
    if ((random() & 1U) == 0U) {
      write_unlock_bucket(hashtable_->get_bucket(tb.b1));
      return (tablePosition){tb.b2, static_cast<uint32_t>(tag % slot_num), rand_overwrite};
    } else {
      write_unlock_bucket(hashtable_->get_bucket(tb.b2));
      return (tablePosition){tb.b1, static_cast<uint32_t>(tag % slot_num), rand_overwrite};
    }
  }
  
	//没意义，run_cuckoo就应该在这一层搞定
  // if (bs1 == full_min_count && bs2 == full_min_count)
  tablePosition tp;
  #ifdef USE_COUNTER
    if (COUNTER(hashtable_->get_bucket(tb.b1)->item_vec[bs1]) > 
        COUNTER(hashtable_->get_bucket(tb.b2)->item_vec[bs2])) {
      write_unlock_bucket(hashtable_->get_bucket(tb.b1));
      tp = {tb.b2, slot2, failure_table_full};
      // return (tablePosition){tb.b2, slot2, inplace};//看看set有没有问题 - 直接转出没问题

    } else {
      write_unlock_bucket(hashtable_->get_bucket(tb.b2));
      tp = {tb.b1, slot1, failure_table_full};
      // return (tablePosition){tb.b1, slot1, inplace};//看看set有没有问题 - 直接转出没问题

    }
  #else
    if ((random() & 1U) == 0U) {
      write_unlock_bucket(hashtable_->get_bucket(tb.b1));
      tp = {tb.b2, static_cast<uint32_t>(tag % slot_num), failure_table_full};
    } else {
      write_unlock_bucket(hashtable_->get_bucket(tb.b2));
      tp = {tb.b1, static_cast<uint32_t>(tag % slot_num), failure_table_full};
    }
  #endif
  
	// //以下内容是counter cuckoo evict，其中cold_counter++暂时实现在319行(该函数外面)
  int depth = 0, loop_break = 0;
	LogSegment *seg = log_->log_segments_[tag % THREAD_NUM];
  cuckooStatus st = run_cuckoo(hashtable_->get_bucket(0), &tp, &depth, &loop_break, seg);
  if (st == ok) {
    assert(is_slot_empty(hashtable_->get_bucket(tp.bucket)->item_vec[tp.slot]));
    stat->loop_breaknum += loop_break;
    stat->cuckoo_depth += depth;
    stat->cuckoo_count += 1;
    
    //  * Since we unlocked the buckets during run_cuckoo, another insert
    //  * could have inserted the same key into either tb.b1 or
    //  * tb.b2, so we check for that before doing the insert.
    //为了防止在run_cuckoo()解锁过程中出现了其他线程的插入导致极端意外情况，所以直接get一下，
		//看有没有插入重复key（相当于忙活半天腾出了位置，结果其他线程又趁机写了一个同key&hash进来)
		//这是有可能的，因为刚进run_cuckoo就把两个初始bkt给解锁了（为了防止迁移过程中死锁）
    // 10.30补充，我没有刚进run_cuckoo就把初始bkt解锁
    
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

  // 现在应该就不会到这里了
  tp.cuckoostatus = failure;
  return tp;
  // return (tablePosition){0, 0, failure};//问题所在！！
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

    
    tablePosition tp;

    while (1) {
        // const Bucket* located_bucket;
        #ifdef RUN_MICA
          uint32_t v1; 
          #ifdef MICA_15
            uint32_t bucket_index = static_cast<uint32_t>(key_hash >> 41);
          #else
            uint32_t bucket_index = static_cast<uint32_t>(key_hash >> 40);
          #endif
	        uint16_t tag = calc_tag(key_hash);
        	Bucket *bucket = hashtable_->get_bucket(bucket_index);

          while (1) {
            v1 = read_version_begin(bucket);
            // tablePosition 
            tp = get_item_index(bucket, key_hash, tag, key, key_length, bucket_index);
            if (v1 == read_version_end(bucket)) break;
          }

        #else
          twoSnapshot *ts1 = (twoSnapshot *)malloc(sizeof(twoBucket));
          twoBucket *tb = (twoBucket *)malloc(sizeof(twoBucket));
          // tablePosition 
          tp = hashtable_->get_table(ts1, tb, key_hash, key, key_length);
        #endif

        uint64_t item_vec;
        
        if (tp.cuckoostatus == ok){
            item_vec = hashtable_->get_bucket(tp.bucket)->item_vec[tp.slot];
        } else if (tp.cuckoostatus == failure_key_not_found) {
          #ifdef RELOAD
            uint8_t *temp_key = const_cast<uint8_t*>(key);

          #ifdef RELOAD_RECORD

          /******************
          *
          * 类型:修改
          * 内容:set设为false，计入统计
          * 添加者:bwb
          * 时间:2023-10-19
          *
          ******************/
            #if defined(size64) || defined(size256) || defined(size512)
              this->set(t_id, key_hash, temp_key, key_length, temp_val_, reload_val_len, false);
            #elif defined(size8)
              this->set(t_id, key_hash, temp_key, key_length, temp_val, 8, false);//最后一个参数为false说明reload计入stat
            #endif
          #else
            #if defined(size64) || defined(size256) || defined(size512)
              this->set(t_id, key_hash, temp_key, key_length, temp_val_, reload_val_len, true);
            #elif defined(size8)
              this->set(t_id, key_hash, temp_key, key_length, temp_val, 8, true);//最后一个参数为true说明reload不计入stat
            #endif
          #endif

          #endif
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
        #ifdef RUN_MICA
        
        /******************
        *
        * 类型:TODO
        * 内容:实现proto-mica的单version快照比较；包括下面这里还有上面的get_item_index()调用处
        * 添加者:bwb
        * 时间:2023-06-12
        *
        ******************/
            if (v1 != read_version_end(bucket)) continue;
        #else
        if (!is_snapshots_same(*ts1, read_two_buckets_end(
            reinterpret_cast<Bucket*>(hashtable_->ptr_), *tb))) continue;
        #endif
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
        allow_mutation = 1;
        // allow_mutation = 0;
        if (allow_mutation)
          move_to_head(reinterpret_cast<Bucket*>(hashtable_->ptr_), &tp,item,
                       key_length, value_length, item_vec, item_offset, t_id);
        break;
    }
    return true;
}

bool Piekv::set(size_t t_id, uint64_t key_hash, uint8_t *key, uint32_t key_len,
                uint8_t *val, uint32_t val_len, bool norecord) {
	
	assert(key_len <= MAX_KEY_LENGTH);
  if (val_len > MAX_VALUE_LENGTH) printf("超过上限了!!!val_length:%u\n", val_len);
	assert(val_len <= MAX_VALUE_LENGTH);
  bool expire_compensate = false;

	LogSegment *segmentToSet = log_->log_segments_[t_id];
  if (!norecord) {
	  segmentToSet->table_stats_->set_count += 1;
    hashtable_->all_table_stats_->set_request += 1;
  }

	uint16_t tag = calc_tag(key_hash);
	assert(t_id == tag % THREAD_NUM);
  Bucket *temp_ptr = hashtable_->get_bucket(0);
  
#ifdef RUN_MICA
  #ifdef MICA_15
    uint32_t bucket_index = static_cast<uint32_t>(key_hash >> 41);
  #else
    uint32_t bucket_index = static_cast<uint32_t>(key_hash >> 40);
  #endif

	Bucket *bucket = hashtable_->get_bucket(bucket_index);
	assert(bucket <= &temp_ptr[bucket_index]);
  write_lock_bucket(bucket);
  // write_lock_bucket(&temp_ptr[bucket_index]/* , t_id */);

	tablePosition tp = find_item_index(bucket, key_hash, tag, key, key_len, bucket_index);
  if (tp.cuckoostatus == failure_table_full) {
    //get_empty_or_oldest()这里实际上可以用mica的这个函数找所谓旧项
    tp.slot = get_oldest(bucket, t_id);
    if (!norecord) {
      if (!segmentToSet->is_valid(ITEM_OFFSET(bucket->item_vec[tp.slot])))
        hashtable_->all_table_stats_->set_evicted_invalid += 1;
      segmentToSet->table_stats_->set_mica_evicted += 1;
      hashtable_->all_table_stats_->set_evicted += 1;
    }
    tp.cuckoostatus = inplace;
	}

	uint64_t entry = bucket->item_vec[tp.slot];
	assert(bucket == hashtable_->get_bucket(tp.bucket));
	assert(bucket == &temp_ptr[tp.bucket]);

#else
  twoBucket tb = cal_two_buckets(key_hash);
	lock_two_buckets(temp_ptr, tb);
  uint16_t temp_counter = 0;
	tablePosition tp = cuckoo_insert(key_hash, tag, tb, key, key_len, segmentToSet->table_stats_, &temp_counter);
	uint64_t entry = hashtable_->get_bucket(tp.bucket)->item_vec[tp.slot];
  uint16_t cold_counter = COUNTER(entry);
	// if(tp.cuckoostatus) printf("tp cuckoo status:%d\n",tp.cuckoostatus);
  #ifdef USE_COUNTER
    if (tp.cuckoostatus == ok) {
      cold_counter += 1;
      cold_counter += temp_counter;
    }
  #endif

	if (tp.cuckoostatus == failure) {
    if (!norecord) {
		  segmentToSet->table_stats_->set_fail++;
    }
    write_unlock_bucket(&temp_ptr[tp.bucket]);
		return false;
	}

	if (tp.cuckoostatus == rand_overwrite) {
    if (!norecord) {
      segmentToSet->table_stats_->set_inplace++;
    }
		tp.cuckoostatus = inplace;
  }

	if (tp.cuckoostatus == cuckoo_evicted) {
    #ifdef USE_COUNTER
		  cold_counter += 2;
    #endif
    if (!norecord) {
      segmentToSet->table_stats_->set_cuckoo_evicted++;
    }
		tp.cuckoostatus = ok;
	}

  if (tp.cuckoostatus == expire_evicted) {//成功驱逐的数量
    if (!norecord) {
      segmentToSet->table_stats_->set_expire_evicted++;
      hashtable_->all_table_stats_->expired_evicted++;
    }
    expire_compensate = true;
		tp.cuckoostatus = ok;
	}
#endif
  
  /******************
  *
  * 类型:待思考
  * 内容:这种替换是合理的吗？
  *     不管合理不合理，目前的client不会触发key重复的情况
  * 添加者:bwb
  * 时间:2023-06-13
  *
  ******************/
	if (tp.cuckoostatus == failure_key_duplicated) {
		LogItem *item = segmentToSet->locateItem(ITEM_OFFSET(entry));
		size_t key_length = std::min(ITEMKEY_LENGTH(item->kv_length_vec),
																	static_cast<uint32_t>(MAX_KEY_LENGTH));
		size_t old_value_length = std::min(ITEMVALUE_LENGTH(item->kv_length_vec),
																				static_cast<uint32_t>(MAX_VALUE_LENGTH));
		
		//目前val_eq()底层只能比较32B大小的两值，且两者len不等时，直接认为不等。
		//下方判断：如果key重复但val不重复，则走后续正常插入流程。只有双重复才返回“失败”
		if (val_eq(val,val_len,item->data + ROUNDUP8(key_length),old_value_length)) {
      if (!norecord) {
  			segmentToSet->table_stats_->set_fail += 1;
	  		hashtable_->all_table_stats_->set_fail += 1;
      }
      write_unlock_bucket(&temp_ptr[tp.bucket]);
    	return false;
		} else {
      if (!norecord) 
			  segmentToSet->table_stats_->set_overwrite_key_duplicate += 1;
			tp.cuckoostatus = inplace;
		}
	}

	assert(tp.cuckoostatus == ok || tp.cuckoostatus == inplace);
	uint64_t new_item_size = static_cast<uint64_t>(sizeof(LogItem) +
																								 ROUNDUP8(key_len) + 
																								 ROUNDUP8(val_len));//隐患：round8()不适配4+4
  
  /******************
  *
  * 类型:问题解决
  * 内容:死锁问题所在，通过增大log空间解决了这里的问题
  * 添加者:bwb
  * 时间:2023-06-14
  *
  ******************/
	uint64_t item_offset = segmentToSet->AllocItem(new_item_size, tp.cuckoostatus == ok);
  
	/******************
	*
	* 类型:
	* 内容:下方检查是否保留
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

  // 在这里判断一下有没有旧项……并返回清理
  #ifdef MEMC3_EVICT
    LogItem *it = segmentToSet->locateItem(item_offset);
    uint64_t old_keyhash;
    uint8_t *old_key = NULL;
    old_keyhash = it->key_hash;
    old_key = it->data;
    if (old_keyhash != 0 && *(uint64_t*)old_key != 0) {
      // memcpy8(old_key, it->data, 8);
      hashtable_->unlink(old_key, old_keyhash);
    }
  #endif

	// uint64_t new_tail = segmentToSet->get_tail();//仿照mica添加
	segmentToSet->set_item(item_offset, key_hash, key, (uint32_t)key_len,
												 val, (uint32_t)val_len, VALID);


#ifdef RUN_MICA
	hashtable_->get_bucket(tp.bucket)->item_vec[tp.slot] = ITEM_VEC(tag, item_offset);

#else
	hashtable_->get_bucket(tp.bucket)->item_vec[tp.slot] = ITEM_VEC(tag, cold_counter, item_offset);
	// &temp_ptr[tp.bucket].item_vec[tp.slot] = ITEM_VEC(tag, cold_counter, item_offset);//编译报错
#endif	
	// unlock_two_buckets(temp_ptr, tb);//原来的两个bkt都备选，因此到这里才全部解锁

	write_unlock_bucket(&temp_ptr[tp.bucket]);
#ifdef RUN_MICA
  #ifdef CLEANUP
    cleanup_bucket(item_offset, segmentToSet->tail_, segmentToSet);
  #endif
#endif
  if (!norecord) {
    segmentToSet->table_stats_->set_success += 1;
    hashtable_->all_table_stats_->set_success += 1;
  }
  //只有ok额外加count，其他没区别
  // #ifdef RELOAD_NORECORD
	//   if (!expire_compensate) //seq下肯定不会发生inplace，除非reload过，但norecord有必要重新计上
  // #else
	//   if (tp.cuckoostatus == ok && !expire_compensate) 
  // #endif
	  if (tp.cuckoostatus == ok && !expire_compensate) {
	// if (tp.cuckoostatus == ok) {
    if (!norecord) {
		  segmentToSet->table_stats_->count += 1;
		  hashtable_->all_table_stats_->count += 1;
    }
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
    uint64_t distance_from_tail = segmentToGet->distance(item_offset);
    if (distance_from_tail > segmentToGet->seg_log_size_ / 2) {

    write_lock_bucket(ptr);
    
    if (hashtable_->get_bucket(tp->bucket)->item_vec[tp->slot] == item_vec) {//仅验证
      // uint64_t new_tail = segmentToGet->get_tail();
      
      uint64_t new_item_offset = segmentToGet->AllocItem_lru(new_item_size);

      if (segmentToGet->is_valid(item_offset)) {

          LogItem *new_item = segmentToGet->locateItem(new_item_offset);
          //下面一行拷贝log
          memcpy8((uint8_t*)new_item, (const uint8_t*)item, new_item_size);
          //下面一行更新hash table
          #ifdef RUN_MICA
            hashtable_->get_bucket(tp->bucket)->item_vec[tp->slot] = ITEM_VEC(TAG(item_vec), new_item_offset);
          #else
            hashtable_->get_bucket(tp->bucket)->item_vec[tp->slot] = ITEM_VEC(TAG(item_vec), COUNTER(item_vec), new_item_offset);
          #endif
          segmentToGet->table_stats_->move_to_head_performed += 1;
      } else {
          // failed -- original data become invalid in the pool
          segmentToGet->table_stats_->move_to_head_failed += 1;
      }

      write_unlock_bucket(ptr);
      #ifdef RUN_MICA
        #ifdef CLEANUP
          cleanup_bucket(new_item_offset, segmentToGet->tail_, segmentToGet);
        #endif
      #endif
    } else {
        segmentToGet->table_stats_->move_to_head_failed += 1;
        write_unlock_bucket(ptr);
    }
  } else {
        segmentToGet->table_stats_->move_to_head_skipped += 1;
  }
}


void Piekv::cleanup_bucket(uint64_t old_tail, uint64_t new_tail, LogSegment *segment) {
//这里mica的offset要右移23位，因为mica的offset本身就48位，右移23刚好差不多是25bit，约等于bucket数量(7*24bit)
//经过检查mica的设置，确实是这样的，让offset右移到刚好比num_buckets多，以至于再移一位就小的程度。这样使得offset差不多转一圈，cleanup也能循环一圈的程度
//
  uint64_t bucket_index = (old_tail >> 23) & (kBucketNum - 1);
  uint64_t bucket_index_end = (new_tail >> 23/* rshift_ */) & (kBucketNum - 1);
  if (bucket_index != bucket_index_end) 
  while (bucket_index != bucket_index_end) {
    Bucket* bucket = hashtable_->get_bucket(bucket_index);

    write_lock_bucket(bucket);

    Bucket* current_bucket = bucket;
    size_t item_index;
    for (item_index = 0; item_index < slot_num; item_index++) {
      uint64_t* item_vec_p = &current_bucket->item_vec[item_index];
      if (*item_vec_p == 0) continue;
      segment->table_stats_->expire_scan++;

      if (!segment->is_valid(ITEM_OFFSET(*item_vec_p))) {
        *item_vec_p = 0;
        segment->table_stats_->count -= 1;
        segment->table_stats_->cleanup_succ += 1;
        hashtable_->all_table_stats_->expired_evicted++;
      }
    }
    segment->table_stats_->cleanup_scan += 1;

  	write_unlock_bucket(bucket);

    bucket_index = (bucket_index + 1UL) & (kBucketNum - 1);
  }
}


void Piekv::print_trigger(double trigger) {
    // printf(" == [STAT] printer started on CORE 36 == \n");
    // bind memflowing thread to core 34
    printf("trigger:%lf \n", trigger);
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(36, &mask);
    if (pthread_setaffinity_np(pthread_self(), sizeof(mask), &mask) < 0) {
        fprintf(stderr, "[E] set thread affinity failed\n");
    }
    bool do_print = false, minimize = false;
    double iLF = 0.0, fbLF = 0.0;
    uint64_t res[14][2], cuckoonum = 0;
    memset(res, 0, sizeof(res));
    gettimeofday(&t_init, NULL);
    AllTableStats* stat = hashtable_->all_table_stats_;
    double time_init = t_init.tv_sec + t_init.tv_usec / 1000000.0, time[2]= {0.0, 0.0}, timediff = 0.;
    while (is_running_) {
        gettimeofday(&t0, NULL);
        time[0] = (t0.tv_sec + t0.tv_usec / 1000000.0);
        timediff = time[0] - time[1];
        res[0][0] = stat->count;
        res[2][0] = stat->set_success;
        res[3][0] = stat->expired_evicted;//expire evict (clean up / caleKV的失效清除)
        res[4][0] = stat->set_evicted;//mica evict
        res[1][0] = stat->get_found;
        res[6][0] = stat->get_request;
        res[7][0] = stat->set_request;
        res[5][0] = stat->set_request + stat->get_request;
        res[8][0] = log_->log_segments_[0]->table_stats_->cuckoo_depth +
                    log_->log_segments_[1]->table_stats_->cuckoo_depth +
                    log_->log_segments_[2]->table_stats_->cuckoo_depth +
                    log_->log_segments_[3]->table_stats_->cuckoo_depth;
        res[9][0] = log_->log_segments_[0]->table_stats_->expire_scan +
                    log_->log_segments_[1]->table_stats_->expire_scan +
                    log_->log_segments_[2]->table_stats_->expire_scan +
                    log_->log_segments_[3]->table_stats_->expire_scan;
        cuckoonum = log_->log_segments_[0]->table_stats_->cuckoo_count +
                    log_->log_segments_[1]->table_stats_->cuckoo_count + 
                    log_->log_segments_[2]->table_stats_->cuckoo_count + 
                    log_->log_segments_[3]->table_stats_->cuckoo_count;

        // res[10][0] = stat->rx_packet;
        // res[11][0] = stat->rx_burst;
        res[12][0] = stat->set_evicted_invalid;//MICA的FIFO中实际清除的失效部分

        // res[10][0] = log_->log_segments_[0]->table_stats_->latency;
        // res[11][0] = log_->log_segments_[1]->table_stats_->latency;
        // res[12][0] = log_->log_segments_[2]->table_stats_->latency;
        // res[13][0] = log_->log_segments_[3]->table_stats_->latency;

        // iLF = hashtable_->calc_load_factor();

        uint64_t num_entry = 0, num_bucket = 0;
        Bucket *bucket;
        for (uint64_t i = 0; i < hashtable_->bucket_num_; i++) {
          bucket = hashtable_->get_bucket(i);
          int cnt = 0;
          for (uint32_t j = 0; j < slot_num; j++) {
            uint64_t vec = bucket->item_vec[j];
            if (!is_slot_empty(vec)) {
              num_entry++;
              cnt++;
            }
          }
          if (cnt == slot_num) num_bucket++;
        }
        iLF = num_entry * 100.0 / kSlotNum;
        fbLF = num_bucket * 100.0 / hashtable_->bucket_num_;

        // overwrite_trigger_ = iLF > kTrigger;//自适应阶段阈值设计
        overwrite_trigger_ = iLF > trigger;//自适应阶段阈值设计

        if (res[0][0] != res[0][1] || res[1][0] != res[1][1] ||
            res[7][0] != res[7][1] || res[6][0] != res[6][1]) {
              do_print = true;

              printf("\n[INFO]Current Time:       %.2lf s\n",time[0] - time_init);   
              printf("\n[P]indexs Load Factor:    %.4lf %%\n", iLF);
              printf("\n[P]full bucket LFactor:   %.4lf %%\n", fbLF);
              // printf("\n[P]set latency:          %zu \n", res[10][0] - res[10][1]);
              // printf("[P]set latency:          %zu \n", res[11][0] - res[11][1]);
              // printf("[P]set latency:          %zu \n", res[12][0] - res[12][1]);
              // printf("[P]set latency:          %zu \n", res[13][0] - res[13][1]);
              printf("\n[P]set request:           %zu \n", res[7][0]/* stat->set_request */);
              printf("[P]get request:           %zu \n", res[6][0]/* stat->get_request */);

             if (!minimize) {
                printf("\n[P]Get Hit Rate:          %.4lf %%\n", (res[1][0] - res[1][1]) * 100.0 / (res[6][0] - res[6][1]));
                printf("[P]ACC Hit Rate:          %.4lf %%\n", res[1][0] * 100.0 / res[6][0]);
              
                // printf("\n[P]mica ttl-evicted:      %zu / s\n", res[4][0] );
                // printf("[P]mica ttl-evt-invalid:  %zu / s\n", res[12][0] );
                // printf("[P]mica ttl-evict-valid:  %zu / s\n", res[4][0] - res[12][0] );
                
                printf("\n[P]total cuckoo num:      %lu \n", cuckoonum);
                printf("[P]total cuckoo path:     %lu \n", res[8][0]);
                printf("[P]total cp perset:       %.6lf / set\n", res[8][0] * 1.0 / res[2][0] );
                printf("[P]total cp percuckoo:    %.6lf / cuckoo num\n", res[8][0] * 1.0 / cuckoonum );

              if (res[8][0]!=res[8][1]) {
                printf("\n[P]cuckoo path / s:       %.3lf / s\n", (res[8][0] - res[8][1]) / timediff );
                printf("[P]cp per set * s:        %.6lf / set * s\n", (res[8][0] - res[8][1]) * 1.0 / (res[2][0] - res[2][1]) );
              } else {
                printf("\n[P]cuckoo path / s:       %d / s\n", 0);
                printf("[P]cp per set * s:        %d / set * s\n", 0);
              }
              if (res[13][0]!=res[13][1]) {
                printf("\n[P]break loop / s:        %.3lf / s\n", (res[13][0] - res[13][1]) / timediff );
                printf("[P]break / set :          %.4lf %%\n", (res[13][0] - res[13][1]) * 100.0 / (res[2][0] - res[2][1]) );
              } else {
                printf("\n[P]break loop / s:        %d / s\n", 0);
                printf("[P]break / set :          %d %%\n", 0);
              }
        
              
              // printf("\n[P]indexs Load Factor:   %.4lf %%\n", stat->count * 100.0 / kSlotNum);
              // printf("\n[P]cirlog Load Factor:   %.4lf %%\n", stat->count * 10 * 100.0 / kMemPool->max_log_size_);
            }
          }
        // if (res[4][0]!=res[4][1])  printf("\n[P]rx burst:        %7.3lf M / s\n",
                                          // (res[4][0] - res[4][1]) / timediff /1000000. );
        // if (!minimize && res[10][0]!=res[10][1]) printf("\n[P]rx packet:             %7.3lf / s\n",
                                          // (res[10][0] - res[10][1]) / timediff );
        // if (!minimize && res[11][0]!=res[11][1]) printf("\n[P]rx burst:              %7.3lf / s\n",
                                          // (res[11][0] - res[11][1]) / timediff );
        if (!minimize && res[5][0]!=res[5][1]) {
          printf("\n[P]Throughput:            %7.3lf ops\n", (res[5][0] - res[5][1]) / timediff );
          printf("[P]all request:           %lu o\n", res[5][0] );
          max_tput_ = std::max(max_tput_, (res[5][0] - res[5][1]) / timediff);
        }
        if (!minimize && res[6][0]!=res[6][1]) {
          printf("\n[P]GetThroughput:         %7.3lf ops\n",(res[6][0] - res[6][1]) / timediff );
        }
        if (!minimize && res[7][0]!=res[7][1]) {
          printf("[P]SetThroughput:         %7.3lf ops\n",(res[7][0] - res[7][1]) / timediff );
          // printf("\n[P]set_request ++:        %lu\n",res[7][0] - res[7][1]);
        }
        if (!minimize && res[0][0]!=res[0][1]) printf("\n[P]Valid Entries:         %7.3lf / s\n",
                                          (res[0][0] - res[0][1]) / timediff );
        //mica相关
        if (!minimize && res[4][0]!=res[4][1]) {
          printf("[P]mica evicted:          %.6lf %%\n", ((res[4][0] - res[4][1]) / timediff) * 100.0 / (iLF/*  * kSlotNum */));
          printf("[P]mica evict-invalid-ps: %.6lf %%\n", ((res[12][0] - res[12][1]) / timediff) * 100.0 / (iLF/*  * kSlotNum */) );
          printf("[P]mica evict-valid-ps:   %.6lf %%\n", (((res[4][0] - res[4][1]) - (res[12][0] - res[12][1])) / timediff) * 100.0 / (iLF/*  * kSlotNum */) );
          printf("[P]mica evict-valid:      %.6lf %%\n", ((res[4][0] - res[4][1]) - (res[12][0] - res[12][1])) * 100.0 / (res[4][0] - res[4][1]) );
          printf("[P]mica evict-invalid:    %.6lf %%\n", (res[12][0] - res[12][1]) * 100.0 / (res[4][0] - res[4][1]) );

        } else if (res[0][0]!=res[0][1]) {
          if (!minimize) {
            printf("[P]mica evicted:          %lu / s\n", res[4][0] - res[4][1]);//没有就显示0
            printf("[P]mica evict-invalid-ps: %lu / s\n", res[4][0] - res[4][1]);
            printf("[P]mica evict-valid-ps:   %lu / s\n", res[4][0] - res[4][1]);
            printf("[P]mica evict-valid:      %lu / s\n", res[4][0] - res[4][1]);
            printf("[P]mica evict-invalid:    %lu / s\n", res[4][0] - res[4][1]);
          }
        }
        if (!minimize && res[2][0]!=res[2][1]) {
          printf("[P]set success:           %7.3lf / s\n", (res[2][0] - res[2][1]) / timediff );
          // printf("\n[P]set_succ ++:           %lu\n", res[2][0] - res[2][1] );
          // printf("\n[P]setsuccess rate:       %7.3lf  %%\n", (res[2][0] - res[2][1]) * 100.0 / (res[7][0] - res[7][1]));
        }  
        
        if (!minimize && res[9][0]!=res[9][1]) {
          printf("\n[P]expire scan persec:    %7.3lf / s\n", (res[9][0] - res[9][1]) / timediff );
          printf("[P]total expire scan:     %ld\n", res[9][0] );
          printf("[P]expire hit persec:     %7.3lf / s\n", (res[3][0] - res[3][1]) / timediff );
          printf("[P]total expire hit:      %ld\n", res[3][0] );
          printf("\n[P]expire hit rate:       %.6lf %%\n", (res[3][0] - res[3][1]) * 100.0/ (res[9][0] - res[9][1]) );
          printf("[P]total expire hit rate: %.6lf %%\n", res[3][0] * 100.0/ res[9][0] );
        }  
        if (!minimize && res[1][0]!=res[1][1]) printf("\n[P]get found:             %7.3lf / s\n",
                                          (res[1][0] - res[1][1]) / timediff );
        if (do_print) printf("\n----------------------------------\n");

        for (int i = 0; i < 14; i++)  {
          res[i][1] = res[i][0];
          res[i][0] = 0;
        }
        time[1] = time[0];
        do_print = false;
        // sleep(1);
        usleep(300000);
        #ifdef LATENCY
          for (int i = 0; i < 4; i++)  {
            latencys[i] = 0;        
            latencyg[i] = 0;        
          }
        #endif

    }
}
