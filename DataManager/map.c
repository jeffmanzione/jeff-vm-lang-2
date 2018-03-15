/*
 * map.c
 *
 *  Created on: Jul 7, 2016
 *      Author: Jeff
 */

#ifndef NEW_MAP

#include "map.h"

#include <stdio.h>
#include <stdlib.h>

#include "error.h"
#include "memory.h"

#define MAP_FUNCS(map)  uint32_t key_hasher(const void *ptr) {          \
                          if (NULL == ptr) {return 0;}                  \
                          Pair *pair = (Pair *) ptr;                    \
                          return map->hash(pair->key);                  \
                        }                                               \
                        int32_t key_comparator(const void *a, const void *b) { \
                          if (NULL == a && NULL == b) {return 0;}       \
                          if (NULL == a) {return -1;}                   \
                          if (NULL == b) {return 1;}                    \
                          Pair *pairA = (Pair *) a;                     \
                          Pair *pairB = (Pair *) b;                     \
                          return map->compare(pairA->key, pairB->key);  \
                        }
#define RESET_SET(map)  map->kv_set.hasher = key_hasher; \
                        map->kv_set.comparator = key_comparator

Map *map_create(uint32_t size, Hasher hasher, Comparator comparator) {
  Map *map = ALLOC(Map);
  map_init(map, size, hasher, comparator);
  return map;
}

Map *map_create_default() {
  return map_create(DEFAULT_MAP_SZ, default_hasher, default_comparator);
}

void map_init(Map *map, uint32_t size, Hasher hasher, Comparator comparator) {
  MAP_FUNCS(map);

  map->hash = hasher;
  map->compare = comparator;

  set_init(&map->kv_set, size, key_hasher, key_comparator);
}

void map_finalize(Map *map) {
  ASSERT_NOT_NULL(map);
  void pair_free(Pair *pair) {
    if (pair) {
      DEALLOC(pair);
    }
  }
  map_iterate(map, pair_free);
  set_finalize(&map->kv_set);
}

void map_delete(Map *map) {
  ASSERT_NOT_NULL(map);
  map_finalize(map);
  DEALLOC(map);
}

bool map_insert(Map *map, const void *key, const void *value) {
  ASSERT_NOT_NULL(map);
  Pair tmp = { key, NULL };
  if (map_lookup(map, (void *) &tmp)) {
    return false;
  }
  MAP_FUNCS(map);
  RESET_SET(map);

  Pair *pair = ALLOC(Pair);
  pair->key = key;
  pair->value = (void *) value;

  set_insert(&map->kv_set, pair);
  return true;
}

Pair map_remove(Map *map, const void *key) {
  ASSERT_NOT_NULL(map);
  if (NULL == map_lookup(map, key)) {
    Pair null_pair = { NULL, NULL };
    return null_pair;
  }

  MAP_FUNCS(map);
  RESET_SET(map);

  Pair pair = { key, NULL };
  const Pair *in_set = set_lookup(&map->kv_set, &pair);
  pair = *in_set;
  ASSERT_NOT_NULL(in_set);
  S_ASSERT(set_remove(&map->kv_set, in_set));
  DEALLOC(in_set);
  return pair;
}

void *map_lookup(const Map *map, const void *key) {
  ASSERT_NOT_NULL(map);
  MAP_FUNCS(map);
  RESET_SET(((Map *)map));

  Pair pair = { key, NULL };
  const Pair *in_set = set_lookup(&map->kv_set, &pair);
  if (!in_set) {
    return NULL;
  }
  return in_set->value;
}

void map_iterate(const Map *map, PairAction action) {
  ASSERT_NOT_NULL(map);
  void kv_split(void *ptr) {
    Pair *pair = (Pair *) ptr;
    if (NULL == pair) {
      return;
    }
    action(pair);
  }
  set_iterate(&map->kv_set, kv_split);
}

uint32_t map_size(const Map *map) {
  ASSERT_NOT_NULL(map);
  return set_size(&map->kv_set);
}

#endif
