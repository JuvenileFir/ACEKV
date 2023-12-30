#pragma once
#ifndef HASHTABLE_H_
#define HASHTABLE_H_
#include <cstdlib>

#include "cuckoo.h"
// #include "basic_hash.h"

#define MAX_KEY_LENGTH 8
#define MAX_VALUE_LENGTH 1024

using namespace std;

typedef struct AllTableStats {
    size_t count;
    size_t rx_burst;
    size_t rx_packet;
    size_t set_request;
    size_t set_success;
    size_t set_fail;
    size_t set_evicted;
    size_t set_evicted_invalid;
    size_t expired_evicted;
    size_t get_request;
    size_t get_found;
    size_t get_notfound;
    size_t delete_found;
    size_t delete_notfound;
}AllTableStats;
// ALIGNED(64), use alignas(64) when imply this struct

/* typedef struct TableBlock {
    void *block_ptr;
    uint32_t block_id;
} TableBlock; */

class HashTable {
private:
    /* hash table data */
public:
    void *ptr_;
    // TableBlock *table_blocks_[MAX_BLOCK_NUM];
    uint64_t max_size_;
    uint64_t bucket_num_;
    AllTableStats *all_table_stats_;
    // uint32_t table_block_num_;//combine "hash_table.num_partitions" & "PartitionMap.numOfpartitions"
    // uint32_t current_version_;
    MemPool *mempool_;
    
    HashTable(MemPool* mempool);
    ~HashTable();
    Bucket *get_bucket(uint32_t bucket_index);

    uint16_t try_read_from_bucket(const Bucket *bucket, const uint16_t tag,
                                  const uint8_t *key, uint32_t keylength);
    bucketStatus try_find_insert_bucket(Bucket *bucket_, uint32_t *slot,
                                const uint16_t tag, const uint8_t *key,
                                uint32_t keylength);
    tablePosition cuckoo_find(uint64_t keyhash, twoBucket tb,
                              const uint8_t *key, uint32_t keylength);
    tablePosition get_table(twoSnapshot *ts1, twoBucket *tb, uint64_t key_hash,
                            const uint8_t *key, size_t key_length);
    void unlink(uint8_t *key, uint64_t key_hash);
    double calc_load_factor();
    // void calc_load_factor();
    // int64_t set_table(tablePosition *tp, twoBucket *tb, uint64_t key_hash,
    //                   const uint8_t *key, size_t key_length);
    
    // void showHashTableStatus();

};
#endif