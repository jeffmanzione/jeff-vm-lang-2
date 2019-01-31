/*
 * ts_map.c
 *
 *  Created on: Jan 6, 2019
 *      Author: Jeff
 */

#include "ts_map.h"

#include "../graph/memory.h"

void ts_map_init(TsMap *map, uint32_t size, Hasher hasher,
    Comparator comparator) {
  map->is_wrapper = false;
  map_init(&map->map, size, hasher, comparator);
  map->map_ptr = &map->map;
//  map->lock = create_rwlock();
  map->lock = mutex_create(NULL);
}

TsMap *ts_map_create(uint32_t size, Hasher hasher, Comparator comparator) {
  TsMap *map = ALLOC(TsMap);
  ts_map_init(map, size, hasher, comparator);
  return map;
}

TsMap *ts_map_create_default() {
  TsMap *map = ALLOC(TsMap);
  ts_map_init_default(map);
  return map;
}

void ts_map_init_default(TsMap *map) {
  ts_map_init(map, DEFAULT_MAP_SZ, default_hasher, default_comparator);
}

void ts_map_wrap(TsMap *tsmap, Map *map) {
  tsmap->map_ptr = map;
  tsmap->is_wrapper = true;
//  tsmap->lock = create_rwlock();
  tsmap->lock = mutex_create(NULL);
}

void ts_map_finalize(TsMap *map) {
  if (!map->is_wrapper) {
    map_finalize(&map->map);
  }
//  close_rwlock(&map->lock);
  mutex_close(map->lock);
}

void ts_map_delete(TsMap *map) {
  ts_map_finalize(map);
  DEALLOC(map);
}

bool ts_map_insert(TsMap *map, const void *key, const void *value) {
//  begin_write(&map->lock);
  mutex_await(map->lock, INFINITE);
  bool inserted = map_insert(map->map_ptr, key, value);
//  end_write(&map->lock);
  mutex_release(map->lock);
  return inserted;
}

Pair ts_map_remove(TsMap *map, const void *key) {
//  begin_write(&map->lock);
  mutex_await(map->lock, INFINITE);
  Pair removed = map_remove(map->map_ptr, key);
//  end_write(&map->lock);
  mutex_release(map->lock);
  return removed;
}

void *ts_map_lookup(const TsMap *map, const void *key) {
//  begin_read((RWLock *) &map->lock);
  mutex_await(map->lock, INFINITE);
  void *val = map_lookup(map->map_ptr, key);
//  end_read((RWLock *) &map->lock);
  mutex_release(map->lock);
  return val;
}

void ts_map_iterate(const TsMap *map, PairAction action) {
//  begin_read((RWLock *) &map->lock);
  mutex_await(map->lock, INFINITE);
  map_iterate(map->map_ptr, action);
  end_read((RWLock *) &map->lock);
}

uint32_t ts_map_size(const TsMap *map) {
//  begin_read((RWLock *) &map->lock);
  mutex_await(map->lock, INFINITE);
  return map_size(map->map_ptr);
//  end_read((RWLock *) &map->lock);
  mutex_release(map->lock);
}
