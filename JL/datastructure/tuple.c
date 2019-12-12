/*
 * tuple.c
 *
 *  Created on: Jan 1, 2017
 *      Author: Dad
 */

#include "tuple.h"

#include <stddef.h>

#include "../error.h"
#include "../memory/memory.h"

#define DEFAULT_TUPLE_SZ 8

struct Tuple_ {
  size_t size, table_size;
  Element *table;
};

Tuple *tuple_create() {
  Tuple *tuple = ALLOC(Tuple);
  tuple->size = 0;
  tuple->table = NULL;
  return tuple;
}

void tuple_add(Tuple *t, Element e) {
  ASSERT_NOT_NULL(t);
  if (0 == t->size) {
    t->table_size = DEFAULT_TUPLE_SZ;
    t->table = ALLOC_ARRAY2(Element, t->table_size);
  } else if (t->size == t->table_size) {
    t->table_size += DEFAULT_TUPLE_SZ;
    t->table = REALLOC(t->table, Element, t->table_size);
  }
  t->table[t->size++] = e;
}

Element tuple_get(const Tuple *t, uint32_t index) {
  ASSERT(NOT_NULL(t), index < t->size);
  return t->table[index];
}

uint32_t tuple_size(const Tuple *t) {
  ASSERT_NOT_NULL(t);
  return t->size;
}

void tuple_delete(Tuple *t) {
  if (t->size > 0) {
    DEALLOC(t->table);
  }
  DEALLOC(t);
}

void tuple_print(const Tuple *t, FILE *file) {
  ASSERT(NOT_NULL(t), NOT_NULL(file));
  fprintf(file, "(");
  if (t->size > 0) {
    elt_to_str(tuple_get(t, 0), file);
    int i;
    for (i = 1; i < t->size; i++) {
      fprintf(file, ", ");
      elt_to_str(tuple_get(t, i), file);
    }
  }
  fprintf(file, ")");
}
