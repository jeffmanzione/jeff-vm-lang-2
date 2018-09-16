/*
 * external.c
 *
 *  Created on: Jun 2, 2018
 *      Author: Jeff
 */

#include "external.h"

#include "../arena/strings.h"
#include "../class.h"
#include "../datastructure/map.h"
#include "../error.h"
#include "../graph/memory.h"
#include "../graph/memory_graph.h"
#include "../vm.h"
#include "error.h"
#include "file.h"
#include "math.h"
#include "modules.h"
#include "strings.h"

ExternalData *externaldata_create(VM *vm, Element obj, Element class) {
  ExternalData *ed = ALLOC2(ExternalData);
  map_init_default(&ed->state);
  ed->vm = vm;
  ed->object = obj;
  ed->deconstructor = obj_get_field(class, DECONSTRUCTOR_KEY).obj->external_fn;
  return ed;
}

VM *externaldata_vm(const ExternalData * const ed) {
  return ed->vm;
}

void externaldata_delete(ExternalData *ed) {
  ASSERT(NOT_NULL(ed));
  map_finalize(&ed->state);
  DEALLOC(ed);
}

void add_io_external(VM *vm, Element builtin) {
  create_file_class(vm, builtin);
}

void add_builtin_external(VM *vm, Element builtin) {
  add_external_function(vm, builtin, "stringify__", stringify__);
  add_external_function(vm, builtin, "token__", token__);
  add_external_function(vm, builtin, "load_module__", load_module__);
  add_external_function(vm, builtin, "log__", log__);
  add_external_function(vm, builtin, "pow__", pow__);
}

void add_external_function(VM *vm, Element parent, const char fn_name[],
    ExternalFunction fn) {
  const char *fn_name_str = strings_intern(fn_name);
  memory_graph_set_field(vm->graph, parent, fn_name_str,
      create_external_function(vm, parent, fn_name_str, fn));
}

void merge_external_class(VM *vm, Element class, ExternalFunction constructor,
    ExternalFunction deconstructor) {
  memory_graph_set_field(vm->graph, class, IS_EXTERNAL_KEY, create_int(1));
  add_external_function(vm, class, CONSTRUCTOR_KEY, constructor);
  add_external_function(vm, class, DECONSTRUCTOR_KEY, deconstructor);
}

Element create_external_class(VM *vm, Element module, const char class_name[],
    ExternalFunction constructor, ExternalFunction deconstructor) {
  const char *name = strings_intern(class_name);
  Element class = class_create(vm, name, class_object);
  merge_external_class(vm, class, constructor, deconstructor);
  memory_graph_set_field(vm->graph, module, name, class);
  return class;
}

bool is_value_type(const Element *e, int type) {
  if (e->type != VALUE) {
    return false;
  }
  return e->val.type == type;
}

bool is_object_type(const Element *e, int type) {
  if (e->type != OBJECT) {
    return false;
  }
  return e->obj->type == type;
}

Element throw_error(VM *vm, const char msg[]) {
  vm_throw_error(vm, vm_current_ins(vm), msg);
  return vm_get_resval(vm);
}

