/*
 * ts_set.h
 *
 *  Created on: Jan 6, 2019
 *      Author: Jeff
 */

#ifndef DATASTRUCTURE_TS_SET_H_
#define DATASTRUCTURE_TS_SET_H_

#include <stdbool.h>
#include <stdint.h>

#include "../shared.h"
#include "../threads/thread_interface.h"
#include "set.h"

typedef struct TsSet_ {
  Set *set_ptr;
  Set set;
  bool is_wrapper;
//  RWLock lock;
  ThreadHandle lock;
} TsSet;

TsSet *ts_set_create(uint32_t table_size, Hasher, Comparator);
TsSet *ts_set_create_default();
void ts_set_init(TsSet *set, uint32_t table_size, Hasher, Comparator);
void ts_set_init_default(TsSet *set);
void ts_set_delete(TsSet *);
void ts_set_finalize(TsSet *);
bool ts_set_insert(TsSet *, const void *);
bool ts_set_remove(TsSet *, const void *);
void *ts_set_lookup(const TsSet *, const void *);
int ts_set_size(const TsSet *);
void ts_set_iterate(const TsSet *, Action);

#endif /* DATASTRUCTURE_TS_SET_H_ */
