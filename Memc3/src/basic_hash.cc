#include "basic_hash.h"

extern MemPool *kMemPool;

uint16_t calc_tag(uint64_t key_hash) {
  uint16_t tag = (uint16_t)(key_hash & TAG_MASK);
  // uint16_t tag = (uint16_t)((key_hash >> 32) & TAG_MASK);
  // uint16_t tag = (uint16_t)(key_hash >> 56) ;
  if (tag == 0)
    return 1;
  else
    return tag;
}

Cbool key_eq(const uint8_t *key1, size_t key1_len, const uint8_t *key2, size_t key2_len) {
  return key1_len == key2_len && memcmp8(key1, key2, key1_len);
}

Cbool val_eq(const uint8_t *val1, size_t val1_len, const uint8_t *val2, size_t val2_len) {
  return val1_len == val2_len && memcmp8(val1, val2, val1_len);
}



