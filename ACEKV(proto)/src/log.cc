#include "log.h"

LogSegment::LogSegment(MemPool *mempool, int index) {
    mempool_ = mempool;
    segment_idx_ = index;
    segment_ptr_ = reinterpret_cast<uint8_t*>(mempool_->log_ptr_[index]);
    seg_log_size_ = ROUNDDOWN40(mempool_->max_log_size_);//1<<32 向下取整 40 的倍数
    
    seg_offset_mask_ = ITEM_OFFSET_MASK >> 2;//每个segment分得的offset部分的mask,为(1<<46) - 1
    head_ = index * kSegOffset;
    tail_ = index * kSegOffset;
    size_ = 0;
    dynamic_end_ = (index + 1) * kSegOffset;
    store_stats_ = new StoreStats();
    table_stats_ = new TableStats();
    memset(store_stats_,0,sizeof(StoreStats));
    memset(table_stats_,0,sizeof(TableStats));
}

LogSegment::~LogSegment() {
    delete store_stats_;
    delete table_stats_;
}

//new log()
Log::Log(MemPool *mempool) {
    mempool_ = mempool;
    // ptr_ = mempool_->log_ptr_;
    max_size_ = mempool_->max_log_size_ * THREAD_NUM;
    segment_num_ = THREAD_NUM;
    
    /******************
    *
    * 类型:说明
    * 内容:mask_ 暂留，未使用
    * 添加者:bwb
    * 时间:2023-2-26
    *
    ******************/
    mask_ = max_size_ - 1;

    for (uint64_t i = 0; i < segment_num_; i++) {
        log_segments_[i] = new LogSegment(mempool, i);
    }
}

Log::~Log() {
}

void LogSegment::set_item(uint64_t item_offset, uint64_t key_hash,
                          const uint8_t *key, uint32_t key_length,
                          const uint8_t *value,uint32_t value_length,
                          uint32_t expire_time) {
  assert(key_length <= ITEMKEY_MASK);
  assert(value_length <= ITEMVALUE_MASK);
  LogItem *item = locateItem(item_offset);
  item->kv_length_vec = ITEMKV_LENGTH_VEC(key_length, value_length);
  item->key_hash = key_hash;
  item->expire_time = expire_time;
  memcpy8(item->data, key, key_length);
  memcpy8(item->data + ROUNDUP8(key_length), value, value_length);
}

bool LogSegment::is_valid(uint64_t log_offset) {
    return ((tail_ - log_offset + seg_offset_mask_ + 1) & seg_offset_mask_) <= size_; 
}

//new locateItem()

/******************
*
* 类型:已解决的问题
* 内容1:下面&的seg_offset_mask_明显有问题，32bit的指针容积，却&了46bit的offset。已改成了32bit的log_segment_size_
* 内容2:这里的指针定位应该先加上offset，后转换类型。否则先转换，后加减，很容易导致越界
* 添加者:bwb
* 时间1:2023-03-11
* 时间2:2023-03-16
*
******************/

LogItem *LogSegment::locateItem(uint64_t log_offset) {
    log_offset = log_offset & (kSegOffset - 1);
    return reinterpret_cast<LogItem*>(segment_ptr_ + log_offset % seg_log_size_);
    // return reinterpret_cast<LogItem*>(segment_ptr_ + (log_offset & (seg_log_size_ - 1)));

}

/* LogItem *LogSegment::locateItem(uint64_t log_offset, uint64_t item_size) {
    log_offset = log_offset % seg_log_size_;
    uint64_t new_size = log_offset + item_size;
    if (__glibc_unlikely(new_size > seg_log_size_)) {
            if (__glibc_unlikely(new_size <= mempool_->max_log_size_)) {
                seg_log_size_ = new_size;
            } else {
                seg_log_size_ = log_offset;
                return reinterpret_cast<LogItem*>(segment_ptr_);
            }
        }
    return reinterpret_cast<LogItem*>(segment_ptr_ + log_offset);
} */



/* int64_t LogSegment::AllocItem(uint64_t item_size) {
    // uint64_t item_size = mem_size;
    //TODO: assert(item_size == ROUNDUP8(item_size));

    int64_t item_offset;
    if (item_size <= BATCH_SIZE) {

        if (log_blocks_[usingblock_]->residue < item_size) {
            // block in use is already filled up 
            // check if there is free block left
            offset_ = 0;
            if (usingblock_ < blocknum_ - 1) {
                // use next block
                usingblock_++;
                if (round_) {
                    store_stats_->actual_used_mem -= mempool_->get_block_size() - log_blocks_[usingblock_]->residue;
                    log_blocks_[usingblock_]->residue = mempool_->get_block_size();
                }   
            } else {
                usingblock_ = 0;
                round_++;
                store_stats_->actual_used_mem -= mempool_->get_block_size() - log_blocks_[0]->residue;
                log_blocks_[0]->residue = mempool_->get_block_size();

                printf("round:%d\n",round_);
                // return -1;
            }
        }
        item_offset = offset_;
        offset_ += item_size;
        
        log_blocks_[usingblock_]->residue -= item_size;
        avg_item_size = 
            (avg_item_size * table_stats_->set_success + item_size) * 1.0 /
            (table_stats_->set_success + 1);
        return item_offset;
    } else {
        // TODO: implement a function `big_set`
        return -2;  // batch_too_small
    }
} */

uint64_t LogSegment::AllocItem(uint64_t item_size) {//实际上执行的是MICA的push_tail()功能

    // uint64_t item_size = mem_size;
    assert(item_size == ROUNDUP8(item_size));

    if (__glibc_likely(item_size <= mempool_->max_log_size_)) {
        uint64_t offset = tail_;
        uint64_t v = offset + item_size;
        
        /******************
        *
        * 类型:隐患
        * 内容:offset实现wrap:若空间不足直接从真头部开始。随之而来的问题
        *      问题1：pop_head()如何跳过碎片？ 已解决!
        *      问题2：tail在前，head在后，如何实现log_size的比较？   已解决!
        *      问题3：该问题完善后如何适配lru的情况？  暂时也沿用AllocItem，直接copy if通过的部分
        *      问题4：line 300越界？以及类似的分线程问题？
        * 添加者:bwb
        * 时间1:2023-02-28
        * 时间2:2023-03-16
        *
        ******************/
       //下面6行对应offset回环的情况，暂时按照v的增长固定 +40 来进行
        if (__glibc_unlikely(v >= dynamic_end_)) {
            if (__glibc_unlikely(v < (segment_idx_ + 1) * kSegOffset)) {
                dynamic_end_ = v;
            } else {
                dynamic_end_ = tail_;
                offset = segment_idx_ * kSegOffset;
            }
        }
        if (v < head_) v += dynamic_end_ & seg_offset_mask_;//考虑到当前48bit并未达到变量的最大容纳空间64bit，因此可以这么加
        while (v > head_ + seg_log_size_) this->pop_head();//确保head至tail的范围不超过log_size
    
        LogItem *new_item = this->locateItem(offset);//弃用 分配版locateItem
        new_item->item_size = item_size;
        tail_ = offset + item_size;
        size_ = v - head_;
        // size_ = tail_ < head_ ? (dynamic_end_ + tail_ - head_) : (tail_ - head_);
        return offset;
    } else {
        return 0xffffffffffffffffUL;  // item_too_big
    }
}

void LogSegment::pop_head() {
    LogItem *item = this->locateItem(head_);
    if (dynamic_end_ == head_) head_ = segment_idx_ * kSegOffset;
    head_ = (head_ + item->item_size) & seg_offset_mask_;
    
}

uint64_t LogSegment::AllocItem_lru(uint64_t item_size) {
    uint64_t offset = tail_;
    uint64_t v = offset + item_size;

    if (__glibc_unlikely(v >= dynamic_end_)) {
        if (__glibc_unlikely(v < (segment_idx_ + 1) * kSegOffset)) {
            dynamic_end_ = v;
        } else {
            dynamic_end_ = tail_;
            offset = segment_idx_ * kSegOffset;
        }
    }
    if (v < head_) v += dynamic_end_ & seg_offset_mask_;
    while (v > head_ + seg_log_size_) this->pop_head();

    LogItem *new_item = this->locateItem(offset);//弃用 分配版locateItem
    new_item->item_size = item_size;
    tail_ = offset + item_size;
    size_ = v - head_;
    return offset;
}

void LogSegment::print_table_stats() {
  printf("valid count:            %10zu\n\n", table_stats_->count);
  printf("set request:            %10zu\n", table_stats_->set_count);
  printf("set_success:            %10zu | ", table_stats_->set_success);
  printf("set_fail:               %10zu\n", table_stats_->set_fail);
//   printf("set_nooverwrite:        %10zu | ", table_stats_->set_nooverwrite);
  printf("set_random:             %10zu | ", table_stats_->set_inplace);
  printf("set_cuckoo_evicted:     %10zu | ", table_stats_->set_cuckoo_evicted);
  printf("set_expire_evicted:     %10zu\n\n", table_stats_->set_expire_evicted);
  printf("get request:            %10zu\n", table_stats_->get_count);
  printf("get_found:              %10zu | ", table_stats_->get_found);
  printf("get_notfound:           %10zu | ", table_stats_->get_notfound);
  printf("get_hit_ratio:          %5lf%%\n\n", table_stats_->get_found * 100.0 / (table_stats_->get_found + table_stats_->get_notfound));
  printf("test_found:             %10zu | ", table_stats_->test_found);
  printf("test_notfound:          %10zu\n", table_stats_->test_notfound);
  printf("move_to_head_performed: %10zu | ", table_stats_->move_to_head_performed);
  printf("move_to_head_skipped:   %10zu | ", table_stats_->move_to_head_skipped);
  printf("move_to_head_failed:    %10zu\n", table_stats_->move_to_head_failed);
}