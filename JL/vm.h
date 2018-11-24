/*
 * vm.h
 *
 *  Created on: Dec 8, 2016
 *      Author: Jeff
 */

#ifndef VM_H_
#define VM_H_

#include <stdbool.h>
#include <stdint.h>

#include "command/commandline.h"
#include "element.h"
#include "instruction.h"
#include "threads/thread_interface.h"

typedef struct MemoryGraph_ MemoryGraph;
typedef struct Module_ Module;

struct VM_ {
  ArgStore *store;
  MemoryGraph *graph;
  Element root, modules;
  ThreadHandle debug_mutex, module_init_mutex;
};

VM *vm_create(ArgStore *store);
Element vm_get_module(const VM *vm, const Thread *t);
void vm_set_module(VM *vm, Thread *t, Element module_element, uint32_t ip);
Element vm_add_module(VM *vm, const Module *module);
Element vm_lookup_module(const VM *vm, const char module_name[]);
Element vm_object_lookup(VM *vm, Element obj, const char name[]);
const MemoryGraph *vm_get_graph(const VM *vm);
void vm_delete(VM *vm);

void vm_add_string_class(VM *vm);

void vm_throw_error(VM *vm, Thread *t, Ins ins, const char fmt[], ...);
Ins vm_current_ins(const VM *vm, Thread *t);
Element vm_current_block(const VM *vm, const Thread *t);
uint32_t vm_get_ip(const VM *vm, Thread *t);

void vm_pushstack(VM *vm, Thread *t, Element element);
//Element vm_popstack(VM *vm);
const Element vm_get_resval(VM *vm, Thread *t);
Element vm_lookup(VM *vm, Thread *t, const char name[]);
void vm_set_resval(VM *vm, Thread *t, const Element elt);

// Creates a block with the specified obj as the parent
Element vm_new_block(VM *vm, Thread *t, Element parent, Element new_this);
// goes back one block
bool vm_back(VM *vm, Thread *t);

bool execute(VM *vm, Thread *t);

void vm_maybe_initialize_and_execute(VM *vm, Thread *t, const Module *module);

void vm_call_fn(VM *vm, Thread *t, Element obj, Element func);
void vm_call_new(VM *vm, Thread *t, Element class);

void vm_start_execution(VM *vm, Element module);

void vm_shift_ip(VM *vm, Thread *t, int32_t);

#endif /* VM_H_ */
