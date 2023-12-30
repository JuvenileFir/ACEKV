#pragma once

#ifndef PIEKV_H_
#define PIEKV_H_

#include <thread>
#include <vector>
#include <csignal>

// #include "directory/directory_client.h"
#include "basic_hash.h"

const size_t kblock_size = 2097152UL;
extern bool allow_mutation;
// typedef client::directory::DirectoryClient DirectoryClient;
typedef struct AllTableStats {
    size_t count;
    size_t rx_burst;
    size_t rx_packet;
    size_t set_request;
    size_t set_success;
    size_t set_evict_succ;
    size_t set_fail;
    size_t set_cuckoo_evicted;
    size_t get_request;
    size_t get_found;
    size_t get_notfound;
    size_t delete_found;
    size_t delete_notfound;
}AllTableStats;

class Piekv
{
private:


public:
    // HashTable *hashtable_;
    // uint32_t stop_entry_gc_;//used for stopping index entry gc when `move_h2t`???
    // MemPool *mempool_;
    // Log *log_;
    double max_tput_;
    AllTableStats *all_table_stats_;
    uint64_t latency[4];

    // DirectoryClient* dir_client_;
    
    uint32_t is_running_;
    uint8_t thread_is_running_[THREAD_NUM];
    
    Piekv(size_t init_table_size, size_t init_log_size);
    ~Piekv();
    
    void print_trigger(size_t maxbytes);
    // void directory_proc();

    bool get(size_t t_id, uint64_t key_hash, const uint8_t *key, size_t key_length, uint8_t *out_value, uint32_t *in_out_value_length);
    bool set(size_t t_id, uint64_t key_hash, uint8_t* key, uint32_t key_len, uint8_t* val, uint32_t val_len, bool overwrite);

    // bool set_check(uint64_t key_hash, const uint8_t *key, size_t key_length);  // TODO: implement , change name

    // void showUtilization();
    // void cleanUpHashTable();
    // void countPreciseAKV(double *averageKVSizes);
};


#endif