/*
 * mutex.c
 *
 *  Created on: Nov 11, 2018
 *      Author: Jeff
 */

#include "mutex.h"

#include <stddef.h>

#include "../arena/strings.h"
#include "../datastructure/map.h"
#include "../error.h"
#include "../external/external.h"
#include "thread_interface.h"

Element Mutex_constructor(VM *vm, Thread *t, ExternalData *data, Element arg) {
  ThreadHandle handle = mutex_create(NULL);
  if (NULL == handle) {
    return throw_error(vm, t, "Failed to create Mutex.");
  }
  map_insert(&data->state, strings_intern("handle"), handle);
  return data->object;
}

Element Mutex_deconstructor(VM *vm, Thread *t, ExternalData *data, Element arg) {
  ThreadHandle handle = map_lookup(&data->state, strings_intern("handle"));
  if (NULL == handle) {
    return create_none();
  }
  mutex_close(handle);
  return data->object;
}

Element Mutex_acquire(VM *vm, Thread *t, ExternalData *data, Element arg) {
  ThreadHandle handle = map_lookup(&data->state, strings_intern("handle"));
  if (NULL == handle) {
    return throw_error(vm, t, "Failed to acquire Mutex.");
  }
//  ulong duration;
//  if (NONE == arg.type) {
//    duration = INFINITE;
//  } else if (is_value_type(&arg, INT)) {
//    duration = VALUE_OF(arg.val);
//  } else {
//    return throw_error(vm, t, "Mutex.wait() requires type Int.");
//  }
  WaitStatus status = mutex_await(handle, INFINITE);
  if (status != WAIT_OBJECT_0) {
    if (status == WAIT_TIMEOUT) {
      return throw_error(vm, t, "Mutex.wait() timed out.");
    }
    return throw_error(vm, t, "Mutex.wait() failed.");
  }
//  DEBUGF("Mutex_acquire=%d", status);

  return data->object;
}

Element Mutex_release(VM *vm, Thread *t, ExternalData *data, Element arg) {
  ThreadHandle handle = map_lookup(&data->state, strings_intern("handle"));
  if (NULL == handle) {
    return throw_error(vm, t, "Failed to release Mutex.");
  }
  mutex_release(handle);
  return data->object;
}

Element add_mutex_class(VM *vm, Element module) {
  Element mutex_class = create_external_class(vm, module,
      strings_intern("Mutex"), Mutex_constructor, Mutex_deconstructor);
  add_external_method(vm, mutex_class, strings_intern("acquire"),
      Mutex_acquire);
  add_external_method(vm, mutex_class, strings_intern("release"),
      Mutex_release);
  return mutex_class;
}
