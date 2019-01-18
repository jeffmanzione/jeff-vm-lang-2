/*
 * arraylike.h
 *
 *  Created on: Sep 15, 2018
 *      Author: Jeff
 */

#ifndef DATASTRUCTURE_ARRAYLIKE_H_
#define DATASTRUCTURE_ARRAYLIKE_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define DEFAULT_TABLE_SIZE 8

#define DEFINE_ARRAYLIKE(name, type) \
  typedef struct name##_ name;\
  struct name##_ {\
    uint32_t table_size;\
    uint32_t num_elts;\
    type *table;\
  };\
  void name##_init(name*);\
  name *name##_create();\
  name *name##_create_copy(const type input[], size_t len);\
  void name##_finalize(name*);\
  void name##_delete(name*);\
  void name##_clear(name* const);\
  void name##_lshrink(name* const array, size_t amount);\
  void name##_rshrink(name* const array, size_t amount);\
  void name##_push(name* const, type);\
  type name##_pop(name* const);\
  void name##_enqueue(name* const, type);\
  type name##_dequeue(name* const);\
  void name##_set(name* const, uint32_t, type);\
  type name##_get(name* const, uint32_t);\
  type name##_remove(name* const, uint32_t);\
  uint32_t name##_size(const name* const);\
  bool name##_is_empty(const name* const);\
  name *name##_copy(const name* const);\
  void name##_append(name* const head, const name* const tail);

#define IMPL_ARRAYLIKE(name, type) \
  void name##_init_sz(name* array, size_t table_sz) {\
    array->table = ALLOC_ARRAY(type, array->table_size = table_sz);\
    array->num_elts = 0;\
  }\
  void name##_init(name *array) {\
    name##_init_sz(array, DEFAULT_TABLE_SIZE);\
  }\
  name *name##_create() {\
    name *array = ALLOC2(name);\
    name##_init(array);\
    return array;\
  }\
  name *name##_create_copy(const type input[], size_t len) {\
    name *array = ALLOC2(name);\
    name##_init_sz(array,\
                   ((len / DEFAULT_TABLE_SIZE) + 1) * DEFAULT_TABLE_SIZE);\
    memmove(array->table, input, len * sizeof(type));\
    array->num_elts = len;\
    return array;\
  }\
  void name##_finalize(name* array) {\
    ASSERT(NOT_NULL(array));\
    DEALLOC(array->table);\
  }\
  \
  void name##_delete(name* array) {\
    ASSERT(NOT_NULL(array));\
    name##_finalize(array);\
    DEALLOC(array);\
  }\
  \
  void name##_maybe_realloc(name * const array, uint32_t need_to_accomodate) {\
    ASSERT(NOT_NULL(array));\
    if (need_to_accomodate <= 0) {\
      return;\
    }\
    uint32_t size =\
        max(array->num_elts,\
            max(need_to_accomodate, (need_to_accomodate / DEFAULT_TABLE_SIZE + 1) * DEFAULT_TABLE_SIZE));\
    if (size <= 0 || size <= array->table_size) {\
      return;\
    }\
    array->table = REALLOC(array->table, type, size);\
    array->table_size = size;\
    memset(array->table + array->num_elts, 0x0,\
        (array->table_size - array->num_elts) * sizeof(type));\
  }\
  \
  void name##_shift(name * const array, uint32_t start_pos, int32_t amount) {\
    ASSERT(NOT_NULL(array), start_pos >= 0, start_pos + amount >= 0);\
    if (0 == amount) { return; }\
    name##_maybe_realloc(array, array->num_elts + amount);\
    memmove(array->table + start_pos + amount, array->table + start_pos,\
        (array->num_elts - start_pos) * sizeof(type));\
    if (amount > 0) {\
      memset(array->table + start_pos, 0x0, amount * sizeof(type));\
    }\
  }\
  \
  void name##_clear(name* const array) {\
    ASSERT(NOT_NULL(array));\
    name##_maybe_realloc(array, array->num_elts = 0);\
  }\
  \
  void name##_push(name* const array, type elt) {\
    ASSERT(NOT_NULL(array));\
    if (array->num_elts > 0) {\
      name##_shift(array, /*start_pos=*/0, /*amount=*/1);\
    }\
    array->table[0] = elt;\
    array->num_elts++;\
  }\
  \
  type name##_pop(name* const array) {\
    ASSERT(NOT_NULL(array), array->num_elts != 0);\
    type to_return = array->table[0];\
    if (array->num_elts > 1) {\
      name##_shift(array, /*start_pos=*/1, /*amount=*/-1);\
    }\
    array->num_elts--;\
    return to_return;\
  }\
  \
  void name##_enqueue(name* const array, type elt) {\
    ASSERT(NOT_NULL(array));\
    name##_maybe_realloc(array, array->num_elts + 1);\
    array->table[array->num_elts++] = elt;\
  }\
  \
  type name##_dequeue(name* const array) {\
    ASSERT(NOT_NULL(array), array->num_elts != 0);\
    type to_return = array->table[array->num_elts - 1];\
    array->num_elts--;\
    return to_return;\
  }\
  \
  void name##_lshrink(name* const array, size_t amount) {\
    ASSERT(NOT_NULL(array), array->num_elts >= amount);\
    name##_shift(array, amount, -amount);\
    array->num_elts -= amount;\
  }\
  \
  void name##_rshrink(name* const array, size_t amount) {\
    ASSERT(NOT_NULL(array), array->num_elts >= amount);\
    array->num_elts -= amount;\
  }\
  \
  void name##_set(name* const array, uint32_t index, type elt) {\
    ASSERT(NOT_NULL(array), index >= 0);\
    if (index >= array->num_elts) {\
      name##_maybe_realloc(array, index + 1);\
      array->num_elts = index + 1;\
    }\
    array->table[index] = elt;\
  }\
  \
  type name##_get(name* const array, uint32_t index) {\
    ASSERT(NOT_NULL(array), index >= 0, index < array->num_elts);\
    return array->table[index];\
  }\
  \
  type name##_remove(name* const array, uint32_t index) {\
    ASSERT(NOT_NULL(array), index >= 0, index < array->num_elts);\
    type to_return = array->table[index];\
    if (array->num_elts > 1) {\
      name##_shift(array, /*start_pos=*/index+1, /*amount=*/-1);\
    }\
    array->num_elts--;\
    return to_return;\
  }\
  \
  uint32_t name##_size(const name* const array) {\
    ASSERT(NOT_NULL(array));\
    return array->num_elts;\
  }\
  bool name##_is_empty(const name* const array) {\
    ASSERT(NOT_NULL(array));\
    return array->num_elts == 0;\
  }\
  \
  name *name##_copy(const name* const array) {\
    ASSERT(NOT_NULL(array));\
    name *copy = ALLOC2(name);\
    *copy = *array;\
    copy->table = ALLOC_ARRAY2(type, array->table_size);\
    memcpy(copy->table, array->table, sizeof(type) * array->table_size);\
    return copy;\
  }\
  \
  void name##_append(name* const head, const name* const tail) {\
    ASSERT(NOT_NULL(head), NOT_NULL(tail));\
    name##_maybe_realloc(head, head->num_elts + tail->num_elts);\
    memcpy(head->table + head->num_elts, tail->table,\
        sizeof(type) * tail->num_elts);\
    head->num_elts += tail->num_elts;\
  }

#endif /* DATASTRUCTURE_ARRAYLIKE_H_ */
