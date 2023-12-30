#include "basic_hash.h"

extern MemPool *kMemPool;

uint16_t calc_tag(uint64_t key_hash) {
  uint16_t tag = (uint16_t)(key_hash & TAG_MASK);
  if (tag == 0)
    return 1;
  else
    return tag;
}

uint32_t calc_segment_id(uint16_t tag) {
  return tag % THREAD_NUM;
  // return tag & ((1 << 2) - 1);//能用吗？
}

uint32_t read_version_begin(const Bucket *bucket /* UNUSED */) {
  while (true) {
    uint32_t v = *(volatile uint32_t *)(&bucket->version);
    memory_barrier();
    if ((v & 1U) != 0U) continue;
    return v;
  }
}

uint32_t read_version_end(const Bucket *bucket UNUSED) {
  memory_barrier();
  uint32_t v = *(volatile uint32_t *)(&bucket->version);
  return v;
}

/******************
*
* 类型:修改记录
* 内容:暂时给放到.h文件里了
* 添加者:bwb
* 时间:2023-xx-xx
*
******************/
// void write_unlock_bucket(const Bucket *bucket UNUSED) {
//   memory_barrier();
//   assert((*(volatile uint32_t *)&bucket->version & 1U) == 1U);
//   // // No need to use atomic add because this thread is the only one writing to version
//   // (*(volatile uint32_t *)&bucket->version)++;
// 	// assert(__sync_bool_compare_and_swap((volatile uint32_t *)&bucket->version, 0U, 1U));
//   __sync_fetch_and_add((volatile uint32_t *)&bucket->version, 1U);
// }

// inline


Cbool key_eq(const uint8_t *key1, size_t key1_len, const uint8_t *key2, size_t key2_len) {
  return key1_len == key2_len && memcmp8(key1, key2, key1_len);
}

Cbool val_eq(const uint8_t *val1, size_t val1_len, const uint8_t *val2, size_t val2_len) {
  return val1_len == val2_len && memcmp8(val1, val2, val1_len);
}

uint16_t try_find_slot(const Bucket *bucket, const uint16_t tag, const uint64_t offset) {
  uint32_t slot;
  for (slot = 0; slot < slot_num; slot++) {
    if (ITEM_OFFSET(bucket->item_vec[slot]) != offset || TAG(bucket->item_vec[slot]) != tag) continue;
    return slot;
  }
  return slot_num;
}

