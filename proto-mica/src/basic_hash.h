#pragma once

#include <stdint.h>

#include "mempool.h"
#include "util.h"

// #define RUN_MICA
 
// #define MEMC3_EVICT

#define USE_COUNTER

// #define COST_COUNTER

// #define USE_HIRAND

#define CLEANUP //固定不变

#define RELOAD  //固定不变

// #define RELOAD_RECORD //严格测cuckoo path时要打开；测性能要关闭

// #define LATENCY  //测LATENCY

//*****以下只有在测试不同kvsize时切换使用
#define size8
// #define size64
// #define size256
// #define size512

// #define MICA_15 //弃用

const double kTrigger = 70.0;  // 0为全程开启判断，70为切换式，100为关闭高段

#ifdef RUN_MICA
  #define TAG_MASK (((uint64_t)1 << 16) - 1)
  #define TAG(item_vec) ((item_vec) >> 48)
  #define SEG_MASK (0x03UL)
  #define SEG(item_vec) ((item_vec >> 48) & SEG_MASK)
  #define COUNTER_MASK (0xffUL)
  #define COUNTER(item_vec) ((item_vec >> 48) & COUNTER_MASK)
  #define ITEM_VEC(tag, item_offset) \
  (((uint64_t)(tag) << 48) | (uint64_t)(item_offset))
#else
  #define TAG_MASK (((uint64_t)1 << 8) - 1)
  #define TAG(item_vec) ((item_vec) >> 56)
  #define SEG_MASK (0x03UL)
  #define SEG(item_vec) ((item_vec >> 56) & SEG_MASK)
  #define COUNTER_MASK (0xffffUL)
  #define COUNTER(item_vec) ((item_vec >> 40) & COUNTER_MASK)

  #define ITEM_VEC(tag, counter, item_offsets) \
  (((uint64_t)(tag) << 56) | ((uint64_t)(counter) << 40) | (uint64_t)(item_offsets))
#endif

#define ITEM_OFFSET_MASK (((uint64_t)1 << 40) - 1)
#define ITEM_OFFSET(item_vec) (item_vec & ITEM_OFFSET_MASK)
#define ITEM_SEG_OFFSET_MASK (((uint64_t)1 << 38) - 1)
#define ITEM_SEG_OFFSET(item_vec) (item_vec & ITEM_SEG_OFFSET_MASK)

typedef enum cuckooStatus {
  ok,
  inplace,
  failure,
  failure_key_not_found,
  failure_key_duplicated,
  failure_table_full,
  cuckoo_evicted,
  expire_evicted,
  rand_overwrite,
} cuckooStatus;

typedef enum bucketStatus {
  slot_empty,
  slot_expire,
  key_duplicated,
  full_random,
  full_min_count,
} bucketStatus;

typedef enum ITEM_RESULT {
  ITEM_OK = 0,
  ITEM_ERROR,
  ITEM_FULL,
  ITEM_EXIST,
  ITEM_NOT_FOUND,
  ITEM_PARTIAL_VALUE,
  ITEM_NOT_PROCESSED,
} ITEM_RESULT;

typedef struct tablePosition {
  uint32_t bucket;
  uint32_t slot;
// #ifndef RUN_MICA
  enum cuckooStatus cuckoostatus;
// #endif
} tablePosition;

#ifdef RUN_MICA
  //一个bucket实际存储slot数量 7、15、31？
  #ifdef MICA_15
    const int slot_num = 15;
  #else
    const int slot_num = 7;
  #endif
#else
  const int slot_num = 7;//未定义mica,即proto的情况
#endif

typedef struct Bucket {
    uint32_t version;
    uint32_t padding;
    uint64_t item_vec[slot_num];
} Bucket;     //  ALIGNED(64)

const uint64_t kBucketSize = sizeof(Bucket);
const uint64_t kIndexSpace = 1ULL << 30;
const uint64_t kBucketNum  = 16777216UL;
const uint64_t kSlotNum  = kBucketNum * slot_num;
// #ifndef RUN_MICA
const double kFactor = 0.8;
const double kFactor2 = 0;
const size_t kIndexThreshold  = static_cast<size_t>(kSlotNum * kFactor);
const size_t kIndexThreshold2  = static_cast<size_t>(kSlotNum * kFactor2);
// #endif

uint16_t calc_tag(uint64_t key_hash);

uint32_t calc_segment_id(uint16_t tag);

uint32_t read_version_begin(const Bucket *bucket UNUSED);

uint32_t read_version_end(const Bucket *bucket UNUSED);

inline void write_lock_bucket(const Bucket *bucket UNUSED) {
  while (1) {
    uint32_t v = *(volatile uint32_t *)&bucket->version & ~1U;
    uint32_t new_v = v | 1U;
    if (__sync_bool_compare_and_swap((volatile uint32_t *)&bucket->version, v, new_v))
      break;
  }
}

// inline void write_lock_bucket(const Bucket *bucket /* UNUSED */, size_t t_id) {
//   memory_barrier();
//   while (1) {
//     uint32_t v = *(volatile uint32_t *)&bucket->version & ~1U;
//     uint32_t new_v = v | 1U;
//     if (__sync_bool_compare_and_swap((volatile uint32_t *)&bucket->version, v, new_v)) {
//       break;
//     } else {
//       printf("tid:%zu version:%u  v:%u new_v:%u\n", t_id,bucket->version, v, new_v);
//     }
//   }
// }

inline void write_unlock_bucket(const Bucket *bucket) {
  memory_barrier();
  assert((*(volatile uint32_t *)&bucket->version & 1U) == 1U);
  // // No need to use atomic add because this thread is the only one writing to version
  __sync_fetch_and_add((volatile uint32_t *)&bucket->version, 1U);
  // (*(volatile uint32_t*)&bucket->version)++;
}

Cbool is_slot_empty(uint64_t index_entry);

Cbool key_eq(const uint8_t *key1, size_t key1_len, const uint8_t *key2, size_t key2_len);

Cbool val_eq(const uint8_t *val1, size_t val1_len, const uint8_t *val2, size_t val2_len);

uint16_t try_find_slot(const Bucket *bucket, const uint16_t tag, const uint64_t offset);

// Cbool try_find_insert_bucket(Bucket *bucket_, uint32_t *slot, const uint16_t tag, const uint8_t *key,
//                              uint32_t keylength, uint64_t *rounds);