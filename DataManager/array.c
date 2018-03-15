/*
 * array.c
 *
 *  Created on: Sep 28, 2016
 *      Author: Jeff
 */

#include "array.h"

#include <stdbool.h>
#include <string.h>

#include "element.h"
#include "error.h"
#include "memory.h"

struct Array_ {
  uint32_t table_size;
  uint32_t num_elts;
  uint32_t next_index;
  Element *table;
};

Array *array_create() {
  Array *array = ALLOC2(Array);
  array->table = ALLOC_ARRAY2(Element, array->table_size = DEFAULT_TABLE_SIZE);
  array->num_elts = 0;
  array->next_index = 0;
  return array;
}

void array_delete(Array* array) {
  ASSERT(NOT_NULL(array));
  DEALLOC(array->table);
  DEALLOC(array);
}

void array_maybe_realloc(Array * const array, uint32_t need_to_accomodate) {
  ASSERT(NOT_NULL(array));
  uint32_t size = (need_to_accomodate / DEFAULT_TABLE_SIZE + 1)
      * DEFAULT_TABLE_SIZE;
  if (size == array->table_size) {
    return;
  }
  array->table = REALLOC(array->table, Element, size);
  array->table_size = size;
}

void array_shift(Array * const array, uint32_t start_pos, int32_t amount) {
  ASSERT(NOT_NULL(array), start_pos >= 0, start_pos + amount >= 0);
  int new_next_index = array->next_index + amount;
  array_maybe_realloc(array, new_next_index);
  memmove(array->table + start_pos + amount, array->table + start_pos,
      (array->num_elts - start_pos) * sizeof(Element));
  if (amount > 0) {
    memset(array->table + start_pos, 0x0, amount * sizeof(Element));
  }
  array->next_index = new_next_index;
}

void array_clear(Array* const array) {
  ASSERT(NOT_NULL(array));
  array_maybe_realloc(array, array->num_elts = array->next_index = 0);
}

void array_push(Array* const array, Element elt) {
  ASSERT(NOT_NULL(array));
  if (array->num_elts > 0) {
    array_shift(array, /*start_pos=*/0, /*amount=*/1);
  }
  array->table[0] = elt;
  array->num_elts++;
}

Element array_pop(Array* const array) {
  ASSERT(array->num_elts != 0);
  Element to_return = array->table[0];
  if (array->num_elts > 1) {
    array_shift(array, /*start_pos=*/1, /*amount=*/-1);
  }
  array->num_elts--;
  return to_return;
}

void array_enqueue(Array* const array, Element elt) {
  ASSERT(NOT_NULL(array));
  array_maybe_realloc(array, array->num_elts + 1);
  array->table[array->num_elts++] = elt;
}

Element array_dequeue(Array* const array) {
  ASSERT(NOT_NULL(array), array->num_elts != 0);
  Element to_return = array->table[array->num_elts - 1];
  array->num_elts--;
  return to_return;
}
//
//void array_insert(Array* const array, uint32_t index, Element elt) {
//
//}
//
//Element array_remove(Array* const array, uint32_t index) {
//
//}
//

void array_set(Array* const array, uint32_t index, Element elt) {
  ASSERT(NOT_NULL(array), index >= 0);
  if (index >= array->num_elts) {
    array_maybe_realloc(array, index + 1);
    array->num_elts = index + 1;
  }
  array->table[index] = elt;
}

Element array_get(Array* const array, uint32_t index) {
  ASSERT(NOT_NULL(array), index >= 0, index < array->num_elts);
  return array->table[index];
}

void array_print(const Array* const array, FILE *file) {
  ASSERT(NOT_NULL(array));
  int i;
  fprintf(file, "[");
  if (array_size(array) > 0) {
    elt_to_str(array->table[0], file);
  }
  for (i = 1; i < array->num_elts; i++) {
    fprintf(file, ",");
    elt_to_str(array->table[i], file);
  }
  fprintf(file, "]");
}

uint32_t array_size(const Array* const array) {
  ASSERT(NOT_NULL(array));
  return array->num_elts;
}
bool array_is_empty(const Array* const array) {
  ASSERT(NOT_NULL(array));
  return array->num_elts == 0;
}

Array *array_copy(const Array* const array) {
  ASSERT(NOT_NULL(array));
  Array *copy = ALLOC2(Array);
  *copy = *array;
  copy->table = ALLOC_ARRAY2(Element, array->table_size);
  memcpy(copy->table, array->table, sizeof(Element) * array->table_size);
  return copy;
}

void array_append(Array* const head, const Array* const tail) {
  ASSERT(NOT_NULL(head), NOT_NULL(tail));
  array_maybe_realloc(head, head->num_elts + tail->num_elts);
  memcpy(head->table + head->num_elts, tail->table,
      sizeof(Element) * tail->num_elts);
  head->num_elts += tail->num_elts;
}

//Array *array_join(const Array * const array1, const Array * const array2) {
//  ASSERT(NOT_NULL(array1), NOT_NULL(array2));
//  Array *cp1 = array_copy(array1);
//  array_append(cp1, array2);
//  return cp1;
//}
