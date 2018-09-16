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

typedef struct MemoryGraph_ MemoryGraph;
typedef struct Module_ Module;

struct VM_ {
  ArgStore *store;
  MemoryGraph *graph;
  Element root, stack,
//  class_manager,
      saved_blocks, modules;
};

VM *vm_create(ArgStore *store);
void vm_set_module(VM *vm, Element module_element, uint32_t ip);
Element vm_add_module(VM *vm, const Module *module);
Element vm_lookup_module(const VM *vm, const char module_name[]);
const MemoryGraph *vm_get_graph(const VM *vm);
void vm_delete(VM *vm);

void vm_add_string_class(VM *vm);

void vm_throw_error(VM *vm, Ins ins, const char fmt[], ...);
Ins vm_current_ins(const VM *vm);

void vm_pushstack(VM *vm, Element element);
Element vm_popstack(VM *vm);
const Element vm_get_resval(VM *vm);
Element vm_lookup(VM *vm, const char name[]);

// Creates a block with the specified obj as the parent
Element vm_new_block(VM *vm, Element parent, Element new_this);
// goes back one block
void vm_back(VM *vm);

bool execute(VM *vm);

void vm_maybe_initialize_and_execute(VM *vm, const Module *module);

#endif /* VM_H_ */
