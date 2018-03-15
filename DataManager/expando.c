/*
 * expando.c
 *
 *  Created on: Jan 14, 2018
 *      Author: Jeff
 */

#include "expando.h"

#include <string.h>

#include "error.h"
#include "memory.h"

struct Expando_ {
  char *arr;
  size_t len, table_sz, obj_sz;
};

Expando *expando__(void *arr, size_t obj_sz, size_t table_sz) {
  ASSERT(NOT_NULL(arr));
  Expando *e = ALLOC(Expando);
  e->arr = (char *) arr;
  e->len = 0;
  e->obj_sz = obj_sz;
  e->table_sz = table_sz;
  return e;
}

int expando_append(Expando * const e, const void *v) {
  ASSERT(NOT_NULL(e), NOT_NULL(v));
  if (e->len == e->table_sz) {
    e->table_sz += DEFAULT_EXPANDO_SIZE;
    e->arr = REALLOC_SZ(e->arr, e->obj_sz, e->table_sz);
  }
  char * start = e->arr + (e->len * e->obj_sz);
  memmove(start, v, e->obj_sz);
  return e->len++;
}

void expando_delete(Expando * const e) {
  ASSERT(NOT_NULL(e), NOT_NULL(e->arr));
  DEALLOC(e->arr);
  DEALLOC(e);
}

void *expando_get(const Expando * const e, uint32_t i) {
  ASSERT(NOT_NULL(e), NOT_NULL(e->arr), i >= 0, i < e->len);
  return e->arr + (i * e->obj_sz);
}

uint32_t expando_len(const Expando * const e) {
  ASSERT(NOT_NULL(e), NOT_NULL(e->arr));
  return e->len;
}

void expando_sort(Expando * const e, Comparator c, ESwapper eswap) {
  ASSERT(NOT_NULL(e), NOT_NULL(c));
  void swap(int x, int y) {
    eswap(expando_get(e, x), expando_get(e, y));
  }
  int partition(int lo, int hi) {
    void *x = expando_get(e, hi);
    int i = lo - 1, j;
    for (j = 1; j <= hi - 1; j++) {
      if (c(expando_get(e, j), x) <= 0) {
        i++;
        swap(i, j);
      }
    }
    swap(i + 1, hi);
    return i + 1;
  }
  void sort(int lo, int hi) {
    if (c(expando_get(e, lo), expando_get(e, hi)) < 0) {
      int p = partition(lo, hi);
      sort(lo, p - 1);
      sort(p + 1, hi);
    }
  }
  sort(0, expando_len(e) - 1);
}
