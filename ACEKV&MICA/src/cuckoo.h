#pragma once

#ifndef CUCKOO_H
#define CUCKOO_H

#include <math.h>
#include "basic_hash.h"
// #include "log.h"

#define MAX_CUCKOO_COUNT 5602U

typedef struct twoBucket {
  uint32_t b1;
  uint32_t b2;
} twoBucket;

typedef struct twoSnapshot {
  uint32_t v1;
  uint32_t v2;
} twoSnapshot;

typedef struct cuckooRecord {
  uint32_t bucket;
  uint32_t slot;
  uint16_t tag;
} cuckooRecord;

typedef struct bSlot {
  uint32_t bucket;
  uint16_t pathcode;
  int8_t depth;
} bSlot;

// bQueue is the queue used to store bSlots for BFS cuckoo hashing.


typedef struct bQueue {
  static_assert(slot_num > 0, "SLOT_PER_BUCKET must be greater than 0.");
  bSlot slots_[MAX_CUCKOO_COUNT];//5602=2 ×（7^0 + 7^1 + 7^2 + 7^3 + 7^4）
  uint32_t first_;
  uint32_t last_;
} bQueue;

// struct Bucket;
// struct tablePosition;

// struct twoBucket cal_two_buckets(uint64_t keyhash);
// twoBucket *cal_two_buckets(uint64_t keyhash);
twoBucket cal_two_buckets(uint64_t keyhash);

//uint32_t alt_bucket(uint32_t b1, uint16_t tag);
uint32_t alt_bucket(uint32_t b1, uint16_t tag);

void swap_uint(uint32_t *i1, uint32_t *i2);

twoSnapshot read_two_buckets_begin(Bucket *ptr, twoBucket tb);

twoSnapshot read_two_buckets_end(Bucket *ptr, twoBucket tb);

uint32_t is_snapshots_same(twoSnapshot ts1, twoSnapshot ts2);

void lock_two_buckets(Bucket *ptr, twoBucket twobuckets);

void unlock_two_buckets(Bucket *ptr, twoBucket twobuckets);

void lock_three_buckets(Bucket *ptr, uint32_t b1, uint32_t b2, uint32_t extrab);

tablePosition cuckoo_find_shallow(Bucket *ptr, twoBucket tb, uint64_t offset, uint16_t tag);

// tablePosition cuckoo_insert(Bucket *partition, uint64_t keyhash, uint16_t tag, struct twoBucket tb,
//                             const uint8_t *key, size_t keylength);
// new cuckoo_insert()
// tablePosition cuckoo_insert(Bucket *ptr, uint64_t keyhash, uint16_t tag,
//                             twoBucket tb, const uint8_t *key, 
//                             size_t keylength);
// 简单RW版
// tablePosition cuckoo_insert(Bucket *ptr, uint64_t keyhash, uint16_t tag, struct twoBucket tb,
//                             const uint8_t *key, size_t keylength, uint64_t *rounds);

// cuckooStatus run_cuckoo(Bucket *ptr, twoBucket tb, uint32_t *insertbucket,
                        // uint32_t *insertslot);
//new "run_cuckoo"
// cuckooStatus run_cuckoo(Bucket *ptr, tablePosition* tp, int *cuckoo_depth, int *break_loop, LogSegment *seg);
//old "cuckoopath_search"
// int cuckoopath_search(Bucket *ptr, cuckooRecord *cuckoopath, const uint32_t b1, const uint32_t b2); 
//new "cuckoopath_search"
// int cuckoopath_search(Bucket *ptr, cuckooRecord *cuckoopath, const uint32_t new_bucket, bool random_pick, LogSegment *seg); 
//old "slot_search"
// bSlot slot_search(Bucket *ptr, const uint32_t b1, const uint32_t b2);
//new "slot_search"
// bSlot slot_search(Bucket *ptr, const uint32_t new_bucket, bool random_pick, LogSegment *seg);

Cbool cuckoopath_move(Bucket *ptr, cuckooRecord *cuckoopath, int depth, twoBucket *tb);

Cbool cuckoopath_move(Bucket *ptr, cuckooRecord *cuckoopath, int depth, const uint32_t new_bucket);

Cbool is_slot_empty(uint64_t index_entry);

inline Cbool is_slot_empty(uint64_t index_entry) { return !index_entry; }
#endif