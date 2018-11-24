/*
 * sync.c
 *
 *  Created on: Nov 11, 2018
 *      Author: Jeff
 */

#include "sync.h"

#include "mutex.h"
#include "thread.h"

void add_sync_external(VM *vm, Element module_element) {
  add_thread_class(vm, module_element);
  add_mutex_class(vm, module_element);
}
