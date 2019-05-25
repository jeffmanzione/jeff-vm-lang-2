/*
 * semaphore.c
 *
 *  Created on: Nov 11, 2018
 *      Author: Jeff
 */

#include <stddef.h>
#include <stdint.h>

#include "../arena/strings.h"
#include "../datastructure/map.h"
#include "../datastructure/tuple.h"
#include "../element.h"
#include "../external/external.h"
#include "thread_interface.h"

Element Semaphore_constructor(VM *vm, Thread *t, ExternalData *data,
                              Element *arg) {
  uint64_t initial_count = 1;
  uint64_t max_count = 1;
  if (!is_object_type(arg, TUPLE) || tuple_size(arg->obj->tuple) != 2) {
    return throw_error(vm, t,
                       "Semaphore requires an initial count and max count.");
  }
  Tuple *args = arg->obj->tuple;
  Element arg0 = tuple_get(args, 0);
  Element arg1 = tuple_get(args, 1);
  if (!is_value_type(&arg0, INT) ||  // @suppress("Symbol is not resolved")
      !is_value_type(&arg1, INT)) {  // @suppress("Symbol is not resolved")
    return throw_error(
        vm, t, "Semaphore requires an initial count and max count to be Ints.");
  }
  initial_count = tuple_get(args, 0).val.int_val;
  max_count = tuple_get(args, 1).val.int_val;

  Semaphore handle = semaphore_create(initial_count, max_count);
  if (NULL == handle) {
    return throw_error(vm, t, "Failed to create Semaphore.");
  }
  map_insert(&data->state, strings_intern("handle"), handle);
  return data->object;
}

Element Semaphore_deconstructor(VM *vm, Thread *t, ExternalData *data,
                                Element *arg) {
  Semaphore handle = map_lookup(&data->state, strings_intern("handle"));
  if (NULL == handle) {
    return create_none();
  }
  semaphore_close(handle);
  return data->object;
}

Element Semaphore_lock(VM *vm, Thread *t, ExternalData *data, Element *arg) {
  Semaphore handle = map_lookup(&data->state, strings_intern("handle"));
  if (NULL == handle) {
    return throw_error(vm, t, "Failed while trying to lock Semaphore.");
  }

  ulong duration;
  if (NONE == arg->type) {
    duration = INFINITE;
  } else if (is_value_type(arg, INT)) {  // @suppress("Symbol is not resolved")
    duration = VALUE_OF(arg->val);       // @suppress("Symbol is not resolved")
  } else {
    return throw_error(vm, t, "Semaphore.lock() requires type Int.");
  }
  WaitStatus status = semaphore_lock(handle, duration);
  return create_int(status);
}

Element Semaphore_unlock(VM *vm, Thread *t, ExternalData *data, Element *arg) {
  Semaphore handle = map_lookup(&data->state, strings_intern("handle"));
  if (NULL == handle) {
    return throw_error(vm, t, "Failed to unlock Semaphore.");
  }
  semaphore_unlock(handle);
  return data->object;
}

Element add_semaphore_class(VM *vm, Element module) {
  Element semaphore_class =
      create_external_class(vm, module, strings_intern("Semaphore"),
                            Semaphore_constructor, Semaphore_deconstructor);
  add_external_method(vm, semaphore_class, strings_intern("lock"),
                      Semaphore_lock);
  add_external_method(vm, semaphore_class, strings_intern("unlock"),
                      Semaphore_unlock);
  return semaphore_class;
}
