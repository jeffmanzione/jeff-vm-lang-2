/*
 * vm.h
 *
 *  Created on: Dec 8, 2016
 *      Author: Jeff
 */

#ifndef VM_VM_H_
#define VM_VM_H_

#include <stdbool.h>
#include <stdint.h>

#include "../command/commandline.h"
#include "../element.h"
#include "../program/instruction.h"
#include "../threads/thread_interface.h"

#define vm_lookup_module(...) CALL_FN(vm_lookup_module__, __VA_ARGS__)

typedef struct MemoryGraph_ MemoryGraph;
typedef struct Module_ Module;

struct VM_ {
  ArgStore *store;
  MemoryGraph *graph;
  Element root, modules;
  ThreadHandle debug_mutex, module_init_mutex;
};

VM *vm_create(ArgStore *store);
Element vm_add_module(VM *vm, const Module *module);
Element vm_object_lookup(VM *vm, Element obj, const char name[]);
const MemoryGraph *vm_get_graph(const VM *vm);
void vm_delete(VM *vm);

void vm_add_string_class(VM *vm);

void vm_throw_error(VM *vm, Thread *t, Ins ins, const char fmt[], ...);

Element vm_lookup(VM *vm, Thread *t, const char name[]);

DEB_FN(Element, vm_lookup_module, const VM *vm, const char module_name[]);

void vm_maybe_initialize_and_execute(VM *vm, Thread *t, Element module_element);

void vm_call_fn(VM *vm, Thread *t, Element obj, Element func);
void vm_call_new(VM *vm, Thread *t, Element class);

void vm_start_execution(VM *vm, Element module);

void vm_set_catch_goto(VM *vm, Thread *t, uint32_t index);

bool execute(VM *vm, Thread *t);

#endif /* VM_VM_H_ */
