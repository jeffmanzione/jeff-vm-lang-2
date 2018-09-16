/*
 * strings.c
 *
 *  Created on: Jun 3, 2018
 *      Author: Jeff
 */

#include "strings.h"

#include <inttypes.h>
#include <stdio.h>

#include "external.h"
#include "../arena/strings.h"
#include "../class.h"
#include "../element.h"
#include "../error.h"
#include "../datastructure/arraylike.h"
#include "../datastructure/map.h"
#include "../datastructure/tuple.h"
#include "../graph/memory.h"
#include "../graph/memory_graph.h"
#include "../vm.h"

Element stringify__(VM *vm, ExternalData *ed, Element argument) {
  ASSERT(argument.type == VALUE);
  Value val = argument.val;
  static const int BUFFER_SIZE = 128;
  char buffer[BUFFER_SIZE];
  int num_written = 0;
  switch (val.type) {
  case INT:
    num_written = snprintf(buffer, BUFFER_SIZE, "%" PRId64, val.int_val);
    break;
  case FLOAT:
    num_written = snprintf(buffer, BUFFER_SIZE, "%f", val.float_val);
    break;
  default /*CHAR*/:
    num_written = snprintf(buffer, BUFFER_SIZE, "%c", val.char_val);
    break;
  }
  ASSERT(num_written > 0);
  return string_create_len(vm, buffer, num_written);
}

IMPL_ARRAYLIKE(String, char);

const char *String_cstr(const String * const string) {
  return string->table;
}

void String_append_unescape(String *string, const char str[], size_t len) {
  int i;
  for (i = 0; i < len; i++) {
    char c = str[i];
    if (c == '\\') {
      ++i;
      c = char_unesc(str[i]);
    }
    String_enqueue(string, c);
  }
}

void String_insert(String *string, int index_in_string, const char src[],
    size_t len) {
  String_shift(string, index_in_string, len);
  memmove(string->table + sizeof(char) * index_in_string, src,
      sizeof(char) * len);
  string->num_elts += len;
}

void String_append_cstr(String *string, const char src[], size_t len) {
  String_insert(string, string->num_elts, src, len);
}

String *String_of(const char *src, size_t len) {
  ASSERT(NOT_NULL(src));
  String *string = String_create();
  String_append_cstr(string, src, len);
  return string;
}

Element string_constructor(VM *vm, ExternalData *data, Element arg) {
  ASSERT(NOT_NULL(data));
  String *string = String_create();
  map_insert(&data->state, strings_intern("string"), string);
  memory_graph_set_field(vm->graph, data->object, LENGTH_KEY, create_int(0));
  return data->object;
}

Element string_deconstructor(VM *vm, ExternalData *data, Element arg) {
  String *string = map_lookup(&data->state, strings_intern("string"));
  if (NULL != string) {
    String_delete(string);
  }
  return create_none();
}

Element string_index(VM *vm, ExternalData *data, Element arg) {
  ASSERT(arg.type == VALUE && arg.val.type == INT);
  String *string = map_lookup(&data->state, strings_intern("string"));
  ASSERT(NOT_NULL(string));
  return create_char(String_get(string, arg.val.int_val));
}

Element string_set(VM *vm, ExternalData *data, Element arg) {
  if (!is_object_type(&arg, TUPLE)) {
    ERROR("Unknown input.");
    return create_none();

  }
  Tuple *args = arg.obj->tuple;
  Element index = tuple_get(args, 0);
  Element val = tuple_get(args, 1);
//  elt_to_str(val, stdout);
//  printf("\n");fflush(stdout);
  ASSERT(index.type == VALUE, index.val.type == INT);
  String *string = map_lookup(&data->state, strings_intern("string"));
  ASSERT(NOT_NULL(string));
  if (val.type == VALUE && val.val.type == CHAR) {
    String_set(string, index.val.int_val, val.val.char_val);
  } else if (OBJECT == val.type
      && obj_get_field(val, CLASS_KEY).obj == class_string.obj) {
    String_append(string, String_extract(val));
  } else {
    ERROR("BAD STRING.");
  }
  memory_graph_set_field(vm->graph, data->object, LENGTH_KEY,
      create_int(String_size(string)));
  return create_none();
}

String *String_extract(Element elt) {
  ASSERT(obj_get_field(elt, CLASS_KEY).obj == class_string.obj);
  String *string = map_lookup(&elt.obj->external_data->state,
      strings_intern("string"));
  ASSERT(NOT_NULL(string));
  return string;
}

void merge_string_class(VM *vm, Element string_class) {
  merge_external_class(vm, string_class, string_constructor,
      string_deconstructor);
  add_external_function(vm, string_class, strings_intern("__index__"),
      string_index);
  add_external_function(vm, string_class, strings_intern("__set__"),
      string_set);
}
