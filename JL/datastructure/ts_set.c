/*
 * ts_set.c
 *
 *  Created on: Jan 6, 2019
 *      Author: Jeff
 */

#include "ts_set.h"

#include "../memory/memory.h"

TsSet *ts_set_create(uint32_t table_size, Hasher hasher, Comparator comparator) {
  TsSet *set = ALLOC(TsSet);
  ts_set_init(set, table_size, hasher, comparator);
  return set;
}

TsSet *ts_set_create_default() {
  TsSet *set = ALLOC(TsSet);
  ts_set_init(set, DEFAULT_TABLE_SZ, default_hasher, default_comparator);
  return set;
}

void ts_set_init(TsSet *set, uint32_t table_size, Hasher hasher,
    Comparator comparator) {
  set->is_wrapper = false;
  set_init(&set->set, table_size, hasher, comparator);
  set->set_ptr = &set->set;
//  set->lock = create_rwlock();
  set->lock = mutex_create(NULL);
}

void ts_set_init_default(TsSet *set) {
  ts_set_init(set, DEFAULT_TABLE_SZ, default_hasher, default_comparator);
}

void ts_set_wrap(TsSet *tsset, Set *set) {
  tsset->set_ptr = set;
  tsset->is_wrapper = true;
  //  tsset->lock = create_rwlock();
  tsset->lock = mutex_create(NULL);
}

void ts_set_delete(TsSet *set) {
  ts_set_finalize(set);
  DEALLOC(set);
}

void ts_set_finalize(TsSet *set) {
  if (!set->is_wrapper) {
    set_finalize(&set->set);
  }
//  close_rwlock(&set->lock);
  mutex_close(set->lock);
}

bool ts_set_insert(TsSet *set, const void *v) {
//  begin_write(&set->lock);
  mutex_await(set->lock, INFINITE);
  bool inserted = set_insert(set->set_ptr, v);
//  end_write(&set->lock);
  mutex_release(set->lock);
  return inserted;

}

bool ts_set_remove(TsSet *set, const void *v) {
//  begin_write(&set->lock);
  mutex_await(set->lock, INFINITE);
  bool removed = set_remove(set->set_ptr, v);
//  end_write(&set->lock);
  mutex_release(set->lock);
  return removed;
}

void *ts_set_lookup(const TsSet *set, const void *v) {
//  begin_read((RWLock *) &set->lock);
  mutex_await(set->lock, INFINITE);
  void *val = set_lookup(set->set_ptr, v);
//  end_read((RWLock *) &set->lock);
  mutex_release(set->lock);
  return val;
}

void ts_set_iterate(const TsSet *set, Action action) {
//  begin_read((RWLock *) &set->lock);
  mutex_await(set->lock, INFINITE);
  set_iterate(set->set_ptr, action);
//  end_read((RWLock *) &set->lock);
  mutex_release(set->lock);
}

int ts_set_size(const TsSet *set) {
//  begin_read((RWLock *) &set->lock);
  mutex_await(set->lock, INFINITE);
  return set_size(set->set_ptr);
//  end_read((RWLock *) &set->lock);
  mutex_release(set->lock);
}
