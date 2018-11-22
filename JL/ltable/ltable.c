/*
 * ltable.c
 *
 *  Created on: Nov 17, 2018
 *      Author: Jeffrey
 */

#include "ltable.h"

#include "../datastructure/map.h"

Map ckey_table_;
Map str_table_;

void CKey_init() {
  map_init_default(&ckey_table_);
  map_init_default(&str_table_);
}

void CKey_finalize() {
  map_finalize(&ckey_table_);
  map_finalize(&str_table_);
}

void CKey_set(CommonKey key, const char *str) {
  map_insert(&ckey_table_, str, (void *) (key + 1));
  map_insert(&ckey_table_, (void *) (key + 1), str);
}

CommonKey CKey_lookup_key(const char *str) {
  return (CommonKey) map_lookup(&ckey_table_, str) - 1;
}

CommonKey CKey_lookup_str(CommonKey key) {
  return (CommonKey) map_lookup(&str_table_, (void *) (key + 1));
}
