/*
 * rwlock.c
 *
 *  Created on: Jan 11, 2019
 *      Author: Jeff
 */

#include "../arena/strings.h"
#include "../datastructure/map.h"
#include "../element.h"
#include "../external/external.h"
#include "thread_interface.h"

Element RWLock_constructor(VM *vm, Thread *t, ExternalData *data,
                           Element *arg) {
  RWLock *lock = create_rwlock();
  if (NULL == lock) {
    return throw_error(vm, t, "Failed to create RWLock.");
  }
  map_insert(&data->state, strings_intern("lock"), lock);
  return data->object;
}

Element RWLock_deconstructor(VM *vm, Thread *t, ExternalData *data,
                             Element *arg) {
  RWLock *lock = map_lookup(&data->state, strings_intern("lock"));
  if (NULL == lock) {
    return create_none();
  }
  close_rwlock(lock);
  return data->object;
}

Element RWLock_begin_read(VM *vm, Thread *t, ExternalData *data, Element *arg) {
  RWLock *lock = map_lookup(&data->state, strings_intern("lock"));
  if (NULL == lock) {
    return throw_error(vm, t, "Failed to begin read RWLock.");
  }
  begin_read(lock);
  return data->object;
}

Element RWLock_end_read(VM *vm, Thread *t, ExternalData *data, Element *arg) {
  RWLock *lock = map_lookup(&data->state, strings_intern("lock"));
  if (NULL == lock) {
    return throw_error(vm, t, "Failed to end read RWLock.");
  }
  end_read(lock);
  return data->object;
}

Element RWLock_begin_write(VM *vm, Thread *t, ExternalData *data,
                           Element *arg) {
  RWLock *lock = map_lookup(&data->state, strings_intern("lock"));
  if (NULL == lock) {
    return throw_error(vm, t, "Failed to begin write RWLock.");
  }
  begin_write(lock);
  return data->object;
}

Element RWLock_end_write(VM *vm, Thread *t, ExternalData *data, Element *arg) {
  RWLock *lock = map_lookup(&data->state, strings_intern("lock"));
  if (NULL == lock) {
    return throw_error(vm, t, "Failed to end write RWLock.");
  }
  end_write(lock);
  return data->object;
}

Element add_rwlock_class(VM *vm, Element module) {
  Element rwlock_class =
      create_external_class(vm, module, strings_intern("RWLock"),
                            RWLock_constructor, RWLock_deconstructor);
  add_external_method(vm, rwlock_class, strings_intern("acquire_read"),
                      RWLock_begin_read);
  add_external_method(vm, rwlock_class, strings_intern("release_read"),
                      RWLock_end_read);
  add_external_method(vm, rwlock_class, strings_intern("acquire_write"),
                      RWLock_begin_write);
  add_external_method(vm, rwlock_class, strings_intern("release_write"),
                      RWLock_end_write);
  return rwlock_class;
}
