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
#include "threads/sync.h"
#include "threads/thread.h"
#include "vm/preloaded_modules.h"

void vm_to_string(const VM *vm, Element elt, FILE *target);
void execute_tget(VM *vm, Thread *t, Ins ins, Element tuple, int64_t index);

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
  Element curr_block = t_current_block(t);

  // TODO: Why do I need to do this? It should automatically init the module.
  vm_maybe_initialize_and_execute(vm, t,
                                  vm_lookup_module(vm, strings_intern("io")));

  memory_graph_set_field(vm->graph, curr_block, ERROR_KEY, create_int(1));
  t_set_resval(t, error_msg);
  vm_call_new(vm, t, error_class);
}

void catch_error(VM *vm, Thread *t) {
  Element curr_block = create_none();
  Element catch_goto = create_none();
  while (true) {
    curr_block = t_current_block(t);
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
    if (!t_back(t)) {
      ERROR("HOW DID I GET HERE?");
      break;
    }
  }
  if (catch_goto.type == NONE) {
    Element error_module = vm_lookup_module(vm, strings_intern("error"));
    Element raise_error =
        obj_get_field(error_module, strings_intern("raise_error"));

    // TODO: Why do I need to do this? It should automatically init the module.
    vm_maybe_initialize_and_execute(vm, t,
                                    vm_lookup_module(vm, strings_intern("io")));

    vm_call_fn(vm, t, error_module, raise_error);
    t_shift_ip(t, 1);
    return;
  }
  t_set_ip(t, catch_goto.val.int_val);
  memory_graph_set_field(vm->graph, t_current_block(t), ERROR_KEY,
                         create_none());
  fflush(stderr);
}

Element vm_lookup_module(const VM *vm, const char module_name[]) {
  ASSERT(NOT_NULL(vm), NOT_NULL(module_name));
  Element module = obj_get_field(vm->modules, module_name);
  ASSERT(NONE != module.type);
  return module;
}

void vm_add_string_class(VM *vm) { merge_string_class(vm, class_string); }

Element maybe_create_class_with_parents(VM *vm, Element module_element,
                                        const char class_name[]) {
  Expando *parents =
      map_lookup(module_class_parents(module_element.obj->module), class_name);
  Element class;

  if (NULL == parents) {
    if ((class = obj_get_field(vm->root, class_name)).type == NONE) {
      class = class_create(vm, class_name, class_object);
    }
  } else {
    Expando *parent_classes = expando(Object *, expando_len(parents));
    void get_parent(void *ptr) {
      Element parent = obj_get_field(module_element, *((char **)ptr));
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
  memory_graph_set_field(vm->graph, builtin_element, CLASSES_KEY,
                         classes = create_array(vm->graph));

  maybe_merge_existing_source(vm, builtin_element, BUILTIN_SRC);

  void add_ref(Pair * pair) {
    Element function =
        create_function(vm, builtin_element, (uint32_t)pair->value, pair->key);
    memory_graph_set_field(vm->graph, builtin_element, pair->key, function);
    memory_graph_set_field(vm->graph, vm->root, pair->key, function);
  }
  map_iterate(module_refs(builtin), add_ref);

  void add_class(const char class_name[], const Map *methods) {
    Element class =
        maybe_create_class_with_parents(vm, builtin_element, class_name);
    memory_graph_array_enqueue(vm->graph, classes, class);
    memory_graph_set_field(vm->graph, builtin_element, class_name, class);
    memory_graph_set_field(vm->graph, class, PARENT_MODULE, builtin_element);

    Element methods_array;
    if (NONE == (methods_array = obj_get_field(class, METHODS_KEY)).type) {
      memory_graph_set_field(vm->graph, class, METHODS_KEY,
                             (methods_array = create_array(vm->graph)));
    }

    void add_method(Pair * pair2) {
      Element method = create_method(vm, builtin_element,
                                     (uint32_t)pair2->value, class, pair2->key);
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
  memory_graph_set_field(vm->graph, module_element, CLASSES_KEY,
                         classes = create_array(vm->graph));

  void add_ref(Pair * pair) {
    memory_graph_set_field(
        vm->graph, module_element, pair->key,
        create_function(vm, module_element, (uint32_t)pair->value, pair->key));
  }
  map_iterate(module_refs(module), add_ref);
  void add_class(const char class_name[], const Map *methods) {
    Element class =
        maybe_create_class_with_parents(vm, module_element, class_name);
    memory_graph_array_enqueue(vm->graph, classes, class);
    memory_graph_set_field(vm->graph, module_element, class_name, class);
    memory_graph_set_field(vm->graph, class, PARENT_MODULE, module_element);

    Element methods_array;
    if (NONE == (methods_array = obj_get_field(class, METHODS_KEY)).type) {
      memory_graph_set_field(vm->graph, class, METHODS_KEY,
                             (methods_array = create_array(vm->graph)));
    }
    void add_method(Pair * pair2) {
      Element method = create_method(vm, module_element, (uint32_t)pair2->value,
                                     class, pair2->key);

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
  memory_graph_set_field(vm->graph, module_element, CLASSES_KEY,
                         classes = create_array(vm->graph));

  maybe_merge_existing_source(vm, module_element, fn);

  void add_ref(Pair * pair) {
    Element function =
        create_function(vm, module_element, (uint32_t)pair->value, pair->key);
    memory_graph_set_field(vm->graph, module_element, pair->key, function);
  }
  map_iterate(module_refs(module), add_ref);

  void add_class(const char class_name[], const Map *methods) {
    Element class;
    if ((class = obj_get_field(vm->root, class_name)).type == NONE) {
      class = maybe_create_class_with_parents(vm, module_element, class_name);
    }
    memory_graph_set_field(vm->graph, module_element, class_name, class);
    memory_graph_set_field(vm->graph, class, PARENT_MODULE, module_element);
    memory_graph_array_enqueue(vm->graph, classes, class);

    Element methods_array;
    if (NONE == (methods_array = obj_get_field(class, METHODS_KEY)).type) {
      memory_graph_set_field(vm->graph, class, METHODS_KEY,
                             (methods_array = create_array(vm->graph)));
    }

    void add_method(Pair * pair2) {
      Element method = create_method(vm, module_element, (uint32_t)pair2->value,
                                     class, pair2->key);
      memory_graph_set_field(vm->graph, class, pair2->key, method);
      memory_graph_array_enqueue(vm->graph, methods_array, method);
    }
    map_iterate(methods, add_method);
  }
  module_iterate_classes(module, add_class);
}

VM *vm_create(ArgStore *store) {
  VM *vm = ALLOC(VM);
  vm->debug_mutex = mutex_create(NULL);
  vm->module_init_mutex = mutex_create(NULL);
  vm->store = store;
  vm->graph = memory_graph_create();
  vm->root = memory_graph_create_root_element(vm->graph);
  class_init(vm);
  vm_add_string_class(vm);
  // Complete the root object.
  fill_object_unsafe(vm->graph, vm->root, class_object);
  memory_graph_set_field(vm->graph, vm->root, ROOT, vm->root);
  memory_graph_set_field(vm->graph, vm->root, MODULES,
                         (vm->modules = create_obj(vm->graph)));

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
  void delete_module(Pair * kv) {
    ASSERT(NOT_NULL(kv));
    ElementContainer *e = ((ElementContainer *)kv->value);
    if (e->elt.type != OBJECT || e->elt.obj->type != MODULE) {
      return;
    }
    Module *module = (Module *)e->elt.obj->module;
    module_delete(module);
  }
  map_iterate(&vm->modules.obj->fields, delete_module);
  memory_graph_delete(vm->graph);
  vm->graph = NULL;
  mutex_close(vm->debug_mutex);
  mutex_close(vm->module_init_mutex);
  DEALLOC(vm);
}

const MemoryGraph *vm_get_graph(const VM *vm) { return vm->graph; }

Element vm_object_lookup(VM *vm, Element obj, const char name[]) {
  if (OBJECT != obj.type) {
    return create_none();
  }
  Element elt = obj_deep_lookup(obj.obj, name);

  // Create MethodInstance for methods since Methods do not contain any Object
  // state.
  if (ISTYPE(elt, class_method)
      // Do not box methods if they are directly retrieved from a class.
      && !(ISCLASS(obj) && elt.obj == obj_get_field(obj, name).obj) &&
      !ISTYPE(obj, class_methodinstance) && !ISTYPE(obj, class_method) &&
      !ISTYPE(obj, class_function) && !ISTYPE(obj, class_external_function)) {
    return create_method_instance(vm->graph, obj, elt);
  }
  return elt;
}

Element vm_object_get(VM *vm, Thread *t, const char name[], bool *has_error) {
  Element resval = t_get_resval(t);
  if (OBJECT != resval.type) {
    vm_throw_error(vm, t, t_current_ins(t), "Cannot get field '%s' from Nil.",
                   name);
    *has_error = true;
    return create_none();
    //    ERROR("Cannot access field(%s) of Nil.", name);
  }
  return vm_object_lookup(vm, resval, name);
}

Element vm_lookup(VM *vm, Thread *t, const char name[]) {
  //////DEBUGF("Looking for '%s'", name);
  Element block = t_current_block(t);
  Element lookup;
  while (NONE != block.type &&
         NONE == (lookup = obj_get_field(block, name)).type) {
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

const Element get_old_resvals(Thread *t) {
  return obj_get_field(t->self, OLD_RESVALS);
}

void call_external_fn(VM *vm, Thread *t, Element obj, Element external_func) {
  if (!ISTYPE(external_func, class_external_function) &&
      !ISTYPE(external_func, class_external_method)) {
    vm_throw_error(
        vm, t, t_current_ins(t),
        "Cannot call ExternalFunction on something not ExternalFunction.");
    return;
  }

  Element module = vm_object_lookup(vm, external_func, PARENT_MODULE);
  if (NONE != module.type) {
    vm_maybe_initialize_and_execute(vm, t, module);
  }

  ASSERT(NOT_NULL(external_func.obj->external_fn));
  Element resval = t_get_resval(t);
  ExternalData *ed = (obj.obj->is_external) ? obj.obj->external_data : NULL;
  ExternalData tmp;
  if (NULL == ed) {
    tmp.object = obj;
    tmp.vm = vm;
    ed = &tmp;
  }
  Element returned = external_func.obj->external_fn(vm, t, ed, resval);
  t_set_resval(t, returned);
}

void call_function_internal(VM *vm, Thread *t, Element obj, Element func) {
  Element parent;
  if (ISTYPE(func, class_method)) {
    Object *function_object =
        *((Object **)expando_get(func.obj->parent_objs, 0));
    parent = obj_get_field_obj(function_object, PARENT_MODULE);
  } else {
    parent = obj_get_field(func, PARENT_MODULE);
  }
  //  DEBUGF("module_name=%s", string_to_cstr(obj_get_field(parent, NAME_KEY)));
  //  elt_to_str(parent, stdout);
  //  printf("\n");
  //  fflush(stdout);

  vm_maybe_initialize_and_execute(vm, t, parent);

  Element new_block = t_new_block(t, parent, obj);
  memory_graph_set_field(vm->graph, new_block, CALLER_KEY, func);
  t_set_module(t, parent, obj_deep_lookup(func.obj, INS_INDEX).val.int_val - 1);
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
    vm_maybe_initialize_and_execute(vm, t, obj_get_field(class, PARENT_MODULE));
    t_set_resval(t, new_obj);
  }
}

void vm_call_fn(VM *vm, Thread *t, Element obj, Element func) {
  if (!ISTYPE(func, class_function) && !ISTYPE(func, class_method) &&
      !ISTYPE(func, class_external_function) &&
      !ISTYPE(func, class_external_method) &&
      !ISTYPE(func, class_methodinstance)) {
    vm_throw_error(vm, t, t_current_ins(t),
                   "Attempted to call something not a function of Class.");
    return;
  }

  if (ISTYPE(func, class_external_function) ||
      ISTYPE(func, class_external_method)) {
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

void execute_object_operation(VM *vm, Thread *t, Element lhs, Element rhs,
                              const char func_name[]) {
  Element args = create_tuple(vm->graph);
  memory_graph_tuple_add(vm->graph, args, lhs);
  memory_graph_tuple_add(vm->graph, args, rhs);
  Element builtin = vm_lookup_module(vm, BUILTIN_MODULE_NAME);
  ASSERT(NONE != builtin.type);
  Element fn = vm_object_lookup(vm, builtin, func_name);
  ASSERT(NONE != fn.type);
  t_set_resval(t, args);
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
      memory_graph_set_field(vm->graph, t_current_block(t), ERROR_KEY,
                             create_int(1));
      catch_error(vm, t);
      // Counter shift forward at end of instruction execution.
      //      t_shift_ip(t, -2);
      return true;
    case PUSH:
      t_pushstack(t, (Element)t_get_resval(t));
      return true;
    case CALL:
      elt = t_popstack(t, &has_error);
      if (has_error) {
        return true;
      }
      if (ISOBJECT(elt)) {
        class = obj_lookup(elt.obj, CKey_class);
        if (inherits_from(class, class_function) ||
            ISTYPE(elt, class_methodinstance)) {
          vm_call_fn(vm, t, vm_lookup(vm, t, SELF), elt);
        } else if (ISTYPE(elt, class_class)) {
          vm_call_new(vm, t, elt);
        }
      } else {
        vm_throw_error(
            vm, t, ins,
            "Cannot execute call something not a Function or Class.");
        return true;
      }
      return true;
    case ASET:
      elt = t_popstack(t, &has_error);
      if (has_error) {
        return true;
      }
      new_val = t_popstack(t, &has_error);
      if (has_error) {
        return true;
      }
      if (elt.obj->is_const) {
        vm_throw_error(vm, t, ins, "Cannot modify const Object.");
        return true;
      }
      if (elt.type != OBJECT) {
        vm_throw_error(
            vm, t, ins,
            "Cannot perform array operation on something not an Object.");
        return true;
      }
      index = t_get_resval(t);
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
          vm_throw_error(
              vm, t, ins,
              "Cannot perform array operation on something not Arraylike.");
          return true;
        }
        Element args = create_tuple(vm->graph);
        memory_graph_tuple_add(vm->graph, args, index);
        memory_graph_tuple_add(vm->graph, args, new_val);
        t_set_resval(t, args);
        vm_call_fn(vm, t, elt, set_fn);
      }
      return true;
    case AIDX:
      elt = t_popstack(t, &has_error);
      if (has_error) {
        return true;
      }
      index = t_get_resval(t);
      if (elt.type != OBJECT) {
        vm_throw_error(vm, t, ins, "Indexing on something not Arraylike.");
        return true;
      }
      if (elt.obj->type == TUPLE || elt.obj->type == ARRAY) {
        if (index.type != VALUE || index.val.type != INT) {
          vm_throw_error(vm, t, ins,
                         "Array indexing with something not an int.");
          return true;
        }
        if (elt.obj->type == TUPLE) {
          execute_tget(vm, t, ins, elt, index.val.int_val);
        } else {
          if (index.val.int_val < 0 ||
              index.val.int_val >= Array_size(elt.obj->array)) {
            vm_throw_error(vm, t, ins,
                           "Array Index out of bounds. Index=%d, Array.len=%d.",
                           index.val.int_val, Array_size(elt.obj->array));
            return true;
          }
          t_set_resval(t, Array_get(elt.obj->array, index.val.int_val));
        }
        return true;
      } else {
        Element index_fn = vm_object_lookup(vm, elt, ARRAYLIKE_INDEX_KEY);
        if (NONE == index_fn.type) {
          vm_throw_error(
              vm, t, ins,
              "Cannot perform array operation on something not Arraylike.");
          return true;
        }
        t_set_resval(t, index);
        vm_call_fn(vm, t, elt, index_fn);
      }
      return true;
    case RES:
      elt = t_popstack(t, &has_error);
      if (has_error) {
        return true;
      }
      t_set_resval(t, elt);
      return true;
    case PEEK:
      t_set_resval(t, t_peekstack(t, 0));
      return true;
    case DUP:
      elt = t_peekstack(t, 0);
      t_pushstack(t, elt);
      return true;
    case NBLK:
      t_new_block(t, t_current_block(t), vm_lookup(vm, t, SELF));
      memory_graph_set_field(vm->graph, t_current_block(t),
                             IS_ITERATOR_BLOCK_KEY, create_int(1));
      return true;
    case BBLK:
      block = t_current_block(t);
      if (!t_back(t)) {
        vm_throw_error(vm, t, ins, "vm_back failed.");
        return true;
      }
      t_set_ip(t, obj_get_field(block, IP_FIELD).val.int_val);
      return true;
    case RET:
      // Clear all loops.
      while (NONE !=
             obj_get_field(t_current_block(t), IS_ITERATOR_BLOCK_KEY).type) {
        if (!t_back(t)) {
          vm_throw_error(vm, t, ins, "vm_back failed.");
          return true;
        }
      }
      // Return to previous function call.
      if (!t_back(t)) {
        vm_throw_error(vm, t, ins, "vm_back (ret) failed.");
        return true;
      }
      return true;
    case PRNT:
      elt = t_get_resval(t);
      vm_to_string(vm, elt, stdout);
      if (DBG) {
        fflush(stdout);
      }
      return true;
    case ANEW:
      t_set_resval(t, create_array(vm->graph));
      return true;
    case NOTC:
      t_set_resval(t, operator_notc(t_get_resval(t)));
      return true;
    case NOT:
      t_set_resval(t, element_not(vm, t_get_resval(t)));
      return true;
    case ADR:
      elt = t_get_resval(t);
      if (OBJECT != elt.type) {
        vm_throw_error(vm, t, ins, "Cannot get the address of a non-object.");
        return true;
      }
      t_set_resval(t, create_int((int32_t)elt.obj));
      return true;
    case CNST:
      t_set_resval(t, make_const(t_get_resval(t)));
      return true;
    case TLEN:
      elt = t_peekstack(t, 0);
      // TODO: Not sure if this is the best solution
      if (!is_object_type(&elt, TUPLE)) {
        t_set_resval(t, create_int(-1));
        return true;
      }
      t_set_resval(t, create_int(tuple_size(elt.obj->tuple)));
      return true;
    default:
      break;
  }

  Element res;
  Element rhs = t_popstack(t, &has_error);
  if (has_error) {
    return true;
  }
  Element lhs = t_popstack(t, &has_error);
  if (has_error) {
    return true;
  }
  switch (ins.op) {
    case ADD:
      if (ISTYPE(lhs, class_string) && ISTYPE(rhs, class_string)) {
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
  t_set_resval(t, res);

  return true;
}

bool execute_id_param(VM *vm, Thread *t, Ins ins) {
  ASSERT_NOT_NULL(ins.str);
  Element block = t_current_block(t);
  Element module, resval, new_res_val, obj;
  int32_t ip;
  bool has_error = false;
  switch (ins.op) {
    case SET:
      if (is_const_ref(block.obj, ins.str)) {
        vm_throw_error(vm, t, ins, "Cannot reassign const reference.");
        return true;
      }
      memory_graph_set_field(vm->graph, block, ins.str, t_get_resval(t));
      break;
    case MDST:
      memory_graph_set_field(vm->graph, t_get_module(t), ins.str,
                             t_get_resval(t));
      break;
    case CNST:
      make_const_ref(block.obj, ins.id);
      break;
    case SETC:
      if (is_const_ref(block.obj, ins.str)) {
        vm_throw_error(vm, t, ins, "Cannot reassign const reference.");
        return true;
      }
      memory_graph_set_field(vm->graph, block, ins.str, t_get_resval(t));
      make_const_ref(block.obj, ins.id);
      break;
    case FLD:
      resval = t_get_resval(t);
      ASSERT(resval.type == OBJECT);
      if (resval.obj->is_const) {
        vm_throw_error(vm, t, ins, "Cannot modify const Object.");
        return true;
      }
      new_res_val = t_popstack(t, &has_error);
      if (has_error) {
        return true;
      }
      memory_graph_set_field(vm->graph, resval, ins.str, new_res_val);
      t_set_resval(t, new_res_val);
      break;
    case PUSH:
      t_pushstack(t, vm_lookup(vm, t, ins.str));
      break;
    case PSRS:
      new_res_val = vm_lookup(vm, t, ins.str);
      t_pushstack(t, new_res_val);
      t_set_resval(t, new_res_val);
      break;
    case RES:
      t_set_resval(t, vm_lookup(vm, t, ins.str));
      break;
    case GET:
      new_res_val = vm_object_get(vm, t, ins.str, &has_error);
      if (!has_error) {
        t_set_resval(t, new_res_val);
      }
      break;
    case GTSH:
      t_pushstack(t, vm_object_get(vm, t, ins.str, &has_error));
      break;
    case INC:
      new_res_val = vm_object_get(vm, t, ins.str, &has_error);
      if (has_error) {
        return true;
      }
      if (VALUE != new_res_val.type) {
        vm_throw_error(vm, t, ins,
                       "Cannot increment '%s' because it is not a value-type.",
                       ins.str);
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
      t_set_resval(t, new_res_val);
      break;
    case DEC:
      new_res_val = vm_object_get(vm, t, ins.str, &has_error);
      if (has_error) {
        return true;
      }
      if (VALUE != new_res_val.type) {
        vm_throw_error(vm, t, ins,
                       "Cannot increment '%s' because it is not a value-type.",
                       ins.str);
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
      t_set_resval(t, new_res_val);
      break;
    case CALL:
      obj = t_popstack(t, &has_error);
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
        elt_to_str(obj, stderr);
        fprintf(stderr, "\n");
        fflush(stderr);
        vm_throw_error(vm, t, ins, "Object has no such function '%s'.",
                       ins.str);
        return true;
      }
      if (inherits_from(obj_lookup(target.obj, CKey_class), class_function) ||
          ISTYPE(target, class_methodinstance)) {
        vm_call_fn(vm, t, obj, target);
      } else if (ISTYPE(target, class_class)) {
        vm_call_new(vm, t, target);
      } else {
        vm_throw_error(
            vm, t, ins,
            "Cannot execute call something not a Function or Class.");
        return true;
      }
      break;
    case RMDL:
      module = vm_lookup_module(vm, ins.str);
      t_set_resval(t, module);
      // TODO: Why do I need this for interpreter mode?
      memory_graph_set_field(vm->graph, block, ins.str, module);
      break;
    case MCLL:
      module = t_popstack(t, &has_error);
      if (has_error) {
        return true;
      }
      ASSERT(OBJECT == module.type, MODULE == module.obj->type);
      t_new_block(t, block, block);
      ip = module_ref(module.obj->module, ins.str);
      ASSERT(ip > 0);
      t_set_module(t, module, ip - 1);
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
  t_set_resval(t, tuple_get(tuple.obj->tuple, index));
}

bool execute_val_param(VM *vm, Thread *t, Ins ins) {
  Element elt = val_to_elt(ins.val), popped;
  Element tuple, array;
  bool has_error = false;
  int i;
  switch (ins.op) {
    case EXIT:
      t_set_resval(t, elt);
      return false;
    case RES:
      t_set_resval(t, elt);
      break;
    case PUSH:
      t_pushstack(t, elt);
      break;
    case PEEK:
      t_set_resval(t, t_peekstack(t, elt.val.int_val));
      break;
    case SINC:
      popped = t_popstack(t, &has_error);
      if (has_error) {
        return true;
      }
      t_pushstack(t, create_int(popped.val.int_val + elt.val.int_val));
      break;
    case TUPL:
      ASSERT(elt.type == VALUE, elt.val.type == INT);
      tuple = create_tuple(vm->graph);
      t_set_resval(t, tuple);
      for (i = 0; i < elt.val.int_val; i++) {
        popped = t_popstack(t, &has_error);
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
      tuple = t_get_resval(t);
      // TODO: Maybe there is a better solution.
      if ((tuple.type != OBJECT || tuple.obj->type != TUPLE) &&
          elt.val.int_val == 0) {
        t_set_resval(t, tuple);
        break;
      }
      execute_tget(vm, t, ins, tuple, elt.val.int_val);
      break;
    case JMP:
      ASSERT(elt.type == VALUE, elt.val.type == INT);
      t_shift_ip(t, elt.val.int_val);
      break;
    case IF:
      ASSERT(elt.type == VALUE, elt.val.type == INT);
      if (NONE != t_get_resval(t).type) {
        t_shift_ip(t, elt.val.int_val);
      }
      break;
    case IFN:
      ASSERT(elt.type == VALUE, elt.val.type == INT);
      if (NONE == t_get_resval(t).type) {
        t_shift_ip(t, elt.val.int_val);
      }
      break;
    case PRNT:
      vm_to_string(vm, elt, stdout);
      if (DBG) {
        fflush(stdout);
      }
      break;
    case ANEW:
      ASSERT(elt.type == VALUE, elt.val.type == INT);
      array = create_array(vm->graph);
      t_set_resval(t, array);
      for (i = 0; i < elt.val.int_val; i++) {
        popped = t_popstack(t, &has_error);
        if (has_error) {
          return true;
        }
        memory_graph_array_enqueue(vm->graph, array, popped);
      }
      break;
    case CTCH:
      ASSERT(elt.type == VALUE, elt.val.type == INT);
      uint32_t ip = t_get_ip(t);
      memory_graph_set_field(vm->graph, t_current_block(t),
                             strings_intern("$try_goto"),
                             create_int(ip + elt.val.int_val + 1));
      break;
    default:
      ERROR("Instruction op was not a val_param. op=%s",
            instructions[(int)ins.op]);
  }
  return true;
}

bool execute_str_param(VM *vm, Thread *t, Ins ins) {
  Element str_array = string_create(vm, ins.str);
  switch (ins.op) {
    case PUSH:
      t_pushstack(t, str_array);
      break;
    case RES:
      t_set_resval(t, str_array);
      break;
    case PRNT:
      elt_to_str(str_array, stdout);
      if (DBG) {
        fflush(stdout);
      }
      break;
    case PSRS:
      t_pushstack(t, str_array);
      t_set_resval(t, str_array);
      break;
    default:
      ERROR("Instruction op was not a str_param. op=%s",
            instructions[(int)ins.op]);
  }
  return true;
}

// returns whether or not the program should continue
bool execute(VM *vm, Thread *t) {
  ASSERT_NOT_NULL(vm);

  Ins ins = t_current_ins(t);

#ifdef DEBUG
  mutex_await(vm->debug_mutex, INFINITE);
  fflush(stderr);
  fprintf(stdout, "module(%s,t=%d) ", module_name(t_get_module(t).obj->module),
          (int)t->id);
  fflush(stdout);
  ins_to_str(ins, stdout);
  fprintf(stdout, "\n");
  fflush(stdout);
  fflush(stderr);
  mutex_release(vm->debug_mutex);
#endif

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
  if (NONE != obj_get_field(t_current_block(t), ERROR_KEY).type) {
    //    // TODO: Does this need to be a loop?
    //    while (NONE != obj_get_field(vm_current_block(vm, t), ERROR_KEY).type)
    //    {
    catch_error(vm, t);
    //    }
    return true;
  }

  t_shift_ip(t, 1);

  return status;
}

void vm_maybe_initialize_and_execute(VM *vm, Thread *t,
                                     Element module_element) {
  mutex_await(vm->module_init_mutex, INFINITE);
  if (is_true(obj_get_field(module_element, INITIALIZED))) {
    mutex_release(vm->module_init_mutex);
    return;
  }
  memory_graph_set_field(vm->graph, module_element, INITIALIZED,
                         element_true(vm));

  memory_graph_array_push(vm->graph, get_old_resvals(t), t_get_resval(t));

  ASSERT(NONE != module_element.type);
  t_new_block(t, module_element, module_element);
  t_set_module(t, module_element, 0);
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
  t_back(t);

  t_set_resval(t, memory_graph_array_pop(vm->graph, get_old_resvals(t)));

  mutex_release(vm->module_init_mutex);
}

void vm_start_execution(VM *vm, Element module) {
  Element main_function =
      create_function(vm, module, 0, strings_intern("main"));
  // Set to true so we don't double-run it.
  memory_graph_set_field(vm->graph, module, INITIALIZED, element_true(vm));
  Element thread = create_thread_object(vm, main_function, create_none());

  thread_start(Thread_extract(thread), vm);
}
