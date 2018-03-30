/*
 * serialize.c
 *
 *  Created on: Jul 21, 2017
 *      Author: Jeff
 */

#include "serialize.h"

#include <stdint.h>
#include <string.h>

#include "error.h"
#include "instruction.h"

int serialize_bytes(WBuffer *buffer, const char *start, int num_bytes) {
  buffer_write(buffer, start, num_bytes);
  return num_bytes;
}

int serialize_val(WBuffer *buffer, Value val) {
  int i = 0;
  uint8_t val_type = (uint8_t) val.type;
  i += serialize_type(buffer, uint8_t, val_type);
  switch (val.type) {
  case INT:
    i += serialize_type(buffer, int64_t, val.int_val);
    break;
  case FLOAT:
    i += serialize_type(buffer, double, val.float_val);
    break;
  case CHAR:
  default:
    i += serialize_type(buffer, int8_t, val.char_val);
  }
  return i;
}

int serialize_str(WBuffer *buffer, const char *str) {
  return serialize_bytes(buffer, str, strlen(str) + 1);
}

int serialize_ins(WBuffer * const buffer, const InsContainer *c,
    const Map * const string_index) {
  bool use_short = map_size(string_index) > UINT8_MAX ? true : false;
  int i = 0;
  uint8_t op = (uint8_t) c->ins.op;
  uint8_t param = (uint8_t) c->ins.param;
  i += serialize_type(buffer, uint8_t, op);
  i += serialize_type(buffer, uint8_t, param);
//  i += serialize_type(buffer, uint16_t, ins->row);
//  i += serialize_type(buffer, uint16_t, ins->col);
  uint16_t ref;
  switch (c->ins.param) {
  case VAL_PARAM:
    i += serialize_val(buffer, c->ins.val);
    break;
  case STR_PARAM:
    ref = (uint16_t) (uint32_t) map_lookup(string_index, c->ins.str);
    ASSERT(ref >= 0);
    if (use_short) {
      i += serialize_type(buffer, uint16_t, ref);
    } else {
      uint8_t ref_byte = (uint32_t) ref;
      i += serialize_type(buffer, uint8_t, ref_byte);
    }
    break;
  case ID_PARAM:
    ref = (uint16_t) (uint32_t) map_lookup(string_index, c->ins.id);
    ASSERT(ref >= 0);
    if (use_short) {
      i += serialize_type(buffer, uint16_t, ref);
    } else {
      uint8_t ref_byte = (uint32_t) ref;
      i += serialize_type(buffer, uint8_t, ref_byte);
    }
    break;
  case NO_PARAM:
  default:
    break;
  }
  return i;
}

//int serialize_module(Buffer * const buffer, const Module * const module) {
//  int i = 0;
//  i += serialize_str(buffer, module_name(module));
//  int16_t num_ins = (int16_t) module_size(module);
//  i += serialize_type(buffer, int16_t, num_ins);
//
//  Map *strings = map_create(DEFAULT_MAP_SZ, string_hasher, string_comparator);
//  int j = 1;
//  void add_string(void *str) {
//    if (!map_lookup(strings, str)) {
//      map_insert(strings, str, (void *) j++);
//      i += serialize_str(buffer, str);
//    }
//  }
//  set_iterate(module_literals(module), add_string);
//  set_iterate(module_vars(module), add_string);
//
//  void add_keys(Pair *kv) {
//    if (!map_lookup(strings, kv->key)) {
//      map_insert(strings, kv->key, (void *) j++);
//      i += serialize_str(buffer, kv->key);
//    }
//  }
//  map_iterate(module_refs(module), add_keys);
//  map_iterate(module_classes(module), add_keys);
//  void add_functions(Pair *kv) {
//    Map *fun = (Map *) kv->value;
//    map_iterate(fun, add_keys);
//  }
//  map_iterate(module_classes(module), add_functions);
//
//  void add_refs(Pair *kv) {
//    int index = (int) map_lookup(strings, kv->key);
//    i += serialize_type(buffer, int16_t, index);
//  }
//  int16_t refs_size = (int16_t) map_size(module_refs(module));
//  i += serialize_type(buffer, int16_t, refs_size);
//  map_iterate(module_refs(module), add_refs);
//
//  void add_class(Pair *kv) {
//    int index = (int) map_lookup(strings, kv->key);
//    i += serialize_type(buffer, int16_t, index);
//    Map *fun = (Map *) kv->value;
//    int16_t fun_size = (int16_t) map_size(fun);
//    i += serialize_type(buffer, int16_t, fun_size);
//    map_iterate(fun, add_refs);
//  }
//  int16_t class_size = (int16_t) map_size(module_classes(module));
//  i += serialize_type(buffer, int16_t, class_size);
//  map_iterate(module_classes(module), add_class);
//
//  for (j = 0; j < num_ins; j++) {
//    i += serialize_ins(buffer, module_ins(module, j), strings);
//  }
//  map_delete(strings);
//  return i;
//}

