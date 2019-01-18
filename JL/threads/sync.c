/*
 * sync.c
 *
 *  Created on: Nov 11, 2018
 *      Author: Jeff
 */

#include "sync.h"

#include "../arena/strings.h"
#include "../external/external.h"
#include "mutex.h"
#include "rwlock.h"
#include "semaphore.h"
#include "thread.h"
#include "thread_interface.h"

Element sleep_fn(VM *vm, Thread *t, ExternalData *data, Element arg) {
  if (!is_value_type(&arg, INT)) { // @suppress("Symbol is not resolved")
    return throw_error(vm, t, "sleep() requires type Int.");
  }
  sleep_thread(VALUE_OF(arg.val)); // @suppress("Symbol is not resolved")
  return create_none();
}

void add_sync_external(VM *vm, Element module_element) {
  add_external_function(vm, module_element, strings_intern("sleep"), sleep_fn);
  add_thread_class(vm, module_element);
  add_mutex_class(vm, module_element);
  add_semaphore_class(vm, module_element);
  add_rwlock_class(vm, module_element);
}
