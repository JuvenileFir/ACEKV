#include <assert.h>
#include "cuckoo.h"
const uint64_t bucket_num = 16777215;
static const uint8_t MAX_BFS_PATH_LEN = 4;

// static const uint16_t const_pow_slot_per_bucket_MAX_BFS_PATH_LEN =
//     (uint16_t)16807;

/* static uint32_t const_pow(uint32_t a, uint32_t b) {
    return (b == 0) ? 1 : a * const_pow(a, b - 1);
} */

static uint32_t empty(bQueue que) { return que.first_ == que.last_; }

static uint32_t full(bQueue que) { return que.last_ == MAX_CUCKOO_COUNT; }

static void enqueue(bQueue *que, bSlot x) {
  assert(!full(*que));
  que->slots_[que->last_++] = x;//并非循环队列，但是预先算好了max_size
}

static bSlot dequeue(bQueue *que) {
  assert(!empty(*que));
  assert(que->first_ < que->last_);
  bSlot x = que->slots_[que->first_++];
  return x;
}

/* twoBucket cal_two_buckets(uint64_t keyhash) {
  struct twoBucket tb;
  tb.b1 = keyhash & BUCKETS_PER_PARTITION;
  tb.b2 = alt_bucket(tb.b1, calc_tag(keyhash));
  tb.b2 += (tb.b1 == tb.b2);
  assert(tb.b1 != tb.b2);

  if (tb.b1 > tb.b2) swap_uint(&tb.b1, &tb.b2);
  assert(tb.b2 <= BUCKETS_PER_PARTITION);
  return tb;
} */

//new cal_two_buckets()
// twoBucket *cal_two_buckets(uint64_t keyhash) {
twoBucket cal_two_buckets(uint64_t keyhash) {
  twoBucket tb;
  // uint32_t b0 = keyhash & bucket_num;
  tb.b1 = keyhash >> 40;
  // tb.b1 = keyhash & bucket_num;
  // if (b0 != tb.b1) printf("hash:%zu\t右移:%u\t取模:%u\n",keyhash, tb.b1, b0);
  tb.b2 = alt_bucket(tb.b1, calc_tag(keyhash));
  tb.b2 += (tb.b1 == tb.b2);
  assert(tb.b1 != tb.b2);

  if (tb.b1 > tb.b2) swap_uint(&tb.b1, &tb.b2);
  assert(tb.b2 <= bucket_num);
  return tb;
}

/* uint32_t alt_bucket(uint32_t b1, uint16_t tag) {
  uint32_t b2 = (b1 ^ (tag * 0xc6a4a7935bd1e995)) & BUCKETS_PER_PARTITION;
  assert(b2 != b1);
  return b2 != b1 ? b2 : (b2 + 1) & BUCKETS_PER_PARTITION;
} */
uint32_t alt_bucket(uint32_t b1, uint16_t tag) {
  uint32_t b2 = (b1 ^ (static_cast<uint32_t>(tag * 0xc6a4a7935bd1e995))) & bucket_num;
  // uint32_t b2 = (b1 ^ (static_cast<uint32_t>(tag * 0xc6a4a7935bd1e995))) >> 8;//下一行会报错
  assert(b2 != b1);
  return b2 != b1 ? b2 : (b2 + 1) & bucket_num;
}

void swap_uint(uint32_t *i1, uint32_t *i2) {
  assert(i1 != i2);
  uint32_t tmp = *i1;
  *i1 = *i2;
  *i2 = tmp;
}

struct twoSnapshot read_two_buckets_begin(Bucket *ptr, twoBucket tb) {
  struct twoSnapshot ts;

  ts.v1 = read_version_begin(&ptr[tb.b1]);
  ts.v2 = read_version_begin(&ptr[tb.b2]);
  return ts;
}

struct twoSnapshot read_two_buckets_end(Bucket *ptr, twoBucket tb) {
  struct twoSnapshot ts;
  ts.v1 = read_version_end(&ptr[tb.b1]);
  ts.v2 = read_version_end(&ptr[tb.b2]);
  return ts;
}

uint32_t is_snapshots_same(twoSnapshot ts1, twoSnapshot ts2) { return (ts1.v1 == ts2.v1) && (ts1.v2 == ts2.v2); }

void lock_two_buckets(Bucket *ptr, twoBucket twobuckets) {
  // sort_two_bucket(&twobuckets);
  // assert(twobuckets.b1 < twobuckets.b2);
  if (twobuckets.b1 > twobuckets.b2) swap_uint(&twobuckets.b1, &twobuckets.b2);
  write_lock_bucket(&ptr[twobuckets.b1]);
  if (twobuckets.b1 != twobuckets.b2) write_lock_bucket(&ptr[twobuckets.b2]);
}

void unlock_two_buckets(Bucket *ptr, twoBucket twobuckets) {
  // sort_two_bucket(&twobuckets);
  write_unlock_bucket(&ptr[twobuckets.b1]);
  write_unlock_bucket(&ptr[twobuckets.b2]);
}

void lock_three_buckets(Bucket *ptr, uint32_t b1, uint32_t b2, uint32_t extrab) {
  assert(b1 < b2);
  if (extrab < b2) swap_uint(&extrab, &b2);
  if (b2 < b1) swap_uint(&b1, &b2);

  write_lock_bucket(&ptr[b1]);
  if (b1 != b2) write_lock_bucket(&ptr[b2]);
  if (b2 != extrab) write_lock_bucket(&ptr[extrab]);
}
//什么作用？
tablePosition cuckoo_find_shallow(Bucket *ptr, twoBucket tb, uint64_t offset, uint16_t tag) {
  struct tablePosition tpos = {tb.b1, 0, ok};
  tpos.slot = try_find_slot(&ptr[tb.b1], tag, offset);
  if (tpos.slot != ITEMS_PER_BUCKET) {
    return tpos;
  }
  tpos.slot = try_find_slot(&ptr[tb.b2], tag, offset);
  if (tpos.slot != ITEMS_PER_BUCKET) {
    tpos.bucket = tb.b2;
    return tpos;
  }
  tpos.cuckoostatus = failure_key_not_found;
  return tpos;
}

/*
 * run_cuckoo performs cuckoo hashing on the table in an attempt to free up
 * a slot on either of the insert buckets, which are assumed to be locked
 * before the start. On success, the bucket and slot that was freed up is
 * stored in insert_bucket and insert_slot. In order to perform the search
 * and the swaps, it has to release the locks, which can lead to certain
 * concurrency issues, the details of which are explained in the function.
 * If run_cuckoo returns ok (success), then `tb` will be active, otherwise it
 * will not.
 */
cuckooStatus run_cuckoo(Bucket *ptr, twoBucket tb, uint32_t *insertbucket,
                        uint32_t *insertslot) {
  /*
   * We must unlock the buckets here, so that cuckoopath_search and
   * cuckoopath_move can lock buckets as desired without deadlock.
   * cuckoopath_move has to move something out of one of the original
   * buckets as its last operation, and it will lock both buckets and
   * leave them locked after finishing. This way, we know that if
   * cuckoopath_move succeeds, then the buckets needed for insertion are
   * still locked. If cuckoopath_move fails, the buckets are unlocked and
   * we try again. This unlocking does present two problems. The first is
   * that another insert on the same key runs and, finding that the key
   * isn't in the table, inserts the key into the table. Then we insert
   * the key into the table, causing a duplication. To check for this, we
   * search the buckets for the key we are trying to insert before doing
   * so (this is done in cuckoo_insert, and requires that both buckets are
   * locked).
   */
  unlock_two_buckets(ptr, tb);
  struct cuckooRecord cuckoopath[MAX_BFS_PATH_LEN];

  uint32_t done = false;
  while (!done) {
    int depth = cuckoopath_search(ptr, cuckoopath, tb.b1, tb.b2);//实现了BFS路径寻找及记录，最长路径是5，超过5直接返回-1
    if (depth < 0) break;

    if (__glibc_likely(cuckoopath_move(ptr, cuckoopath, depth, &tb))) {//正常情况下这个if就是直接过了
      *insertbucket = cuckoopath[0].bucket;
      *insertslot = cuckoopath[0].slot;
      assert(*insertbucket == tb.b1 || *insertbucket == tb.b2);
      assert(is_slot_empty(ptr[*insertbucket].item_vec[*insertslot]));
      done = true;
      break;
    }
  }
  return done ? ok : failure;
}
//new "run_cuckoo"
cuckooStatus run_cuckoo(Bucket *ptr, tablePosition* tp) {

  struct cuckooRecord cuckoopath[MAX_BFS_PATH_LEN];
  //用tag算出另一个bucket,以此启动
  uint32_t new_bucket = alt_bucket(tp->bucket, TAG((ptr[tp->bucket]).item_vec[tp->slot]));
  
  uint32_t done = false;

  while (!done) {

    int depth = cuckoopath_search(ptr, cuckoopath, new_bucket);//实现了BFS路径寻找及记录，最长路径是5，超过5直接返回-1

    // int depth = cuckoopath_search(ptr, cuckoopath, tb.b1, tb.b2);//实现了BFS路径寻找及记录，最长路径是5，超过5直接返回-1
    if (depth < 0) break;

    if (__glibc_likely(cuckoopath_move(ptr, cuckoopath, depth, new_bucket))) {//正常情况下这个if就是直接过了

      // assert(is_empty(ptr[*insertbucket].item_vec[*insertslot]));
      //cuckoopath[0].bucket就是new_bucket
      assert(is_slot_empty(ptr[new_bucket].item_vec[cuckoopath[0].slot]));
      ptr[new_bucket].item_vec[cuckoopath[0].slot] = ptr[tp->bucket].item_vec[tp->slot];
      ptr[tp->bucket].item_vec[tp->slot] = 0;
      done = true;
      break;
    }
  }

  // if (!done) write_unlock_bucket(&ptr[tp->bucket]);
  return done ? ok : failure;
}
//old "cuckoopath_search"
int cuckoopath_search(Bucket *ptr, cuckooRecord *cuckoopath, const uint32_t b1, const uint32_t b2) {
  bSlot x = slot_search(ptr, b1, b2);
  if (x.depth == -1) {
    return -1;
  }
  /* Fill in the cuckoo path slots from the end to the beginning. */
  //倒序填充cuckoopath[]的slot
  for (int i = x.depth; i >= 0; i--) {
    cuckoopath[i].slot = x.pathcode % ITEMS_PER_BUCKET;
    x.pathcode /= ITEMS_PER_BUCKET;
  }
  // Fill in the cuckoo_path buckets and keys from the beginning to the
  // end, using the final pathcode to figure out which bucket the path
  // starts on. Since data could have been modified between slot_search
  // and the computation of the cuckoo path, this could be an invalid
  // cuckoo_path.
  //正序填充cuckoopath[]的bucket（还需要借助前一个的tag）
  cuckooRecord *first = &cuckoopath[0];
  if (x.pathcode == 0) {
    first->bucket = b1;
  } else {
    assert(x.pathcode == 1);
    first->bucket = b2;
  }
  {
    Bucket *b = &ptr[first->bucket];
    write_lock_bucket(b);
    //if代码块作用：如果b1,b2中有空位（说明后来变了），直接输出（感觉可以去掉这一块，因为经过细化实际上不会有这种清空）
    if (is_slot_empty(b->item_vec[first->slot])) {
      // We can terminate here
      write_unlock_bucket(b);
      return 0;
    }
    first->tag = TAG(b->item_vec[first->slot]);
    write_unlock_bucket(b);
  }
  for (int i = 1; i <= x.depth; ++i) {
    cuckooRecord *curr = &cuckoopath[i];
    const struct cuckooRecord *prev = &cuckoopath[i - 1];

    // We get the bucket that this slot is on by computing the alternate
    // index of the previous bucket
    curr->bucket = alt_bucket(prev->bucket, prev->tag);
    Bucket *b = &ptr[curr->bucket];
    write_lock_bucket(b);
    //if代码块作用：如果路上有空位（说明后来变了），直接输出（感觉可以去掉这一块，因为实际上应该不会有这种清空）
    if (is_slot_empty(b->item_vec[curr->slot])) {
      // We can terminate here
      write_unlock_bucket(b);
      return i;
    }
    curr->tag = TAG(b->item_vec[curr->slot]);
    write_unlock_bucket(b);
  }
  return x.depth;
}
//new "cuckoopath_search"
int cuckoopath_search(Bucket *ptr, cuckooRecord *cuckoopath, const uint32_t new_bucket) {
  bSlot x = slot_search(ptr, new_bucket);
  if (x.depth == -1) return -1;
  /* Fill in the cuckoo path slots from the end to the beginning. */
  //使用记录的pathcode，倒序填充cuckoopath[]的slot
  for (int i = x.depth; i >= 0; i--) {
    cuckoopath[i].slot = x.pathcode % ITEMS_PER_BUCKET;
    x.pathcode /= ITEMS_PER_BUCKET;
  }

  //以下至line 298: 填充cuckoopath[0]的bucket和tag
  cuckooRecord *first = &cuckoopath[0];
  if (x.pathcode == 0) {
    first->bucket = new_bucket;
  } else {
    assert(0);
  }
  {
    Bucket *b = &ptr[first->bucket];
    // write_lock_bucket(b);
    //if代码块作用：如果当前bkt中有空位，直接输出index 0
    if (is_slot_empty(b->item_vec[first->slot])) {
      // write_unlock_bucket(b);
      return 0;
    }
    first->tag = TAG(b->item_vec[first->slot]);//填tag
    // write_unlock_bucket(b);
  }
  //以下至line 307: 从1开始，正序循环填充cuckoopath[i]的bucket
  for (int i = 1; i <= x.depth; ++i) {
    cuckooRecord *curr = &cuckoopath[i];
    const cuckooRecord *prev = &cuckoopath[i - 1];
    curr->bucket = alt_bucket(prev->bucket, prev->tag);//根据前一个bucket和tag填新bucket
    Bucket *b = &ptr[curr->bucket];
    // write_lock_bucket(b);
    //下面的if块作用：如果有空位，直接输出（大概率是既定位置，也可以是路上中途发现空位（几乎不可能））
    if (is_slot_empty(b->item_vec[curr->slot])) {
      // write_unlock_bucket(b);
      return i;
    }
    curr->tag = TAG(b->item_vec[curr->slot]);//填tag
    // write_unlock_bucket(b);
  }
  return x.depth;//正常情况下不走这里，最后一次循环会走line 314直接跳出
  //反过来说如果走到了这里，似乎是有问题的？
}
/*
 * slot_search searches for a cuckoo path using breadth-first search. It
 * starts with the b1 and b2 buckets, and, until it finds a bucket with an
 * empty slot, adds each slot of the bucket in the bSlot. If the queue runs
 * out of space, it fails.
 */
bSlot slot_search(Bucket *ptr, const uint32_t b1, const uint32_t b2) {
  bQueue que;
  que.first_ = 0;
  que.last_ = 0;
  // The initial pathcode informs cuckoopath_search which bucket the path starts on
  enqueue(&que, (bSlot){b1, 0, 0});
  enqueue(&que, (bSlot){b2, 1, 0});
  while (!empty(que)) {
    bSlot x = dequeue(&que);
    Bucket *b = &ptr[x.bucket];
    write_lock_bucket(b);
    // Picks a (sort-of) random slot to start from
    uint32_t startingslot = x.pathcode % ITEMS_PER_BUCKET;
    for (uint32_t i = 0; i < ITEMS_PER_BUCKET; ++i) {
      uint32_t slot = (startingslot + i) % ITEMS_PER_BUCKET;
      if (is_slot_empty(b->item_vec[slot])) {
        // We can terminate the search here
        x.pathcode = x.pathcode * ITEMS_PER_BUCKET + slot;
        write_unlock_bucket(b);
        return x;
      }
      // If x has less than the maximum number of path components, create a new b_slot item,
      // that represents the bucket we would have come from if we kicked out the item at this slot.
      if (x.depth < MAX_BFS_PATH_LEN - 1) {
        assert(!full(que));
        uint32_t alt_bucket_index = alt_bucket(x.bucket, TAG(b->item_vec[slot]));
        if (alt_bucket_index != b1 && alt_bucket_index != b2) {
          bSlot y = {alt_bucket_index,
                     (uint16_t)(x.pathcode * ITEMS_PER_BUCKET + slot), (int8_t)(x.depth + 1)};
          enqueue(&que, y);
        }
      }
    }
    write_unlock_bucket(b);
  }
  // We didn't find a short-enough cuckoo path, so the search terminated. Return a failure value.
  return (bSlot){0, 0, -1};
}

//new "slot_search"
bSlot slot_search(Bucket *ptr, const uint32_t new_bucket) {
  bQueue que;
  que.first_ = 0;
  que.last_ = 0;
  uint64_t min = 256, min_i = 0, min_slot;
  // The initial pathcode informs cuckoopath_search which bucket the path starts on

  enqueue(&que, (bSlot){new_bucket, 0, 0});
  

  // while (!empty(que)) {
  //   bSlot x = dequeue(&que);
  //   Bucket *b = &ptr[x.bucket];
  //   // write_lock_bucket(b);
  //   // pathcode用于从“随机”slot启动，而非总是0->6
  //   uint32_t startingslot = x.pathcode % ITEMS_PER_BUCKET;

  //   for (uint32_t i = 0; i < ITEMS_PER_BUCKET; ++i) {

  //     uint32_t slot = (startingslot + i) % ITEMS_PER_BUCKET;
  //     //BFS过程中发现空位，直接返回，成功退出
  //     if (is_slot_empty(b->item_vec[slot])) {
  //       x.pathcode = x.pathcode * ITEMS_PER_BUCKET + slot;
  //       // write_unlock_bucket(b);
        
  //       return x;
  //     }

  //     // 只要没达到最大深度，就继续向深层遍历，创建新bSlot入队
  //     if (x.depth < MAX_BFS_PATH_LEN - 1) {

  //       assert(!full(que));
  //       uint32_t alt_bucket_index = alt_bucket(x.bucket, TAG(b->item_vec[slot]));
  //       if (alt_bucket_index != new_bucket) {
  //         bSlot y = {alt_bucket_index, (uint16_t)(x.pathcode * ITEMS_PER_BUCKET + slot),
  //                    (int8_t)(x.depth + 1)};
  //         enqueue(&que, y);
  //       }
  //     }

  //   }
  //   // write_unlock_bucket(b);
  // }
  //路径长度为5之内如果找不到空位，就输出depth -1
  while (!empty(que)) {
    bSlot x = dequeue(&que);
    Bucket *b = &ptr[x.bucket];
    min = 256;
    min_i = 0;
    min_slot = ITEMS_PER_BUCKET;
    // write_lock_bucket(b);
    // pathcode用于从“随机”slot启动，而非总是0->6
    uint32_t startingslot = x.pathcode % ITEMS_PER_BUCKET;

    for (uint32_t i = 0; i < ITEMS_PER_BUCKET; ++i) {

      uint32_t slot = (startingslot + i) % ITEMS_PER_BUCKET;
      //BFS过程中发现空位，直接返回，成功退出
      if (is_slot_empty(b->item_vec[slot])) {
        x.pathcode = x.pathcode * ITEMS_PER_BUCKET + slot;
        // write_unlock_bucket(b);
        
        return x;
      } else if (COUNTER(b->item_vec[slot]) < min) {
        min = COUNTER(b->item_vec[slot]);
        min_i = i;
      }
    }
      // 只要没达到最大深度，就继续向深层遍历，创建新bSlot入队
      if (x.depth < MAX_BFS_PATH_LEN - 1) {
        min_slot = (startingslot + min_i) % ITEMS_PER_BUCKET;
        assert(!full(que));
        uint32_t alt_bucket_index = alt_bucket(x.bucket, TAG(b->item_vec[min_slot]));
        if (alt_bucket_index != new_bucket) {
          bSlot y = {alt_bucket_index, (uint16_t)(x.pathcode * ITEMS_PER_BUCKET + min_slot),
                     (int8_t)(x.depth + 1)};
          enqueue(&que, y);
        }
      }
    // write_unlock_bucket(b);
  }

  return (bSlot){0, 0, -1};
}

//old "cuckoopath_move"
/*
 * cuckoopath_move moves keys along the given cuckoo path in order to make
 * an empty slot in one of the buckets in cuckoo_insert. Before the start of
 * this function, the two insert-locked buckets were unlocked in run_cuckoo.
 * At the end of the function, if the function returns true (success), then
 * both insert-locked buckets remain locked. If the function is
 * unsuccessful, then both insert-locked buckets will be unlocked.
 */
uint32_t cuckoopath_move(Bucket *ptr, cuckooRecord *cuckoopath, int depth, twoBucket *tb) {
  if (depth == 0) {//路径如果是0长度，那就不用移动了，这里只作检查
    // There is a chance that depth == 0, when try_add_to_bucket sees
    // both buckets as full and cuckoopath_search finds one empty. In
    // this case, we lock both buckets. If the slot that
    // cuckoopath_search found empty isn't empty anymore, we unlock them
    // and return false. Otherwise, the bucket is empty and insertable,
    // so we hold the locks and return true.
    const uint32_t bucket_i = cuckoopath[0].bucket;
    assert(bucket_i == tb->b1 || bucket_i == tb->b2);
    lock_two_buckets(ptr, *tb);
    if (is_slot_empty(ptr[bucket_i].item_vec[cuckoopath[0].slot])) {
      return true;//不解锁？应该有道理，因为接下来还要做一遍验证才能给你解锁。
    } else {
      unlock_two_buckets(ptr, *tb);//检查失败的之后肯定不用了，所以当场解锁
      return false;//你这depth有问题啊？回去重算
    }
    return true;
  }

  while (depth > 0) {
    cuckooRecord *from = &cuckoopath[depth - 1];//倒序进行迁移
    cuckooRecord *to = &cuckoopath[depth];
    uint32_t fs = from->slot;
    uint32_t ts = to->slot;
    twoBucket twob;
    uint32_t extra_bucket = bucket_num + 1;//特殊数字，解锁时当flag用
    if (depth == 1) {
      // Even though we are only swapping out of one of the original
      // buckets, we have to lock both of them along with the slot we
      // are swapping to, since at the end of this function, they both
      // must be locked. We store tb inside the extrab container so it
      // is unlocked at the end of the loop.
      //上面意思是因为移动完输出的时候要保证是原来的俩tb是上锁状态，所以最后一次移动的
      //时候要锁3个，然后只解锁一个，以实现最后输出的时候只有两个上锁的效果。为了实现这个效果，花了大量的篇幅
      if (to->bucket != tb->b1 && to->bucket != tb->b2) {//如果depth是1，大概率出现的情况就是two bucket和目的bkt三者相异
        extra_bucket = to->bucket;
        twob = *tb;
        lock_three_buckets(ptr, tb->b1, tb->b2, to->bucket);//所以，tb的俩bkt和空白的目的地bkt全锁
      } else {
        twob = (twoBucket){from->bucket, to->bucket};//否则，只锁俩（这种情况应该就不会出现，要不然就直接depth=0了）
        lock_two_buckets(ptr, twob);
      }
    } else {
      twob = (twoBucket){from->bucket, to->bucket};//depth大于1，直接锁倒数两个就行
      lock_two_buckets(ptr, twob);
    }

    Bucket *from_b = &ptr[from->bucket];
    Bucket *to_b = &ptr[to->bucket];

    /*
     * We plan to kick out fs, but let's check if it is still there;
     * there's a small chance we've gotten scooped by a later cuckoo. If
     * that happened, just... try again. Also the slot we are filling in
     * may have already been filled in by another thread, or the slot we
     * are moving from may be empty, both of which invalidate the swap.
     * We only need to check that the hash value is the same, because,
     * even if the keys are different and have the same hash value, then
     * the cuckoopath is still valid.
     */
    // ToThink: Why should it returns false when the slot we are moving
    // from is empty. Why not just step forward to next move?
    //bwb：上面的意思是说既然倒二是空，何不将错就错，直接用倒二的空？但我觉得还是打回重新确认一下路径为好
    if (!is_slot_empty(to_b->item_vec[ts]) || is_slot_empty(from_b->item_vec[fs]) ||
        (TAG(from_b->item_vec[fs]) != from->tag)) {//出错：目的bkt非空，或倒二bkt空，或倒二tag与记录冲突，打回重新找路！
      if (extra_bucket != bucket_num + 1) write_unlock_bucket(&ptr[extra_bucket]);//如果当初锁了3个，把额外的解锁了
      unlock_two_buckets(ptr, twob);
      return false;
    }

    to_b->item_vec[ts] = from_b->item_vec[fs];//核心操作：item_vector迁移
#ifdef IMPORT_LOG
    printf("[CUK] from(%u, %u) to(%u, %u) entry: %lu\n", from->bucket, from->slot, to->bucket, to->slot,
           from_b->item_vec[fs]);
#endif
    from_b->item_vec[fs] = 0UL;//核心操作：倒二改成0，准备下一轮的迁移

    if (extra_bucket != bucket_num + 1) write_unlock_bucket(&ptr[extra_bucket]);//如果当初锁了3个，把额外的解锁了

    if (depth != 1) {
      // Release the locks contained in twob
      unlock_two_buckets(ptr, twob);//正常情况下就解锁俩就好。depth是1的话，待会就要return true出去了，所以暂时不解锁。
    }
    depth--;
  }
  return true;
}

//new "cuckoopath_move"
uint32_t cuckoopath_move(Bucket *ptr, cuckooRecord *cuckoopath, int depth, const uint32_t new_bucket) {

  if (depth == 0) {//路径如果是0长度，那就不用移动了，这里只作检查
    assert(cuckoopath[0].bucket == new_bucket);
    if (is_slot_empty(ptr[cuckoopath[0].bucket].item_vec[cuckoopath[0].slot])) {
      return true;
    } else {
      return false;//depth有问题,回去重算
    }
    // return true;
  }

  while (depth > 0) {
    cuckooRecord *from = &cuckoopath[depth - 1];//倒序进行迁移
    cuckooRecord *to = &cuckoopath[depth];
    uint32_t fs = from->slot;
    uint32_t ts = to->slot;
    twoBucket twob;

    twob = (twoBucket){from->bucket, to->bucket};
    // lock_two_buckets(ptr, twob);

    Bucket *from_b = &ptr[from->bucket];
    Bucket *to_b = &ptr[to->bucket];
    //出错：目的bkt非空，或倒二bkt空，或倒二tag与记录冲突,回去重算
    if (!is_slot_empty(to_b->item_vec[ts]) ||
        is_slot_empty(from_b->item_vec[fs]) ||
        (TAG(from_b->item_vec[fs]) != from->tag)) {
      // unlock_two_buckets(ptr, twob);
      return false;
    }

    to_b->item_vec[ts] = from_b->item_vec[fs];//核心操作：item_vector迁移
    from_b->item_vec[fs] = 0UL;//核心操作：倒二改成0，准备下一轮的迁移

    // unlock_two_buckets(ptr, twob);
    depth--;
  }
  return true;
}