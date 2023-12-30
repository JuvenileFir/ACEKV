#include <cassert>
#include <cstdint> 
#include <cstdio>
#include "hashtable.h"
// #include "basic_hash.h"

HashTable::HashTable(MemPool* mempool) {
	// is_setting_ = 0;
	// is_flexibling_ = 0;
  // is_swapping = 0;
	// current_version_ = 0;
  mempool_ = mempool;
  ptr_ = mempool_->table_ptr_;
  max_size_ = mempool_->max_table_size_;
  bucket_num_ = max_size_ / kBucketSize;
  all_table_stats_ = new AllTableStats();
  memset(all_table_stats_,0,sizeof(AllTableStats));

	// table_block_num_ = mempool->get_block_available_num();
	// assert(table_block_num_);

	// round_hash_ = new RoundHash(table_block_num_, 8);
  // round_hash_new_ = new RoundHash(table_block_num_, 8);

  /* for (int i = 0; i < MAX_BLOCK_NUM; i++) {
        // TODO: use max here for temp, create an init block function later
        table_blocks_[i] = new TableBlock;
    }

	for (uint32_t i = 0; i < table_block_num_; i++) {
		int32_t alloc_id = mempool->alloc_block();
		assert(alloc_id + 1); 
		table_blocks_[i]->block_id = (uint32_t)alloc_id;
		table_blocks_[i]->block_ptr = mempool->get_block_ptr(alloc_id);
		mempool->memset_block(alloc_id);
	} */
}

HashTable::~HashTable() {
  /* ...... */
}

Bucket *HashTable::get_bucket(uint32_t bucket_index) {
	// return &reinterpret_cast<Bucket*>(ptr_)[bucket_index];
	return reinterpret_cast<Bucket*>(ptr_) + bucket_index;
}

uint16_t HashTable::try_read_from_bucket(const Bucket *bucket, const uint16_t tag,
                              const uint8_t *key, uint32_t keylength) {
  uint32_t slot;
  for (slot = 0; slot < slot_num; slot++) {
    if (TAG(bucket->item_vec[slot]) != tag) continue;

    // we may read garbage values, which do not cause any fatal issue
    LogItem *item = mempool_->locate_item(SEG(bucket->item_vec[slot]), ITEM_SEG_OFFSET(bucket->item_vec[slot]));
    // a key comparison reads up to min(source key length and destination key length), which is always safe to do
    if (!key_eq(item->data, ITEMKEY_LENGTH(item->kv_length_vec), key, keylength)) continue;

    return slot;
  }
  return slot_num;
}

tablePosition HashTable::cuckoo_find(uint64_t keyhash, twoBucket tb, 
                                     const uint8_t *key, uint32_t keylength) {
  uint16_t tag = calc_tag(keyhash);

  struct tablePosition tpos = {tb.b1, 0, ok};
  tpos.slot = try_read_from_bucket(get_bucket(tb.b1), tag, key, keylength);
  if (tpos.slot != slot_num) {
    return tpos;
  }
  tpos.slot = try_read_from_bucket(get_bucket(tb.b2), tag, key, keylength);
  if (tpos.slot != slot_num) {
    tpos.bucket = tb.b2;
    return tpos;
  }
  tpos.cuckoostatus = failure_key_not_found;
  return tpos;
}

tablePosition HashTable::get_table(twoSnapshot *ts1, twoBucket *tb,
                                   uint64_t key_hash, const uint8_t *key,
                                   size_t key_length) {
  tablePosition tp;
  *tb = cal_two_buckets(key_hash);
  while (1) {
    *ts1 = read_two_buckets_begin(reinterpret_cast<Bucket*>(ptr_), *tb);
    tp = cuckoo_find(key_hash, *tb, key, key_length);
    if (is_snapshots_same(*ts1, read_two_buckets_end(
        reinterpret_cast<Bucket*>(ptr_), *tb))) break;
  }
  if (tp.cuckoostatus == failure_key_not_found) return tp;
  
  assert(tp.cuckoostatus == ok);

  return tp;
}

double HashTable::calc_load_factor() {
  uint64_t num_entry = 0/* , num_bucket = 0, num_empty = 0 */;
  // bool not_empty = false;
  Bucket *bucket;
  for (uint64_t i = 0; i < bucket_num_; i++) {
    bucket = get_bucket(i);
    for (uint32_t j = 0; j < slot_num; j++) {
      if (!is_slot_empty(bucket->item_vec[j])) {
        num_entry++;
        // not_empty = true;
      }
    }
    /*if (not_empty) {
      // num_bucket++;
      not_empty = false;
    }  else {
      num_empty++;
    } */
    // if (i % 1000000 == 0 && num_empty) {
    //   printf("\nend:%10lu\tnum:%10lu\n", i, num_empty);
    //   num_empty = 0;
    // } 
      
  }
  return num_entry * 100.0 / kSlotNum;
  // printf("Calc Load Factor:       %5lf%%\n", num_entry * 100.0 / kSlotNum);
  // printf("Bucket Load Factor:     %5lf%%\n\n", num_bucket * 100.0 / bucket_num_);
}

void HashTable::unlink(uint8_t *key, uint64_t key_hash) {
  twoSnapshot *ts1 = (twoSnapshot *)malloc(sizeof(twoBucket));
  twoBucket *tb = (twoBucket *)malloc(sizeof(twoBucket));
  tablePosition tp = get_table(ts1, tb, key_hash, key, 8);
  if (tp.cuckoostatus == ok){
      get_bucket(tp.bucket)->item_vec[tp.slot] = 0;
  }
}
