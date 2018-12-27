/*
 * vm.c
 *
 *  Created on: Dec 8, 2016
 *      Author: Jeff
 */

#include "vm.h"

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "arena/strings.h"
#include "class.h"
#include "codegen/tokenizer.h"
#include "datastructure/array.h"
#include "datastructure/map.h"
#include "datastructure/tuple.h"
#include "error.h"
#include "external/external.h"
#include "external/strings.h"
#include "file_load.h"
#include "graph/memory.h"
#include "graph/memory_graph.h"
#include "instruction.h"
#include "module.h"
#include "ops.h"
#include "shared.h"
#include "tape.h"
#include "threads/thread.h"
#include "threads/sync.h"

#ifdef DEBUG
#define BUILTIN_SRC "builtin.jl"
#define IO_SRC      "io.jl"
#define STRUCT_SRC  "struct.jl"
#define ERROR_SRC   "error.jl"
#define SYNC_SRC    "sync.jl"
#else
#define BUILTIN_SRC "builtin.jb"
#define IO_SRC      "io.jb"
#define STRUCT_SRC  "struct.jb"
#define ERROR_SRC   "error.jb"
#define SYNC_SRC    "sync.jb"
#endif

const char *PRELOADED[] = { IO_SRC, STRUCT_SRC, ERROR_SRC, SYNC_SRC };

const Element vm_get_resval(VM *vm, Thread *t);
void vm_set_resval(VM *vm, Thread *t, const Element elt);
void vm_shift_ip(VM *vm, Thread *t, int num_ins);

int preloaded_size() {
  return sizeof(PRELOADED) / sizeof(PRELOADED[0]);
}

void vm_to_string(const VM *vm, Element elt, FILE *target);
void execute_tget(VM *vm, Thread *t, Ins ins, Element tuple, int64_t index);

Element vm_current_block(const VM *vm, const Thread *t) {
  return t->current_block;
}

uint32_t vm_get_ip(const VM *vm, Thread *t) {
  ASSERT_NOT_NULL(vm);
  Element block = vm_current_block(vm, t);
  ASSERT(OBJECT == block.type);
  Element ip = obj_get_field(block, IP_FIELD);
  ASSERT(VALUE == ip.type, INT == ip.val.type);
  return (uint32_t) ip.val.int_val;
}

void vm_set_ip(VM *vm, Thread *t, uint32_t ip) {
  ASSERT_NOT_NULL(vm);
  Element block = vm_current_block(vm, t);
  ASSERT(OBJECT == block.type);
  memory_graph_set_field(vm->graph, block, IP_FIELD, create_int(ip));
}

Element vm_get_module(const VM *vm, const Thread *t) {
  ASSERT_NOT_NULL(vm);
  Element block = vm_current_block(vm, t);
  ASSERT(OBJECT == block.type);
  Element module = obj_get_field(block, MODULE_FIELD);
  ASSERT(OBJECT == module.type, MODULE == module.obj->type);
  return module;
}

void vm_throw_error(VM *vm, Thread *t, Ins ins, const char fmt[], ...) {
  fflush(stdout);
  fflush(stderr);
  va_list args;
  va_start(args, fmt);
  char buffer[1024];
  vsprintf(buffer, fmt, args);
  va_end(args);
  Element error_msg = string_create(vm, buffer);
  Element error_module = vm_lookup_module(vm, strings_intern("error"));
  Element error_class = obj_get_field(error_module, strings_intern("Error"));
  Element curr_block = vm_current_block(vm, t);

  // TODO: Why do I need to do this? It should automatically init the module.
  vm_maybe_initialize_and_execute(vm, t,
      vm_lookup_module(vm, strings_intern("io")).obj->module);

  memory_graph_set_field(vm->graph, curr_block, ERROR_KEY, create_int(1));
  vm_set_resval(vm, t, error_msg);
  vm_call_new(vm, t, error_class);
}

void catch_error(VM *vm, Thread *t) {
  Element curr_block = create_none();
  Element catch_goto = create_none();
  while (true) {
    curr_block = vm_current_block(vm, t);
    if (curr_block.type == NONE) {
      break;
    }
    catch_goto = obj_get_field(curr_block, strings_intern("$try_goto"));
    if (catch_goto.type != NONE) {
      break;
    }
    if (0 == Array_size(t->saved_blocks.obj->array)) {
      break;
    }
    if (!vm_back(vm, t)) {
      ERROR("HOW DID I GET HERE?");
      break;
    }
  }
  if (catch_goto.type == NONE) {
    Element error_module = vm_lookup_module(vm, strings_intern("error"));
    Element raise_error = obj_get_field(error_module,
        strings_intern("raise_error"));

    // TODO: Why do I need to do this? It should automatically init the module.
    vm_maybe_initialize_and_execute(vm, t,
        vm_lookup_module(vm, strings_intern("io")).obj->module);

    vm_call_fn(vm, t, error_module, raise_error);
    vm_shift_ip(vm, t, 1);
    return;
  }
  vm_set_ip(vm, t, catch_goto.val.int_val);
  memory_graph_set_field(vm->graph, vm_current_block(vm, t), ERROR_KEY,
      create_none());
}

Element vm_lookup_module(const VM *vm, const char module_name[]) {
  ASSERT(NOT_NULL(vm), NOT_NULL(module_name));
  Element module = obj_get_field(vm->modules, module_name);
  ASSERT(NONE != module.type);
  return module;
}

void vm_set_module(VM *vm, Thread *t, Element module_element, uint32_t ip) {
  ASSERT_NOT_NULL(vm);
  Element block = vm_current_block(vm, t);
  ASSERT(OBJECT == block.type);
  memory_graph_set_field(vm->graph, block, MODULE_FIELD, module_element);
  memory_graph_set_field(vm->graph, block, IP_FIELD, create_int(ip));
}

void vm_shift_ip(VM *vm, Thread *t, int num_ins) {
  ASSERT_NOT_NULL(vm);
  Element block = vm_current_block(vm, t);
  ASSERT(OBJECT == block.type);
  memory_graph_set_field(vm->graph, block, IP_FIELD,
      create_int(vm_get_ip(vm, t) + num_ins));
}

Ins vm_current_ins(const VM *vm, Thread *t) {
  ASSERT_NOT_NULL(vm);
  const Module *m = vm_get_module(vm, t).obj->module;
  uint32_t i = vm_get_ip(vm, t);
  ASSERT(NOT_NULL(m), i >= 0, i < module_size(m));
  return module_ins(m, i);
}

void vm_add_string_class(VM *vm) {
  merge_string_class(vm, class_string);
}

Element maybe_create_class_with_parents(VM *vm, Element module_element,
    const char class_name[]) {
  Expando *parents = map_lookup(
      module_class_parents(module_element.obj->module), class_name);
  Element class;

  if (NULL == parents) {
    if ((class = obj_get_field(vm->root, class_name)).type == NONE) {
      class = class_create(vm, class_name, class_object);
    }
  } else {
    Expando *parent_classes = expando(Object *, expando_len(parents));
    void get_parent(void *ptr) {
      Element parent = obj_get_field(module_element, *((char **) ptr));
      ASSERT(NONE != parent.type);
      expando_append(parent_classes, &parent.obj);
    }
    expando_iterate(parents, get_parent);

    if ((class = obj_get_field(vm->root, class_name)).type == NONE) {
      class = class_create_list(vm, class_name, parent_classes);
    } else {
      class_fill_list(vm, class, class_name, parent_classes);
    }
    expando_delete(parent_classes);
  }
  return class;
}

void vm_add_builtin(VM *vm) {
  Module *builtin = load_fn(strings_intern(BUILTIN_SRC), vm->store);
  ASSERT(NOT_NULL(vm), NOT_NULL(builtin));
  Element builtin_element = create_module(vm, builtin);
  memory_graph_set_field(vm->graph, vm->modules, module_name(builtin),
      builtin_element);
  memory_graph_set_field(vm->graph, builtin_element, PARENT, vm->root);
  memory_graph_set_field(vm->graph, builtin_element, INITIALIZED,
      element_false(vm));
  Element classes;
  memory_graph_set_field(vm->graph, builtin_element, CLASSES_KEY, classes =
      create_array(vm->graph));

  add_builtin_external(vm, builtin_element);
  add_global_builtin_external(vm, builtin_element);

  void add_ref(Pair *pair) {
    Element function = create_function(vm, builtin_element,
        (uint32_t) pair->value, pair->key);
    memory_graph_set_field(vm->graph, builtin_element, pair->key, function);
    memory_graph_set_field(vm->graph, vm->root, pair->key, function);
  }
  map_iterate(module_refs(builtin), add_ref);
  void add_class(const char class_name[], const Map *methods) {
    Element class = maybe_create_class_with_parents(vm, builtin_element,
        class_name);
    memory_graph_array_enqueue(vm->graph, classes, class);
    memory_graph_set_field(vm->graph, builtin_element, class_name, class);
    memory_graph_set_field(vm->graph, class, PARENT_MODULE, builtin_element);

    Element methods_array;
    if (NONE == (methods_array = obj_get_field(class, METHODS_KEY)).type) {
      memory_graph_set_field(vm->graph, class, METHODS_KEY, (methods_array =
          create_array(vm->graph)));
    }

    void add_method(Pair *pair2) {
      Element method = create_method(vm, builtin_element,
          (uint32_t) pair2->value, class, pair2->key);
      memory_graph_set_field(vm->graph, class, pair2->key, method);
      memory_graph_array_enqueue(vm->graph, methods_array, method);
    }
    map_iterate(methods, add_method);
  }
  module_iterate_classes(builtin, add_class);
//  map_iterate(module_classes(builtin), add_class);
}

Element vm_add_module(VM *vm, const Module *module) {
  ASSERT(NOT_NULL(vm), NOT_NULL(module));
  ASSERT(NONE == obj_get_field(vm->modules, module_name(module)).type);
  Element module_element = create_module(vm, module);
  memory_graph_set_field(vm->graph, vm->modules, module_name(module),
      module_element);
  memory_graph_set_field(vm->graph, module_element, PARENT, vm->root);
  memory_graph_set_field(vm->graph, module_element, INITIALIZED,
      element_false(vm));
  Element classes;
  memory_graph_set_field(vm->graph, module_element, CLASSES_KEY, classes =
      create_array(vm->graph));

  void add_ref(Pair *pair) {
    memory_graph_set_field(vm->graph, module_element, pair->key,
        create_function(vm, module_element, (uint32_t) pair->value, pair->key));
  }
  map_iterate(module_refs(module), add_ref);
  void add_class(const char class_name[], const Map *methods) {
    Element class = maybe_create_class_with_parents(vm, module_element,
        class_name);
    memory_graph_array_enqueue(vm->graph, classes, class);
    memory_graph_set_field(vm->graph, module_element, class_name, class);
    memory_graph_set_field(vm->graph, class, PARENT_MODULE, module_element);

    Element methods_array;
    if (NONE == (methods_array = obj_get_field(class, METHODS_KEY)).type) {
      memory_graph_set_field(vm->graph, class, METHODS_KEY, (methods_array =
          create_array(vm->graph)));
    }
    void add_method(Pair *pair2) {
      Element method = create_method(vm, module_element,
          (uint32_t) pair2->value, class, pair2->key);

      memory_graph_set_field(vm->graph, class, pair2->key, method);
      memory_graph_array_enqueue(vm->graph, methods_array, method);
    }
    map_iterate(methods, add_method);
  }
  module_iterate_classes(module, add_class);
//  map_iterate(module_classes(module), add_class);
  return module_element;
}

void vm_merge_module(VM *vm, const char fn[]) {
  Module *module = load_fn(strings_intern(fn), vm->store);
  ASSERT(NOT_NULL(vm), NOT_NULL(module));
  Element module_element = create_module(vm, module);
  memory_graph_set_field(vm->graph, vm->modules, module_name(module),
      module_element);
  memory_graph_set_field(vm->graph, module_element, PARENT, vm->root);
  memory_graph_set_field(vm->graph, module_element, INITIALIZED,
      element_false(vm));
  Element classes;
  memory_graph_set_field(vm->graph, module_element, CLASSES_KEY, classes =
      create_array(vm->graph));

  if (starts_with(fn, IO_SRC)) {
    add_io_external(vm, module_element);
  } else if (starts_with(fn, SYNC_SRC)) {
    add_sync_external(vm, module_element);
  }

  void add_ref(Pair *pair) {
    Element function = create_function(vm, module_element,
        (uint32_t) pair->value, pair->key);
    memory_graph_set_field(vm->graph, module_element, pair->key, function);
  }
  map_iterate(module_refs(module), add_ref);
  void add_class(Pair *pair) {
    Element class;
    if ((class = obj_get_field(vm->root, pair->key)).type == NONE) {
      class = class_create(vm, pair->key, class_object);
    }
    memory_graph_set_field(vm->graph, class, PARENT_MODULE, module_element);
    memory_graph_set_field(vm->graph, module_element, pair->key, class);

    memory_graph_array_enqueue(vm->graph, classes, class);

    Element methods_array;
    if (NONE == (methods_array = obj_get_field(class, METHODS_KEY)).type) {
      memory_graph_set_field(vm->graph, class, METHODS_KEY, (methods_array =
          create_array(vm->graph)));
    }

    void add_method(Pair *pair2) {
      Element method = create_method(vm, module_element,
          (uint32_t) pair2->value, class, pair2->key);
      memory_graph_set_field(vm->graph, class, pair2->key, method);
      memory_graph_array_enqueue(vm->graph, methods_array, method);

    }
    map_iterate(pair->value, add_method);
  }
  map_iterate(module_classes(module), add_class);
}

VM *vm_create(ArgStore *store) {
  VM *vm = ALLOC(VM);
  vm->debug_mutex = create_mutex(NULL);
  vm->module_init_mutex = create_mutex(NULL);
  vm->store = store;
  vm->graph = memory_graph_create();
  vm->root = memory_graph_create_root_element(vm->graph);
  class_init(vm);
  vm_add_string_class(vm);
// Complete the root object.
  fill_object_unsafe(vm->graph, vm->root, class_object);
  memory_graph_set_field(vm->graph, vm->root, ROOT, vm->root);
  memory_graph_set_field(vm->graph, vm->root, MODULES, (vm->modules =
      create_obj(vm->graph)));

  vm_add_builtin(vm);

  memory_graph_set_field(vm->graph, vm->root, NIL_KEYWORD, create_none());
  memory_graph_set_field(vm->graph, vm->root, FALSE_KEYWORD, create_none());
  memory_graph_set_field(vm->graph, vm->root, TRUE_KEYWORD, create_int(1));

  int i;
  for (i = 0; i < preloaded_size(); ++i) {
    vm_merge_module(vm, PRELOADED[i]);
  }
  return vm;
}

void vm_delete(VM *vm) {
  ASSERT_NOT_NULL(vm->graph);
  void delete_module(Pair *kv) {
    ASSERT(NOT_NULL(kv));
    Element *e = ((Element *) kv->value);
    if (e->type != OBJECT || e->obj->type != MODULE) {
      return;
    }
    Module *module = (Module*) e->obj->module;
    module_delete(module);
  }
  map_iterate(&vm->modules.obj->fields, delete_module);
  memory_graph_delete(vm->graph);
  vm->graph = NULL;
  close_mutex(vm->debug_mutex);
  close_mutex(vm->module_init_mutex);
  DEALLOC(vm);
}

void vm_pushstack(VM *vm, Thread *t, Element element) {
  ASSERT_NOT_NULL(vm);
  ASSERT(t->stack.type == OBJECT);
  ASSERT(t->stack.obj->type == ARRAY);
  ASSERT_NOT_NULL(t->stack.obj->array);
  memory_graph_array_enqueue(vm->graph, t->stack, element);
}

Element vm_popstack(VM *vm, Thread *t, bool *has_error) {
  ASSERT_NOT_NULL(vm);
  ASSERT(t->stack.type == OBJECT);
  ASSERT(t->stack.obj->type == ARRAY);
  ASSERT_NOT_NULL(t->stack.obj->array);
  if (Array_is_empty(t->stack.obj->array)) {
    *has_error = true;
//    vm_throw_error(vm, vm_current_ins(vm),
//        "Attempted to pop from the empty stack.");
    return create_none();
  }
  return memory_graph_array_dequeue(vm->graph, t->stack);
}

Element vm_peekstack(VM *vm, Thread *t, int distance) {
  ASSERT_NOT_NULL(vm);
  ASSERT(t->stack.type == OBJECT);
  ASSERT(t->stack.obj->type == ARRAY);
  Array *array = t->stack.obj->array;
  ASSERT_NOT_NULL(array);
  ASSERT(!Array_is_empty(array), (Array_size(array) - 1 - distance) >= 0);
  return Array_get(array, Array_size(array) - 1 - distance);
}

Element vm_new_block(VM *vm, Thread *t, Element parent, Element new_this) {
  ASSERT_NOT_NULL(vm);
  ASSERT(OBJECT == new_this.type);
  ASSERT_NOT_NULL(new_this.obj);
  Element old_block = vm_current_block(vm, t);
  // Save stack size for later for cleanup on ret.
  memory_graph_set_field(vm->graph, old_block, STACK_SIZE_NAME,
      create_int(Array_size(t->stack.obj->array)));

  memory_graph_array_push(vm->graph, t->saved_blocks, old_block);

  Element new_block = create_obj(vm->graph);

  Element module = vm_get_module(vm, t);
  uint32_t ip = vm_get_ip(vm, t);

  t->current_block = new_block;
  memory_graph_set_field(vm->graph, t->self, CURRENT_BLOCK, new_block);
  memory_graph_set_field(vm->graph, new_block, PARENT, parent);
  memory_graph_set_field(vm->graph, new_block, SELF, new_this);
  vm_set_module(vm, t, module, ip);
  return new_block;
}

// Returns false if there is an error.
bool vm_back(VM *vm, Thread *t) {
  ASSERT_NOT_NULL(vm);
  Element parent_block = memory_graph_array_pop(vm->graph, t->saved_blocks);
  // Remove accumulated stack.
  // TODO: Maybe consider an increased stack a bug in the future.
  Element old_stack_size = obj_get_field(parent_block, STACK_SIZE_NAME);
  bool has_error = false;
  if (NONE != old_stack_size.type) {
    ASSERT(ISVALUE(old_stack_size), INT == old_stack_size.val.type);
    int i;
    for (i = 0;
        i < Array_size(t->stack.obj->array) - old_stack_size.val.int_val; ++i) {
      vm_popstack(vm, t, &has_error);
      // TODO: Maybe return instead.
      if (has_error) {
        return false;
      }
    }
  }
  t->current_block = parent_block;
  memory_graph_set_field(vm->graph, t->self, CURRENT_BLOCK, parent_block);
  return true;
}

const MemoryGraph *vm_get_graph(const VM *vm) {
  return vm->graph;
}

Element vm_object_lookup(VM *vm, Element obj, const char name[]) {
  if (OBJECT != obj.type) {
    return create_none();
  }
  Element elt = obj_deep_lookup(obj.obj, name);

// Create MethodInstance for methods since Methods do not contain any Object
// state.
  if (ISTYPE(elt, class_method)
// Do not box methods if they are directly retrieved from a class.
      && !(ISCLASS(obj) && elt.obj == obj_get_field(obj, name).obj)
      && !ISTYPE(obj, class_methodinstance)
      && !ISTYPE(obj, class_method) && !ISTYPE(obj, class_function)
      && !ISTYPE(obj, class_external_function)) {
    return create_method_instance(vm->graph, obj, elt);
  }
  return elt;
}

Element vm_object_get(VM *vm, Thread *t, const char name[], bool *has_error) {
  Element resval = vm_get_resval(vm, t);
  if (OBJECT != resval.type) {
    vm_throw_error(vm, t, vm_current_ins(vm, t),
        "Cannot get field '%s' from Nil.", name);
    *has_error = true;
    return create_none();
//    ERROR("Cannot access field(%s) of Nil.", name);
  }
  return vm_object_lookup(vm, resval, name);
}

Element vm_lookup(VM *vm, Thread *t, const char name[]) {
//////DEBUGF("Looking for '%s'", name);
  Element block = vm_current_block(vm, t);
  Element lookup;
  while (NONE != block.type
      && NONE == (lookup = obj_get_field(block, name)).type) {
    block = obj_lookup(block.obj, CKey_parent);
    //////DEBUGF("Looking for '%s'", name);
  }

  if (NONE == block.type) {
    // Try checking the thread if all else fails.
    if (NONE != (lookup = obj_get_field(t->self, name)).type) {
      return lookup;
    }
    return create_none();
  }
  return lookup;
}

void vm_set_resval(VM *vm, Thread *t, const Element elt) {
//  elt_to_str(elt, stdout);
//  printf(" <-- " RESULT_VAL "\n");
//  fflush(stdout);
  memory_graph_set_field(vm->graph, t->self, RESULT_VAL, elt);
}

const Element vm_get_resval(VM *vm, Thread *t) {
  return obj_lookup(t->self.obj, CKey_resval);
}

const Element vm_get_old_resvals(VM *vm, Thread *t) {
  return obj_get_field(t->self, OLD_RESVALS);
}

void call_external_fn(VM *vm, Thread *t, Element obj, Element external_func) {
  if (!ISTYPE(external_func,
      class_external_function) && !ISTYPE(external_func, class_external_method)) {
    vm_throw_error(vm, t, vm_current_ins(vm, t),
        "Cannot call ExternalFunction on something not ExternalFunction.");
    return;
  }
  ASSERT(NOT_NULL(external_func.obj->external_fn));
  Element resval = vm_get_resval(vm, t);
  ExternalData *ed = (obj.obj->is_external) ? obj.obj->external_data : NULL;
  ExternalData tmp;
  if (NULL == ed) {
    tmp.object = obj;
    tmp.vm = vm;
    ed = &tmp;
  }
  Element returned = external_func.obj->external_fn(vm, t, ed, resval);
  vm_set_resval(vm, t, returned);
}

void call_function_internal(VM *vm, Thread *t, Element obj, Element func) {
  Element parent;
  if (ISTYPE(func, class_method)) {
    Object *function_object = *((Object **) expando_get(func.obj->parent_objs,
        0));
    parent = obj_get_field_obj(function_object, PARENT_MODULE);
  } else {
    parent = obj_get_field(func, PARENT_MODULE);
  }
  ASSERT(ISTYPE(parent, class_module));
//  DEBUGF("module_name=%s", string_to_cstr(obj_get_field(parent, NAME_KEY)));
//  elt_to_str(parent, stdout);
//  printf("\n");
//  fflush(stdout);

  vm_maybe_initialize_and_execute(vm, t, parent.obj->module);

  Element new_block = vm_new_block(vm, t, parent, obj);
  memory_graph_set_field(vm->graph, new_block, CALLER_KEY, func);
  vm_set_module(vm, t, parent,
      obj_deep_lookup(func.obj, INS_INDEX).val.int_val - 1);

}

void call_methodinstance(VM *vm, Thread *t, Element methodinstance) {
//  DEBUGF("call_methodinstance");
// self.method.call(self.obj)
//  vm_set_resval(vm, obj_get_field(methodinstance, OBJ_KEY));
//  call_function_internal(vm, obj_get_field(methodinstance, METHOD_KEY),
//      obj_get_field(class_method, CALL_KEY));
  call_function_internal(vm, t, obj_get_field(methodinstance, OBJ_KEY),
      obj_get_field(methodinstance, METHOD_KEY));
}

void vm_call_new(VM *vm, Thread *t, Element class) {
////DEBUGF("call_new");
  ASSERT(ISCLASS(class));
  Element new_obj;
  if (NONE != obj_get_field(class, IS_EXTERNAL_KEY).type) {
    new_obj = create_external_obj(vm, class);
  } else {
    new_obj = create_obj_of_class(vm->graph, class);
  }
  Element new_func = obj_get_field(class, CONSTRUCTOR_KEY);
  if (NONE != new_func.type) {
    vm_call_fn(vm, t, new_obj, new_func);
  } else {
    vm_maybe_initialize_and_execute(vm, t,
        obj_get_field(class, PARENT_MODULE).obj->module);
    vm_set_resval(vm, t, new_obj);
  }
}

void vm_call_fn(VM *vm, Thread *t, Element obj, Element func) {
//  DEBUGF("A");
//  elt_to_str(obj, stdout);
//  printf("\n");
//  elt_to_str(func, stdout);
//  printf("\n");
//  fflush(stdout);
  if (!ISTYPE(func, class_function) &&
  !ISTYPE(func, class_method) &&
  !ISTYPE(func, class_external_function) &&
  !ISTYPE(func, class_external_method) &&
  !ISTYPE(func, class_methodinstance)) {
    vm_throw_error(vm, t, vm_current_ins(vm, t),
        "Attempted to call something not a function of Class.");
    return;
  }

  if (ISTYPE(func,
      class_external_function) || ISTYPE(func, class_external_method)) {
    call_external_fn(vm, t, obj, func);
    return;
  }
  if (ISTYPE(func, class_methodinstance)) {
    call_methodinstance(vm, t, func);
    return;
  }
  call_function_internal(vm, t, obj, func);
}

void vm_to_string(const VM *vm, Element elt, FILE *target) {
  elt_to_str(elt, target);
}

void execute_object_operation(VM *vm, Thread * t, Element lhs, Element rhs,
    const char func_name[]) {
  Element args = create_tuple(vm->graph);
  memory_graph_tuple_add(vm->graph, args, lhs);
  memory_graph_tuple_add(vm->graph, args, rhs);
  Element builtin = vm_lookup_module(vm, BUILTIN_MODULE_NAME);
  ASSERT(NONE != builtin.type);
  Element fn = vm_object_lookup(vm, builtin, func_name);
  ASSERT(NONE != fn.type);
  vm_set_resval(vm, t, args);
  vm_call_fn(vm, t, builtin, fn);
}

bool execute_no_param(VM *vm, Thread *t, Ins ins) {
  Element elt, index, new_val, class, block;
  bool has_error = false;
  switch (ins.op) {
  case NOP:
    return true;
  case EXIT:
//    vm_set_resval(vm, vm_popstack(vm));
    fflush(stdout);
    fflush(stderr);
    return false;
  case RAIS:
    memory_graph_set_field(vm->graph, vm_current_block(vm, t), ERROR_KEY,
        create_int(1));
    catch_error(vm, t);
    // Counter shift forward at end of instruction execution.
    vm_shift_ip(vm, t, -1);
    return true;
  case PUSH:
    vm_pushstack(vm, t, (Element) vm_get_resval(vm, t));
    return true;
  case CALL:
    elt = vm_popstack(vm, t, &has_error);
    if (has_error) {
      return true;
    }
    if (ISOBJECT(elt)) {
      class = obj_lookup(elt.obj, CKey_class);
      if (inherits_from(class,
          class_function) || ISTYPE(elt, class_methodinstance)) {
        vm_call_fn(vm, t, vm_lookup(vm, t, SELF), elt);
      } else if (ISTYPE(elt, class_class)) {
        vm_call_new(vm, t, elt);
      }
    } else {
      vm_throw_error(vm, t, ins,
          "Cannot execute call something not a Function or Class.");
      return true;
    }
    return true;
  case ASET:
    elt = vm_popstack(vm, t, &has_error);
    if (has_error) {
      return true;
    }
    new_val = vm_popstack(vm, t, &has_error);
    if (has_error) {
      return true;
    }
    if (elt.is_const) {
      vm_throw_error(vm, t, ins, "Cannot modify const Object.");
      return true;
    }
    if (elt.type != OBJECT) {
      vm_throw_error(vm, t, ins,
          "Cannot perform array operation on something not an Object.");
      return true;
    }
    index = vm_get_resval(vm, t);
    if (elt.obj->type == ARRAY) {
      if (index.type != VALUE || index.val.type != INT) {
        vm_throw_error(vm, t, ins,
            "Cannot index an array with something not an int.");
        return true;
      }
      memory_graph_array_set(vm->graph, elt, index.val.int_val, new_val);
    } else {
      Element set_fn = vm_object_lookup(vm, elt, ARRAYLIKE_SET_KEY);
      if (NONE == set_fn.type) {
        vm_throw_error(vm, t, ins,
            "Cannot perform array operation on something not Arraylike.");
        return true;
      }
      Element args = create_tuple(vm->graph);
      memory_graph_tuple_add(vm->graph, args, index);
      memory_graph_tuple_add(vm->graph, args, new_val);
      vm_set_resval(vm, t, args);
      vm_call_fn(vm, t, elt, set_fn);
    }
    return true;
  case AIDX:
    elt = vm_popstack(vm, t, &has_error);
    if (has_error) {
      return true;
    }
    index = vm_get_resval(vm, t);
    if (elt.type != OBJECT) {
      vm_throw_error(vm, t, ins, "Indexing on something not Arraylike.");
      return true;
    }
    if (elt.obj->type == TUPLE || elt.obj->type == ARRAY) {
      if (index.type != VALUE || index.val.type != INT) {
        vm_throw_error(vm, t, ins, "Array indexing with something not an int.");
        return true;
      }
      if (elt.obj->type == TUPLE) {
        execute_tget(vm, t, ins, elt, index.val.int_val);
      } else {
        if (index.val.int_val < 0
            || index.val.int_val >= Array_size(elt.obj->array)) {
          vm_throw_error(vm, t, ins,
              "Array Index out of bounds. Index=%d, Array.len=%d.",
              index.val.int_val, Array_size(elt.obj->array));
          return true;
        }
        vm_set_resval(vm, t, Array_get(elt.obj->array, index.val.int_val));
      }
      return true;
    } else {
      Element index_fn = vm_object_lookup(vm, elt, ARRAYLIKE_INDEX_KEY);
      if (NONE == index_fn.type) {
        vm_throw_error(vm, t, ins,
            "Cannot perform array operation on something not Arraylike.");
        return true;
      }
      vm_set_resval(vm, t, index);
      vm_call_fn(vm, t, elt, index_fn);
    }
    return true;
  case RES:
    elt = vm_popstack(vm, t, &has_error);
    if (has_error) {
      return true;
    }
    vm_set_resval(vm, t, elt);
    return true;
  case PEEK:
    vm_set_resval(vm, t, vm_peekstack(vm, t, 0));
    return true;
  case DUP:
    elt = vm_peekstack(vm, t, 0);
    vm_pushstack(vm, t, elt);
    return true;
  case NBLK:
    vm_new_block(vm, t, vm_current_block(vm, t), vm_lookup(vm, t, SELF));
    memory_graph_set_field(vm->graph, vm_current_block(vm, t),
        IS_ITERATOR_BLOCK_KEY, create_int(1));
    return true;
  case BBLK:
    block = vm_current_block(vm, t);
    if (!vm_back(vm, t)) {
      vm_throw_error(vm, t, ins, "vm_back failed.");
      return true;
    }
    vm_set_ip(vm, t, obj_get_field(block, IP_FIELD).val.int_val);
    return true;
  case RET:
    // Clear all loops.
    while (NONE
        != obj_get_field(vm_current_block(vm, t), IS_ITERATOR_BLOCK_KEY).type) {
      if (!vm_back(vm, t)) {
        vm_throw_error(vm, t, ins, "vm_back failed.");
        return true;
      }
    }
    // Return to previous function call.
    if (!vm_back(vm, t)) {
      vm_throw_error(vm, t, ins, "vm_back (ret) failed.");
      return true;
    }
    return true;
  case PRNT:
    elt = vm_get_resval(vm, t);
    vm_to_string(vm, elt, stdout);
    if (DBG) {
      fflush(stdout);
    }
    return true;
  case ANEW:
    vm_set_resval(vm, t, create_array(vm->graph));
    return true;
  case NOTC:
    vm_set_resval(vm, t, operator_notc(vm_get_resval(vm, t)));
    return true;
  case NOT:
    vm_set_resval(vm, t, element_not(vm, vm_get_resval(vm, t)));
    return true;
  case ADR:
    elt = vm_get_resval(vm, t);
    if (OBJECT != elt.type) {
      vm_throw_error(vm, t, ins, "Cannot get the address of a non-object.");
      return true;
    }
    vm_set_resval(vm, t, create_int((int32_t) elt.obj));
    return true;
  case CNST:
    vm_set_resval(vm, t, make_const(vm_get_resval(vm, t)));
    return true;
  default:
    break;
  }

  Element res;
  Element rhs = vm_popstack(vm, t, &has_error);
  if (has_error) {
    return true;
  }
  Element lhs = vm_popstack(vm, t, &has_error);
  if (has_error) {
    return true;
  }
  switch (ins.op) {
  case ADD:
    if ( ISTYPE(lhs, class_string) && ISTYPE(rhs, class_string)) {
      res = string_add(vm, lhs, rhs);
      break;
    }
    res = operator_add(lhs, rhs);
    break;
  case SUB:
    res = operator_sub(lhs, rhs);
    break;
  case MULT:
    res = operator_mult(lhs, rhs);
    break;
  case DIV:
    res = operator_div(lhs, rhs);
    break;
  case MOD:
    res = operator_mod(lhs, rhs);
    break;
  case EQ:
//    DEBUGF("lhs.type=%d, rhs.type=%d", lhs.type, rhs.type);
    if (lhs.type == VALUE && rhs.type == VALUE) {
      res = operator_eq(vm, lhs, rhs);
      break;
    }
    execute_object_operation(vm, t, lhs, rhs, EQ_FN_NAME);
    return true;
  case NEQ:
//    DEBUGF("lhs.type=%d, rhs.type=%d", lhs.type, rhs.type);
    if (lhs.type == VALUE && rhs.type == VALUE) {
      res = operator_neq(vm, lhs, rhs);
      break;
    }
    execute_object_operation(vm, t, lhs, rhs, NEQ_FN_NAME);
    return true;
  case GT:
    res = operator_gt(vm, lhs, rhs);
    break;
  case LT:
    res = operator_lt(vm, lhs, rhs);
    break;
  case GTE:
    res = operator_gte(vm, lhs, rhs);
    break;
  case LTE:
    res = operator_lte(vm, lhs, rhs);
    break;
  case AND:
    res = operator_and(vm, lhs, rhs);
    break;
  case OR:
    res = operator_or(vm, lhs, rhs);
    break;
  case XOR:
    res = operator_or(vm, operator_and(vm, lhs, element_not(vm, rhs)),
        operator_and(vm, element_not(vm, lhs), rhs));
    break;
  case IS:
    if (!ISCLASS(rhs)) {
      vm_throw_error(vm, t, ins,
          "Cannot perfom type-check against a non-object type.");
      return true;
    }
    if (lhs.type != OBJECT) {
      res = element_false(vm);
      break;
    }
    class = obj_lookup(lhs.obj, CKey_class);
    if (inherits_from(class, rhs)) {
//      ////DEBUGF("TRUE");
      res = element_true(vm);
    } else {
//      ////DEBUGF("FALSE");
      res = element_false(vm);
    }
    break;
  default:
    DEBUGF("Weird op=%d", ins.op);
    vm_throw_error(vm, t, ins, "Instruction op was not a no_param.");
    return true;
  }
  vm_set_resval(vm, t, res);

  return true;
}

bool execute_id_param(VM *vm, Thread *t, Ins ins) {
  ASSERT_NOT_NULL(ins.str);
  Element block = vm_current_block(vm, t);
  Element module, resval, new_res_val, obj;
  int32_t ip;
  bool has_error = false;
  switch (ins.op) {
  case SET:
    if (is_const_ref(block.obj, ins.str)) {
      vm_throw_error(vm, t, ins, "Cannot reassign const reference.");
      return true;
    }
    memory_graph_set_field(vm->graph, block, ins.str, vm_get_resval(vm, t));
    break;
  case MDST:
    memory_graph_set_field(vm->graph, vm_get_module(vm, t), ins.str,
        vm_get_resval(vm, t));
    break;
  case CNST:
    make_const_ref(block.obj, ins.id);
    break;
  case SETC:
    if (is_const_ref(block.obj, ins.str)) {
      vm_throw_error(vm, t, ins, "Cannot reassign const reference.");
      return true;
    }
    memory_graph_set_field(vm->graph, block, ins.str, vm_get_resval(vm, t));
    make_const_ref(block.obj, ins.id);
    break;
  case FLD:
    resval = vm_get_resval(vm, t);
    if (resval.is_const) {
      vm_throw_error(vm, t, ins, "Cannot modify const Object.");
      return true;
    }
    new_res_val = vm_popstack(vm, t, &has_error);
    if (has_error) {
      return true;
    }
    memory_graph_set_field(vm->graph, resval, ins.str, new_res_val);
    vm_set_resval(vm, t, new_res_val);
    break;
  case PUSH:
    vm_pushstack(vm, t, vm_lookup(vm, t, ins.str));
    break;
  case PSRS:
    new_res_val = vm_lookup(vm, t, ins.str);
    vm_pushstack(vm, t, new_res_val);
    vm_set_resval(vm, t, new_res_val);
    break;
  case RES:
    vm_set_resval(vm, t, vm_lookup(vm, t, ins.str));
    break;
  case GET:
    new_res_val = vm_object_get(vm, t, ins.str, &has_error);
    if (!has_error) {
      vm_set_resval(vm, t, new_res_val);
    }
    break;
  case GTSH:
    vm_pushstack(vm, t, vm_object_get(vm, t, ins.str, &has_error));
    break;
  case INC:
    new_res_val = vm_object_get(vm, t, ins.str, &has_error);
    if (has_error) {
      return true;
    }
    if (VALUE != new_res_val.type) {
      vm_throw_error(vm, t, ins,
          "Cannot increment '%s' because it is not a value-type.", ins.str);
      return true;
    }
    switch (new_res_val.val.type) {
    case INT:
      new_res_val.val.int_val++;
      break;
    case FLOAT:
      new_res_val.val.float_val++;
      break;
    case CHAR:
      new_res_val.val.char_val++;
      break;
    }
    memory_graph_set_field(vm->graph, block, ins.str, new_res_val);
    vm_set_resval(vm, t, new_res_val);
    break;
  case DEC:
    new_res_val = vm_object_get(vm, t, ins.str, &has_error);
    if (has_error) {
      return true;
    }
    if (VALUE != new_res_val.type) {
      vm_throw_error(vm, t, ins,
          "Cannot increment '%s' because it is not a value-type.", ins.str);
      return true;
    }
    switch (new_res_val.val.type) {
    case INT:
      new_res_val.val.int_val--;
      break;
    case FLOAT:
      new_res_val.val.float_val--;
      break;
    case CHAR:
      new_res_val.val.char_val--;
      break;
    }
    memory_graph_set_field(vm->graph, block, ins.str, new_res_val);
    vm_set_resval(vm, t, new_res_val);
    break;
  case CALL:
    obj = vm_popstack(vm, t, &has_error);
    if (has_error) {
      return true;
    }
    if (obj.type == NONE) {
      vm_throw_error(vm, t, ins, "Cannot deference Nil.");
      return true;
    }
    if (!ISOBJECT(obj)) {
      vm_throw_error(vm, t, ins, "Cannot call a non-object.");
      return true;
    }
    Element target = vm_object_lookup(vm, obj, ins.str);
    if (OBJECT != target.type) {
      vm_throw_error(vm, t, ins, "Object has no such function '%s'.", ins.str);
      return true;
    }
    if (inherits_from(obj_lookup(target.obj, CKey_class),
        class_function) || ISTYPE(target, class_methodinstance)) {
      vm_call_fn(vm, t, obj, target);
    } else if (ISTYPE(target, class_class)) {
      vm_call_new(vm, t, target);
    } else {
      vm_throw_error(vm, t, ins,
          "Cannot execute call something not a Function or Class.");
      return true;
    }
    break;
  case RMDL:
    module = vm_lookup_module(vm, ins.str);
    vm_set_resval(vm, t, module);
    // TODO: Why do I need this for interpreter mode?
    memory_graph_set_field(vm->graph, block, ins.str, module);
    break;
  case MCLL:
    module = vm_popstack(vm, t, &has_error);
    if (has_error) {
      return true;
    }
    ASSERT(OBJECT == module.type, MODULE == module.obj->type)
    ;
    vm_new_block(vm, t, block, block);
    ip = module_ref(module.obj->module, ins.str);
    ASSERT(ip > 0);
    vm_set_module(vm, t, module, ip - 1);
    break;
  case PRNT:
    elt_to_str(vm_lookup(vm, t, ins.str), stdout);
    if (DBG) {
      fflush(stdout);
    }
    break;
  default:
    ERROR("Instruction op was not a id_param");
  }
  return true;
}

void execute_tget(VM *vm, Thread *t, Ins ins, Element tuple, int64_t index) {
  if (tuple.type != OBJECT || tuple.obj->type != TUPLE) {
    vm_throw_error(vm, t, ins, "Attempted to index something not a tuple.");
    return;
  }
  if (index < 0 || index >= tuple_size(tuple.obj->tuple)) {
    vm_throw_error(vm, t, ins,
        "Tuple Index out of bounds. Index=%d, Tuple.len=%d.", index,
        tuple_size(tuple.obj->tuple));
    return;
  }
  vm_set_resval(vm, t, tuple_get(tuple.obj->tuple, index));
}

bool execute_val_param(VM *vm, Thread *t, Ins ins) {
  Element elt = val_to_elt(ins.val), popped;
  Element tuple, array;
  bool has_error = false;
  int i;
  switch (ins.op) {
  case EXIT:
    vm_set_resval(vm, t, elt);
    return false;
  case RES:
    vm_set_resval(vm, t, elt);
    break;
  case PUSH:
    vm_pushstack(vm, t, elt);
    break;
  case PEEK:
    vm_set_resval(vm, t, vm_peekstack(vm, t, elt.val.int_val));
    break;
  case SINC:
    popped = vm_popstack(vm, t, &has_error);
    if (has_error) {
      return true;
    }
    vm_pushstack(vm, t, create_int(popped.val.int_val + elt.val.int_val));
    break;
  case TUPL:
    ASSERT(elt.type == VALUE, elt.val.type == INT)
    ;
    tuple = create_tuple(vm->graph);
    vm_set_resval(vm, t, tuple);
    for (i = 0; i < elt.val.int_val; i++) {
      popped = vm_popstack(vm, t, &has_error);
      if (has_error) {
        return true;
      }
      memory_graph_tuple_add(vm->graph, tuple, popped);
    }
    break;
  case TGET:
    if (elt.type != VALUE || elt.val.type != INT) {
      vm_throw_error(vm, t, ins,
          "Attempted to index a tuple with something not an int.");
      return true;
    }
    tuple = vm_get_resval(vm, t);
    execute_tget(vm, t, ins, tuple, elt.val.int_val);
    break;
  case JMP:
    ASSERT(elt.type == VALUE, elt.val.type == INT)
    ;
    vm_shift_ip(vm, t, elt.val.int_val);
    break;
  case IF:
    ASSERT(elt.type == VALUE, elt.val.type == INT)
    ;
    if (NONE != vm_get_resval(vm, t).type) {
      vm_shift_ip(vm, t, elt.val.int_val);
    }
    break;
  case IFN:
    ASSERT(elt.type == VALUE, elt.val.type == INT)
    ;
    if (NONE == vm_get_resval(vm, t).type) {
      vm_shift_ip(vm, t, elt.val.int_val);
    }
    break;
  case PRNT:
    vm_to_string(vm, elt, stdout);
    if (DBG) {
      fflush(stdout);
    }
    break;
  case ANEW:
    ASSERT(elt.type == VALUE, elt.val.type == INT)
    ;
    array = create_array(vm->graph);
    vm_set_resval(vm, t, array);
    for (i = 0; i < elt.val.int_val; i++) {
      popped = vm_popstack(vm, t, &has_error);
      if (has_error) {
        return true;
      }
      memory_graph_array_enqueue(vm->graph, array, popped);
    }
    break;
  case CTCH:
    ASSERT(elt.type == VALUE, elt.val.type == INT)
    ;
    uint32_t ip = vm_get_ip(vm, t);
    memory_graph_set_field(vm->graph, vm_current_block(vm, t),
        strings_intern("$try_goto"), create_int(ip + elt.val.int_val + 1));
    break;
  default:
    ERROR("Instruction op was not a val_param. op=%s",
        instructions[(int ) ins.op]);
  }
  return true;
}

bool execute_str_param(VM *vm, Thread *t, Ins ins) {
  Element str_array = string_create(vm, ins.str);
  switch (ins.op) {
  case PUSH:
    vm_pushstack(vm, t, str_array);
    break;
  case RES:
    vm_set_resval(vm, t, str_array);
    break;
  case PRNT:
    elt_to_str(str_array, stdout);
    if (DBG) {
      fflush(stdout);
    }
    break;
  case PSRS:
    vm_pushstack(vm, t, str_array);
    vm_set_resval(vm, t, str_array);
    break;
  default:
    ERROR("Instruction op was not a str_param. op=%s",
        instructions[(int ) ins.op]);
  }
  return true;
}

// returns whether or not the program should continue
bool execute(VM *vm, Thread *t) {
  ASSERT_NOT_NULL(vm);

  Ins ins = vm_current_ins(vm, t);

//  wait_for_mutex(vm->debug_mutex, INFINITE);
//  fprintf(stdout, "module(%s,t=%d) ",
//      module_name(vm_get_module(vm, t).obj->module), (int) t->id);
//  fflush(stdout);
//  ins_to_str(ins, stdout);
//  fprintf(stdout, "\n");
//  fflush(stdout);
//  release_mutex(vm->debug_mutex);

  bool status;
  switch (ins.param) {
  case ID_PARAM:
    status = execute_id_param(vm, t, ins);
    break;
  case VAL_PARAM:
    status = execute_val_param(vm, t, ins);
    break;
  case STR_PARAM:
    status = execute_str_param(vm, t, ins);
    break;
  default:
    status = execute_no_param(vm, t, ins);
  }
  if (NONE != obj_get_field(vm_current_block(vm, t), ERROR_KEY).type) {
//    // TODO: Does this need to be a loop?
//    while (NONE != obj_get_field(vm_current_block(vm, t), ERROR_KEY).type) {
    catch_error(vm, t);
//    }
    return true;
  }

  vm_shift_ip(vm, t, 1);

  return status;
}

void vm_maybe_initialize_and_execute(VM *vm, Thread *t, const Module *module) {
  Element module_element = vm_lookup_module(vm, module_name(module));
  wait_for_mutex(vm->module_init_mutex, INFINITE);
  if (is_true(obj_get_field(module_element, INITIALIZED))) {
    release_mutex(vm->module_init_mutex);
    return;
  }
  memory_graph_set_field(vm->graph, module_element, INITIALIZED,
      element_true(vm));

  memory_graph_array_push(vm->graph, vm_get_old_resvals(vm, t),
      vm_get_resval(vm, t));

  ASSERT(NONE != module_element.type);
  vm_new_block(vm, t, module_element, module_element);
  vm_set_module(vm, t, module_element, 0);
//  int i = 0;
  while (execute(vm, t)) {
//    ////DEBUGF("EXECUTING INSTRUCTION #%d", i);
//    if (0 == ((i+1) % 100)) {
//      ////DEBUGF("FREEING SPACE YAY (%d)", i);
//      memory_graph_free_space(vm->graph);
//      ////DEBUGF("DONE FREEING SPACE", i);
//    }
//    i++;
  }
  vm_back(vm, t);

  vm_set_resval(vm, t,
      memory_graph_array_pop(vm->graph, vm_get_old_resvals(vm, t)));

  release_mutex(vm->module_init_mutex);
}

void vm_start_execution(VM *vm, Element module) {
  Element main_function = create_function(vm, module, 0,
      strings_intern("main"));
  // Set to true so we don't double-run it.
  memory_graph_set_field(vm->graph, module, INITIALIZED, element_true(vm));
  Element thread = create_thread_object(vm, main_function, create_none());

  thread_start(Thread_extract(thread));
}
