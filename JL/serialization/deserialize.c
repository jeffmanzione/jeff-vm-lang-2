/*
 * deserialize.c
 *
 *  Created on: Nov 10, 2017
 *      Author: Jeff
 */

#include "deserialize.h"

#include "../element.h"
#include "../error.h"
#include "../program/instruction.h"

#define deserialize_type(file, type, p) deserialize_bytes(file, sizeof(type), ((char *)p), sizeof(type))

int deserialize_bytes(FILE *file, uint32_t num_bytes, char *buffer,
    uint32_t buffer_sz) {
  ASSERT(NOT_NULL(file), NOT_NULL(buffer));
  if (num_bytes > buffer_sz) {
    return 0;
  }
  return fread(buffer, sizeof(char), num_bytes, file);
}

int deserialize_string(FILE *file, char *buffer, uint32_t buffer_sz) {
  char *start = buffer;

  do {
    if (!fread(buffer, sizeof(char), 1, file)) {
      *buffer = '\0';
    }
  } while ('\0' != *(buffer++));
  return buffer - start;
}

int deserialize_val(FILE *file, Value *val) {
  int i = 0;
  uint8_t val_type;
  i += deserialize_type(file, uint8_t, &val_type);
  val->type = val_type;
  switch (val->type) {
  case INT:
    i += deserialize_type(file, int64_t, &val->int_val);
    break;
  case FLOAT:
    i += deserialize_type(file, double, &val->float_val);
    break;
  case CHAR:
  default:
    i += deserialize_type(file, int8_t, &val->char_val);
  }
  return i;
}

int deserialize_ins(FILE *file, const Expando* const strings, InsContainer *c) {
  int i = 0;
  uint8_t op;
  uint8_t param;
  i += deserialize_type(file, uint8_t, &op);
  i += deserialize_type(file, uint8_t, &param);
  c->ins.op = op;
  c->ins.param = param;
  uint16_t ref16;
  uint8_t ref8;
  switch (param) {
  case VAL_PARAM:
    i += deserialize_val(file, &c->ins.val);
    break;
  case STR_PARAM:
  case ID_PARAM:
    if (expando_len(strings) > UINT8_MAX) {
      i += deserialize_type(file, uint16_t, &ref16);
    } else {
      i += deserialize_type(file, uint8_t, &ref8);
      ref16 = (uint16_t) ref8;
    }
    char *str = *((char **) expando_get(strings, (uint32_t) ref16));
    if (ID_PARAM == param) {
      c->ins.id = str;
    } else {
      c->ins.str = str;
    }
    break;
  case NO_PARAM:
  default:
    break;
  }
  return i;
}
