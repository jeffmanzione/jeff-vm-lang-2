/*
 * ltable.c
 *
 *  Created on: Nov 17, 2018
 *      Author: Jeffrey
 */

#include "ltable.h"

#include "../arena/strings.h"
#include "../datastructure/map.h"

#define CKEY_TABLE_SZ 251

char *ckeys[CKey_END];

Map ckey_table_;

void CKey_set(CommonKey key, const char *str) {
  ckeys[key] = (char *)str;
  map_insert(&ckey_table_, str, (void *)(key + 1));
}

void CKey_init() {
  map_init(&ckey_table_, CKEY_TABLE_SZ, default_hasher, default_comparator);
  CKey_set(CKey_$ip, IP_FIELD);
  CKey_set(CKey_$module, MODULE_FIELD);
  CKey_set(CKey_$has_error, ERROR_KEY);
  CKey_set(CKey_len, LENGTH_KEY);
  CKey_set(CKey_class, CLASS_KEY);
  CKey_set(CKey_$resval, RESULT_VAL);
  CKey_set(CKey_$adr, ADDRESS_KEY);
  CKey_set(CKey_module, MODULE_KEY);
  CKey_set(CKey_name, NAME_KEY);
  CKey_set(CKey_$parent, PARENT);
  CKey_set(CKey_parents, PARENTS_KEY);
  CKey_set(CKey_parent_class, PARENT_CLASS);
}

void CKey_finalize() { map_finalize(&ckey_table_); }

CommonKey CKey_lookup_key(const char *str) {
  return (CommonKey)map_lookup(&ckey_table_, str) - 1;
}

const char *CKey_lookup_str(CommonKey key) { return ckeys[key]; }
