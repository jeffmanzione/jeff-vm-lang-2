/*
 * strings.c
 *
 *  Created on: Jun 3, 2018
 *      Author: Jeff
 */

#include "strings.h"

#include <inttypes.h>
#include <stdio.h>

#include "../element.h"
#include "../error.h"

Element stringify__(VM *vm, ExternalData *ed, Element argument) {
  ASSERT(argument.type == VALUE);
  Value val = argument.val;
  char buffer[128];
  switch (val.type) {
  case INT:
    sprintf(buffer, "%" PRId64, val.int_val);
    break;
  case FLOAT:
    sprintf(buffer, "%f", val.float_val);
    break;
  default /*CHAR*/:
    sprintf(buffer, "%c", val.char_val);
    break;
  }
  return string_create(vm, buffer);
}
