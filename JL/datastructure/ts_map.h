/*
 * ts_map.h
 *
 *  Created on: Jan 6, 2019
 *      Author: Jeff
 */

#ifndef DATASTRUCTURE_TS_MAP_H_
#define DATASTRUCTURE_TS_MAP_H_

#include <stdbool.h>
#include <stdint.h>

#include "../shared.h"
#include "../threads/thread_interface.h"
#include "map.h"

typedef struct TsMap_ {
  Map *map_ptr;
  Map map;
  bool is_wrapper;
//  RWLock lock;
  ThreadHandle lock;
} TsMap;

TsMap *ts_map_create(uint32_t size, Hasher, Comparator);
void ts_map_wrap(TsMap *tsmap, Map *map);
void ts_map_init_default(TsMap *map);
TsMap *ts_map_create_default();

void ts_map_finalize(TsMap *);
void ts_map_delete(TsMap *);

bool ts_map_insert(TsMap *, const void *key, const void *value);
Pair ts_map_remove(TsMap *, const void *key);
void *ts_map_lookup(const TsMap *, const void *key);

void ts_map_iterate(const TsMap *, PairAction);

uint32_t ts_map_size(const TsMap *);

#endif /* DATASTRUCTURE_TS_MAP_H_ */
