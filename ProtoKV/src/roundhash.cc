//  #include <cassert>
// #include "roundhash.h"

// uint64_t hash_param;

/*inline uint64_t HashValue(uint64_t val, uint64_t mod_pow) {
  uint64_t s0 = val;
  uint64_t s1 = hash_param;

  s1 ^= s0;
  s0 = rotl(s0, 55) ^ s1 ^ (s1 << 14);  // a, b
  s1 = rotl(s1, 36);                    // c
  uint64_t hash1 = s0 + s1;
  hash1 = hash1 & ((1ULL << mod_pow) - 1);
  return hash1;
}

RoundHash::RoundHash(uint32_t num, uint64_t S){
	S_ = S;
	num_long_arcs_ = 1;
	num_short_arc_groups_ = 0;
	num_short_arcs_ = 0;
	current_s_ = S_;
	arc_groups_ = 1;
	S_log_ = 0;

	while ((1UL << S_log_) < S_) 
		S_log_++;//log2(S_)
	assert((1UL << S_log_) == S_);
	for (size_t i = 1; i < num; i++) {
		NewBucket();
	}

	lh_l = 0;
	while ((num >> lh_l) > 2 * S - 1) //2*S-1 为不发生split的group_size(上限)=15
		lh_l++;//得到log2(group数)
	lh_n = num >> lh_l;//Unused,含义:得到小于group上限的block数，应为120>>3=15(120/8=15)
	lh_p = num - (lh_n << lh_l);//Unused,含义:余数120-8×15=0
}

RoundHash::~RoundHash(){
  // ......
}

uint64_t RoundHash::get_block_num(){
	return num_long_arcs_ + num_short_arcs_;
}

size_t RoundHash::ArcNum(uint64_t divs, uint64_t hash){
	uint64_t divs1 = divs >> 32;
  uint64_t divs0 = divs & 0xffffffff;
  uint64_t hash1 = hash >> 32;
  uint64_t hash0 = hash & 0xffffffff;
  uint64_t low = (hash0 * divs0) >> 32;
  size_t new_num = (hash1 * divs1) + ((hash1 * divs0 + hash0 * divs1 + low) >> 32);
  return new_num;
}

size_t RoundHash::HashToArc(uint64_t hash){
	if (get_block_num() < S_) {
    return ArcNum(get_block_num(), hash);
  }
  size_t arc_candidate = ArcNum((current_s_ + 1) * arc_groups_, hash);
  if (arc_candidate < num_short_arcs_) {
    return arc_candidate;
  }
  arc_candidate = ArcNum(current_s_ * arc_groups_, hash) + num_short_arc_groups_;
  return arc_candidate;
}

size_t RoundHash::ArcToBucket(size_t arc_num){
  if (arc_num < S_) {
    return arc_num;
  }
  uint64_t s_to_use = current_s_ + (num_short_arcs_ > arc_num);
  arc_num -= num_short_arc_groups_ & -(num_short_arcs_ <= arc_num);
  size_t arc_group = arc_num / s_to_use;
  size_t position_in_group = arc_num - arc_group * s_to_use;
  size_t initial_groups_ = arc_groups_;
  if (s_to_use > S_) {
    initial_groups_ <<= 1;
    arc_group <<= 1;
    arc_group += (position_in_group >> S_log_);
    position_in_group &= S_ - 1;
  }
  size_t dist = __builtin_ctzll(arc_group);//返回从最低位开始0的个数
  size_t new_ret = ((S_ + position_in_group) * initial_groups_ + arc_group); 
  new_ret = new_ret >> (dist + 1);
  return new_ret;
}

size_t RoundHash::HashToBucket(uint64_t value){
#ifndef ASSUME_HASHED
  value = HashValue(value, 60);
#endif
  const size_t arc = this->HashToArc(value);
  const size_t bucket = this->ArcToBucket(arc);
  return bucket;
}

void RoundHash::NewBucket(){
	assert(num_short_arcs_ == num_short_arc_groups_ * (current_s_ + 1));
  // Simple case: we can change all the hashes
  if (get_block_num() < S_) {//起步用，只用8次
    num_long_arcs_++;
    return;
  }
  num_long_arcs_ -= current_s_;//long_arc减s_，小group数减1
  num_short_arc_groups_++;//大group数加1
  num_short_arcs_ += current_s_ + 1;//short_arc加s_+1,block总数+1

  // If we are done going around the circle ...
  if (num_long_arcs_ == 0) {
    num_long_arcs_ = num_short_arcs_;
    num_short_arc_groups_ = 0;
    num_short_arcs_ = 0;
    current_s_++;
  }
  // If we completed a doubling...
  if (current_s_ == 2 * S_) {
    current_s_ = S_;
    arc_groups_ *= 2;
  }

}

void RoundHash::DelBucket(){
	assert(get_block_num());
  // Simple case: we can change all the hashes
  if (get_block_num() <= S_) {
    num_long_arcs_--;
    return;
  }
  // If we completed a doubling.
  if (current_s_ == S_ && !num_short_arcs_) {
    current_s_ = (S_ << 1);
    arc_groups_ >>= 1;
  }
  // If we are done going around the circle.
  if (num_short_arcs_ == 0) {
    num_short_arcs_ = num_long_arcs_;
    num_short_arc_groups_ = arc_groups_;
    num_long_arcs_ = 0;
    current_s_--;
  }
  num_short_arcs_ -= current_s_ + 1;
  num_short_arc_groups_--;
  num_long_arcs_ += current_s_;
}
 */
/* void RoundHash::get_parts_to_remove(size_t *parts, size_t *count){
	// from the second to the last
	*count = 0;
  size_t max_arc_num = get_block_num() - 1;
  // If finish a round
  if (num_short_arc_groups_ == 0) {
    size_t actual_count = current_s_;
    //若当前group size等于下限S_，则×2后赋值给actual_count(group数为1时,赋值NumBuckets)
    if (current_s_ == S_) 
      actual_count = (S_ << 1) < (max_arc_num + 1) ? (S_ << 1) : max_arc_num + 1;
    // actual_count = actual_count == S_ ? 
    //((S_ << 1) < (max_arc_num + 1) ? (S_ << 1) : max_arc_num + 1) : 
    //actual_count;
    for (; *count < actual_count; (*count)++) {
      parts[*count] = ArcToBucket(max_arc_num - *count);
    }
  } else {
    max_arc_num = num_short_arc_groups_ * (current_s_ + 1) - 1;
    for (; *count <= (current_s_); (*count)++) {
      parts[*count] = ArcToBucket(max_arc_num - *count);
    }
  }
} */

/* void RoundHash::get_parts_to_add(size_t *parts, size_t *count){
	// from the last to the first
  *count = current_s_;
  for (int i = current_s_ - 1; i >= 0; i--) {
    parts[i] = ArcToBucket(num_short_arcs_ + (current_s_ - 1 - i));
  }
} */