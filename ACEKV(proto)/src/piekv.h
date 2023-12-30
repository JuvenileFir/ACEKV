#pragma once

#ifndef PIEKV_H_
#define PIEKV_H_

#include <thread>
#include <vector>
#include <csignal>

// #include "directory/directory_client.h"
#include "log.h"

const size_t kblock_size = 2097152UL;
extern bool allow_mutation;
// typedef client::directory::DirectoryClient DirectoryClient;

class Piekv
{
private:


public:
    HashTable *hashtable_;
    uint32_t stop_entry_gc_;//used for stopping index entry gc when `move_h2t`???
    MemPool *mempool_;
    Log *log_;
    double max_tput_;
    // DirectoryClient* dir_client_;
    
    uint32_t is_running_;
    uint8_t thread_is_running_[THREAD_NUM];
    
    Piekv(size_t init_table_size, size_t init_log_size);
    ~Piekv();
    
    void print_trigger();
    // void directory_proc();

    bucketStatus try_find_insert_bucket(Bucket *bucket_, uint32_t *slot,
                                        const uint16_t tag, const uint8_t *key,
                                        uint32_t keylength);
    tablePosition cuckoo_insert(uint64_t keyhash, uint16_t tag, twoBucket tb,
                                const uint8_t *key, size_t keylength);

    bool get(size_t t_id, uint64_t key_hash, const uint8_t *key, size_t key_length, uint8_t *out_value, uint32_t *in_out_value_length);
    bool set(size_t t_id, uint64_t key_hash, uint8_t* key, uint32_t key_len, uint8_t* val, uint32_t val_len, bool overwrite);

    // bool set_check(uint64_t key_hash, const uint8_t *key, size_t key_length);  // TODO: implement , change name

    // void showUtilization();
    void move_to_head(const Bucket* ptr, tablePosition* tp,
                      const LogItem* item, size_t key_length,
                      size_t value_length, uint64_t item_vec,
                      uint64_t item_offset, size_t t_id);
    // void cleanUpHashTable();
    // void countPreciseAKV(double *averageKVSizes);
};


#endif