/*
 * math.c
 *
 *  Created on: Aug 3, 2018
 *      Author: Jeff
 */

#include <math.h>

#include "../codegen/tokenizer.h"
#include "../datastructure/tuple.h"
#include "../element.h"
#include "external.h"

double log_special(double base, double num) { return log10(num) / log10(base); }

Element log_tuple(VM *vm, Thread *th, Element *tuple) {
  Tuple *t = tuple->obj->tuple;
  if (tuple_size(t) != 2) {
    return throw_error(vm, th, "Invalid tuple arg to log__.");
  }
  Element base = tuple_get(t, 0);
  Element num = tuple_get(t, 1);

  if (base.type != VALUE) {
    return throw_error(vm, th, "Cannot perform log__ with non-numeric base.");
  }
  if (num.type != VALUE) {
    return throw_error(vm, th, "Cannot perform log__ with non-numeric input.");
  }
  return create_float(
      log_special((double)VALUE_OF(base.val), (double)VALUE_OF(num.val)));
}

Element log__(VM *vm, Thread *t, ExternalData *ed, Element *argument) {
  if (is_object_type(argument, TUPLE)) {
    return log_tuple(vm, t, argument);
  }
  if (argument->type != VALUE) {
    return throw_error(vm, t, "Cannot perform log__ on a non-value.");
  }
  return create_float(log((double)VALUE_OF(argument->val)));
}

Element pow__(VM *vm, Thread *th, ExternalData *ed, Element *argument) {
  if (!is_object_type(argument, TUPLE)) {
    return throw_error(vm, th, "pow_ expects multiple arguments.");
  }
  Tuple *t = argument->obj->tuple;
  if (tuple_size(t) != 2) {
    return throw_error(vm, th, "pow__ expects exactly 2 arguments.");
  }
  Element num = tuple_get(t, 0);
  Element power = tuple_get(t, 1);

  if (num.type != VALUE) {
    return throw_error(vm, th, "Cannot perform pow__ with non-numeric input.");
  }
  if (power.type != VALUE) {
    return throw_error(vm, th, "Cannot perform pow__ with non-numeric power.");
  }
  return create_float(
      pow((double)VALUE_OF(num.val), (double)VALUE_OF(power.val)));
}
