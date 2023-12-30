#pragma once

#include <stdint.h>

#include "mempool.h"
#include "util.h"

#define bkt_size 4

#define TAG_MASK (((uint64_t)1 << 8) - 1)
#define TAG(item_vec) ((item_vec) >> 56)
#define SEG_MASK (0x03UL)
#define SEG(item_vec) ((item_vec >> 56) & SEG_MASK)
#define COUNTER_MASK (0xffUL)
#define COUNTER(item_vec) ((item_vec >> 48) & COUNTER_MASK)
#define ITEM_OFFSET_MASK (((uint64_t)1 << 48) - 1)
#define ITEM_OFFSET(item_vec) (item_vec & ITEM_OFFSET_MASK)
#define ITEM_SEG_OFFSET_MASK (((uint64_t)1 << 46) - 1)
#define ITEM_SEG_OFFSET(item_vec) (item_vec & ITEM_SEG_OFFSET_MASK)

#define SET_PAGE(entry, pageNumber) (((uint64_t)entry) | ((uint64_t)pageNumber) << 27)

#define ITEM_VEC(tag, counter, item_offset) \
  (((uint64_t)(tag) << 56) | ((uint64_t)(counter) << 48) | (uint64_t)(item_offset))

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
  enum cuckooStatus cuckoostatus;
} tablePosition;


uint16_t calc_tag(uint64_t key_hash);

uint32_t calc_segment_id(uint16_t tag);

Cbool is_slot_empty(uint64_t index_entry);

Cbool key_eq(const uint8_t *key1, size_t key1_len, const uint8_t *key2, size_t key2_len);

Cbool val_eq(const uint8_t *val1, size_t val1_len, const uint8_t *val2, size_t val2_len);


// Cbool try_find_insert_bucket(Bucket *bucket_, uint32_t *slot, const uint16_t tag, const uint8_t *key,
//                              uint32_t keylength, uint64_t *rounds);