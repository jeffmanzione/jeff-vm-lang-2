/*
 * set2.c
 *
 *  Created on: Feb 5, 2018
 *      Author: Jeff
 */

#ifdef NEW_MAP

#include <stdbool.h>
#include <stdint.h>

#include "map.h"

#include "../error.h"
#include "../memory/memory.h"
#include "../shared.h"

#ifdef DEBUG
int MAP__count = 0;
int MAP__insert_count = 0;
int MAP__insert_compares_count = 0;
int MAP__lookup_count = 0;
int MAP__lookup_compares_count = 0;
#endif

#define pos(hval, num_probes, table_sz) \
  (((hval) + ((num_probes) * (num_probes))) % (table_sz))
#define calculate_new_size(current_sz) (((current_sz)*2) + 1)
#define calculate_thresh(table_sz) ((int)(((float)(3 * (table_sz))) / 4))

typedef void (*EntryAction)(MEntry *me);

void resize_table(Map *map);

Map *map_create(uint32_t size, Hasher hasher, Comparator comparator) {
  Map *map = ALLOC2(Map);
  map_init(map, size, hasher, comparator);
  return map;
}

void map_init(Map *map, uint32_t size, Hasher hasher, Comparator comparator) {
#ifdef DEBUG
  MAP__count++;
#endif
  map->hash = hasher;
  map->compare = comparator;
  map->table_sz = size;
  map->entries_thresh = calculate_thresh(size);
  //  DEBUGF("map_init 1");
  map->table = ALLOC_ARRAY(MEntry, size);
  //  DEBUGF("map_init 2");
  map->first = NULL;
  map->last = NULL;
  map->num_entries = 0;
}

void map_init_default(Map *map) {
  map_init(map, DEFAULT_MAP_SZ, default_hasher, default_comparator);
}

Map *map_create_default() {
  return map_create(DEFAULT_MAP_SZ, default_hasher, default_comparator);
}

void map_finalize(Map *map) { DEALLOC(map->table); }

void map_delete(Map *map) {
  ASSERT_NOT_NULL(map);
  map_finalize(map);
  DEALLOC(map);
}

bool map_insert_helper(Map *map, const void *key, const void *value,
                       uint32_t hval, MEntry *table, uint32_t table_sz,
                       MEntry **first, MEntry **last) {
  ASSERT(NOT_NULL(map));
  int num_probes = 0;
  MEntry *first_empty = NULL;
  int num_probes_at_first_empty = -1;
  while (true) {
    int table_index = pos(hval, num_probes, table_sz);
    num_probes++;
    MEntry *me = table + table_index;
    // Position is vacant.
    if (0 == me->num_probes) {
      // Use the previously empty slot if we don't find our element.
      if (first_empty != NULL) {
        me = first_empty;
        num_probes = num_probes_at_first_empty;
      }
      // Take the vacant spot.
      me->pair.key = key;
      me->pair.value = (void *)value;
      me->hash_value = hval;
      me->num_probes = num_probes;
      me->prev = *last;
      me->next = NULL;
      if (NULL != *last) {
        me->prev->next = me;
      }
      *last = me;
      if (NULL == *first) {
        *first = me;
      }
      return true;
    }
    // Spot is vacant but previously used, mark it so we can use it later.
    if (-1 == me->num_probes) {
      if (NULL == first_empty) {
        first_empty = me;
        num_probes_at_first_empty = num_probes;
      }
      continue;
    }
    // Pair is already present in the table, so the mission is accomplished.
    if (hval == me->hash_value) {
#ifdef DEBUG
      MAP__insert_compares_count++;
#endif
      if (0 == map->compare(key, me->pair.key)) {
        return false;
      }
    }
    // Rob this entry if it did fewer probes.
    if (me->num_probes < num_probes) {
      //      DEBUGF("ROBBING!");
      MEntry tmp_me = *me;
      // Take its spot.
      me->pair.key = key;
      me->pair.value = (void *)value;
      me->hash_value = hval;
      me->num_probes = num_probes;
      // It is the new insertion.
      key = tmp_me.pair.key;
      value = tmp_me.pair.value;
      hval = tmp_me.hash_value;
      num_probes = tmp_me.num_probes;
      first_empty = NULL;
      num_probes_at_first_empty = -1;
    }
  }
}

bool map_insert(Map *map, const void *key, const void *value) {
#ifdef DEBUG
  MAP__insert_count++;
#endif
  ASSERT(NOT_NULL(map));
  //  DEBUGF("num_entries=%d entries_thresh=%d, table_sz=%d", map->num_entries,
  //      map->entries_thresh, map->table_sz);
  if (map->num_entries > map->entries_thresh) {
    resize_table(map);
  }
  bool was_inserted =
      map_insert_helper(map, key, value, map->hash(key), map->table,
                        map->table_sz, &map->first, &map->last);
  if (was_inserted) {
    map->num_entries++;
  }
  return was_inserted;
}

MEntry *map_lookup_entry(const Map *map, const void *key, MEntry *table,
                         uint32_t table_sz) {
  ASSERT(NOT_NULL(map));
  uint32_t hval = map->hash(key);
  int num_probes = 0;
  while (true) {
    int table_index = pos(hval, num_probes, table_sz);
    ++num_probes;
    MEntry *me = table + table_index;
    if (0 == me->num_probes) {
      // Found empty.
      return NULL;
    }
    if (-1 == me->num_probes) {
      // Found previously used.
      continue;
    }
    if (hval == me->hash_value) {
#ifdef DEBUG
      MAP__lookup_compares_count++;
#endif
      if (0 == map->compare(key, me->pair.key)) {
        return me;
      }
    }
  }
}

Pair map_remove(Map *map, const void *key) {
  ASSERT(NOT_NULL(map));
  MEntry *me = map_lookup_entry(map, key, map->table, map->table_sz);
  if (NULL == me) {
    Pair pair = {key, NULL};
    return pair;
  }
  if (map->last == me) {
    map->last = me->prev;
  } else {
    me->next->prev = me->prev;
  }
  if (map->first == me) {
    map->first = me->next;
  } else {
    me->prev->next = me->next;
  }
  me->num_probes = -1;
  map->num_entries--;
  return me->pair;
}

DEB_FN(void *, map_lookup, const Map *map, const void *key) {
#ifdef DEBUG
  MAP__lookup_count++;
#endif
  ASSERT(NOT_NULL(map));
  MEntry *me = map_lookup_entry(map, key, map->table, map->table_sz);
  if (NULL == me) {
    return NULL;
  }
  return me->pair.value;
}

void map_iterate_internal(const Map *map, EntryAction action) {
  MEntry *me;
  for (me = map->first; NULL != me; me = me->next) {
    action(me);
  }
}

void map_iterate(const Map *map, PairAction action) {
  ASSERT(NOT_NULL(map));
  void pair_action(MEntry * me) { action(&me->pair); }
  map_iterate_internal(map, pair_action);
}

uint32_t map_size(const Map *map) { return map->num_entries; }

void resize_table(Map *map) {
  ASSERT(NOT_NULL(map));
  //  DEBUGF("resize_table num_entries=%d entries_thresh=%d, table_sz=%d",
  //      map->num_entries, map->entries_thresh, map->table_sz);
  int new_table_sz = calculate_new_size(map->table_sz);
  MEntry *new_table = ALLOC_ARRAY(MEntry, new_table_sz);
  MEntry *new_first = NULL;
  MEntry *new_last = NULL;

  void insert_in_new_table(MEntry * me) {
    map_insert_helper(map, me->pair.key, me->pair.value, me->hash_value,
                      new_table, new_table_sz, &new_first, &new_last);
  }
  map_iterate_internal(map, insert_in_new_table);

  DEALLOC(map->table);
  map->table = new_table;
  map->table_sz = new_table_sz;
  map->first = new_first;
  map->last = new_last;
  map->entries_thresh = calculate_thresh(new_table_sz);
}

#endif
