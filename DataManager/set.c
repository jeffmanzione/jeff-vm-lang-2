/*
 * set.c
 *
 *  Created on: Jul 6, 2016
 *      Author: Jeff
 */

#ifndef NEW_MAP

#include "set.h"

#include <string.h>

#include "error.h"
#include "memory.h"
#include "shared.h"

void set_iterate_buckets(const Set *set, Action /*(Queue*)*/action);

Set *set_create(uint32_t size, Hasher hasher, Comparator comparator) {
  Set *set = ALLOC2(Set);
  set_init(set, size, hasher, comparator);
  return set;
}

Set *set_create_default() {
  return set_create(DEFAULT_TABLE_SZ, default_hasher, default_comparator);
}

void set_init(Set *set, uint32_t size, Hasher hasher, Comparator comparator) {
  set->hasher = hasher;
  set->comparator = comparator;
  set->table_size = size;
  set->size = 0;

  set->queue = ALLOC_ARRAY(Queue, set->table_size);
}

void set_finalize(Set *set) {
  ASSERT_NOT_NULL(set);
  void free_it(void *q) {
    queue_shallow_delete((Queue *) q);
  }
  set_iterate_buckets(set, free_it);
  set->size = 0;
  DEALLOC(set->queue);
  set->table_size = 0;
}

void set_delete(Set *set) {
  ASSERT_NOT_NULL(set);
  set_finalize(set);
  DEALLOC(set);
}

bool set_insert(Set *set, const void *ptr) {
  ASSERT_NOT_NULL(set);
  if (set_lookup(set, ptr)) {
    return false;
  }
  uint32_t hash = set->hasher(ptr);

  int index = hash % set->table_size;
  if (0 == set->queue[index].size) {
    queue_init(&set->queue[index]);
  }
  queue_add_front(&set->queue[index], ptr);
  set->size++;
  return true;
}

bool set_remove(Set *set, const void *ptr) {
  ASSERT_NOT_NULL(set);
  if (!set_lookup(set, ptr)) {
    return false;
  }

  uint32_t hash = set->hasher(ptr);
  int index = hash % set->table_size;

  ASSERT(0 != set->queue[index].size);
  S_ASSERT(queue_remove_elt(&set->queue[index], ptr));
  return true;
}

void *set_lookup(const Set *set, const void *ptr) {
  ASSERT_NOT_NULL(set);
  Queue *queue = &set->queue[set->hasher(ptr) % set->table_size];
  if (NULL == queue) {
    return NULL;
  }
  void *result = NULL;

  bool check_elt(const void *elt) {
    if (!set->comparator(ptr, elt)) {
      result = (void *) elt;
      return true;
    }
    return false;
  }
  queue_iterate_until(queue, check_elt);
  return result;
}

int set_size(const Set *set) {
  return set->size;
}

void set_iterate(const Set *set, Action action) {
  ASSERT_NOT_NULL(set);
  void element_wise(void *queue) {
    ASSERT_NOT_NULL(queue);
    queue_iterate_mutable((Queue *) queue, action);
  }
  set_iterate_buckets(set, element_wise);
}

void set_iterate_buckets(const Set *set, Action /*(Queue*)*/action) {
  ASSERT_NOT_NULL(set);
  int i;
  for (i = 0; i < set->table_size; i++) {
    if (0 != set->queue[i].size) {
      action(&set->queue[i]);
    }
  }
}

#endif
