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
#include "../external/external.h"
#include "thread_interface.h"

Element Mutex_constructor(VM *vm, Thread *t, ExternalData *data, Element arg) {
  ThreadHandle handle = create_mutex(NULL);
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
  close_mutex(handle);
  return data->object;
}

Element Mutex_acquire(VM *vm, Thread *t, ExternalData *data, Element arg) {
  ThreadHandle handle = map_lookup(&data->state, strings_intern("handle"));
  if (NULL == handle) {
    return throw_error(vm, t, "Failed to acquire Mutex.");
  }
  wait_for_mutex(handle, INFINITE);
  return data->object;
}
Element Mutex_release(VM *vm, Thread *t, ExternalData *data, Element arg) {
  ThreadHandle handle = map_lookup(&data->state, strings_intern("handle"));
  if (NULL == handle) {
    return throw_error(vm, t, "Failed to release Mutex.");
  }
  release_mutex(handle);
  return data->object;
}

Element add_mutex_class(VM *vm, Element module) {
  Element mutex_class = create_external_class(vm, module,
      strings_intern("Mutex"), Mutex_constructor, Mutex_deconstructor);
  add_external_function(vm, mutex_class, strings_intern("acquire"),
      Mutex_acquire);
  add_external_function(vm, mutex_class, strings_intern("release"),
      Mutex_release);
  return mutex_class;
}
