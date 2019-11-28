/*
 * modules.c
 *
 *  Created on: Jun 24, 2018
 *      Author: Jeff
 */

#include "modules.h"

#include "../arena/strings.h"
#include "../class.h"
#include "../error.h"
#include "../file_load.h"
#include "../vm/vm.h"

Element load_module__(VM *vm, Thread *thread, ExternalData *ed,
                      Element *argument) {
  ASSERT(ISTYPE(*argument, class_string));
  Module *module = load_fn(string_to_cstr(*argument), vm->store);
  return vm_add_module(vm, module);
}
