#pragma once

#ifndef LOG_H_
#define LOG_H_

// #include <cstdint>
#include "hashtable.h"

// #define BLOCK_MAX_NUM 16383  // a temp max num, remove it later
// #define BATCH_SIZE (2097152U)

// extern bool pr;

typedef struct TableStats
{
    size_t count;
    size_t rx_pkt_num;
    size_t set_count;
    size_t set_success;
    size_t set_fail;
    // size_t set_nooverwrite;
    size_t set_inplace;
    size_t set_cuckoo_evicted;
    size_t set_expire_evicted;
    size_t get_count;
    size_t get_found;
    size_t get_notfound;
    size_t test_found;
    size_t test_notfound;
    size_t delete_found;
    size_t delete_notfound;
    size_t move_to_head_performed;
    size_t move_to_head_skipped;
    size_t move_to_head_failed;
}TableStats;

typedef struct StoreStats
{
    size_t actual_used_mem;
    size_t wasted;
}StoreStats;

class LogSegment {

private:
    /* data */
public:
    MemPool *mempool_;
    int segment_idx_;
    uint8_t *segment_ptr_;
    uint64_t seg_log_size_;
    uint64_t head_;
    uint64_t tail_;
    uint64_t seg_offset_mask_;
    uint64_t dynamic_end_;
    uint64_t size_;//待完善size的记录
    StoreStats *store_stats_;
    TableStats *table_stats_;

    LogSegment(MemPool *mempool, int index);//new
    ~LogSegment();

    void set_item(uint64_t item_offset, uint64_t key_hash, const uint8_t *key,
                  uint32_t key_length, const uint8_t *value,
                  uint32_t value_length, uint32_t expire_time);
    
    bool is_valid(uint64_t log_offset);
    LogItem *locateItem(uint64_t log_offset);//定位版
    // LogItem *locateItem(uint64_t log_offset, uint64_t item_size);//分配版
    uint64_t AllocItem(uint64_t item_size);
    uint64_t AllocItem_lru(uint64_t item_size);
    void pop_head();
    void print_table_stats();
};

class Log {
private:
    // uint16_t resizing_pointer_;

public:
    MemPool *mempool_;
    LogSegment *log_segments_[THREAD_NUM];
    uint64_t max_size_;
    uint16_t segment_num_;
    uint64_t mask_;
    Log(MemPool *mempool);//new
    ~Log();

};

#endif