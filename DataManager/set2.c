/*
 * set2.c
 *
 *  Created on: Feb 6, 2018
 *      Author: Jeff
 */

#include <stdbool.h>
#include <stdint.h>

#include "map.h"

/*
 * set.c
 *
 *  Created on: Jul 6, 2016
 *      Author: Jeff
 */

#ifdef NEW_MAP

#include "set.h"

#include <string.h>

#include "error.h"
#include "map.h"
#include "memory.h"
#include "shared.h"

Set *set_create(uint32_t size, Hasher hasher, Comparator comparator) {
  Set *set = ALLOC2(Set);
  set_init(set, size, hasher, comparator);
  return set;
}

Set *set_create_default() {
  return set_create(DEFAULT_TABLE_SZ, default_hasher, default_comparator);
}

void set_init(Set *set, uint32_t size, Hasher hasher, Comparator comparator) {
  map_init(&set->map, size, hasher, comparator);
}

void set_init_default(Set *set) {
  set_init(set, DEFAULT_TABLE_SZ, default_hasher, default_comparator);
}

void set_finalize(Set *set) {
  ASSERT_NOT_NULL(set);
  map_finalize(&set->map);
}

void set_delete(Set *set) {
  ASSERT_NOT_NULL(set);
  set_finalize(set);
  DEALLOC(set);
}

bool set_insert(Set *set, const void *ptr) {
  ASSERT_NOT_NULL(set);
  return map_insert(&set->map, ptr, ptr);
}

bool set_remove(Set *set, const void *ptr) {
  ASSERT_NOT_NULL(set);
  Pair p = map_remove(&set->map, ptr);
  return NULL != p.value;
}

void *set_lookup(const Set *set, const void *ptr) {
  ASSERT_NOT_NULL(set);
  return map_lookup(&set->map, ptr);
}

int set_size(const Set *set) {
  return map_size(&set->map);
}

void set_iterate(const Set *set, Action action) {
  ASSERT_NOT_NULL(set);
  void value_action(Pair *p) {
    action(p->value);
  }
  map_iterate(&set->map, value_action);
}

#endif
