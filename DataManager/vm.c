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

#include "array.h"
#include "class.h"
#include "error.h"
#include "file_load.h"
#include "instruction.h"
#include "map.h"
#include "memory.h"
#include "memory_graph.h"
#include "module.h"
#include "ops.h"
#include "shared.h"
#include "strings.h"
#include "tape.h"
#include "tokenizer.h"
#include "tuple.h"

#ifdef DEBUG
#define BUILTIN_SRC "builtin.jl"
#else
#define BUILTIN_SRC "builtin.jb"
#endif

void vm_to_string(const VM *vm, Element elt, FILE *target);
void execute_tget(VM *vm, Ins ins, Element tuple, int64_t index);

Element current_block(const VM *vm) {
  return obj_get_field(vm->root, CURRENT_BLOCK);
}

uint32_t vm_get_ip(const VM *vm) {
  ASSERT_NOT_NULL(vm);
  Element block = current_block(vm);
  ASSERT(OBJECT == block.type);
  Element ip = obj_get_field(block, IP_FIELD);
  ASSERT(VALUE == ip.type, INT == ip.val.type);
  return (uint32_t) ip.val.int_val;
}

void vm_set_ip(VM *vm, uint32_t ip) {
  ASSERT_NOT_NULL(vm);
  Element block = current_block(vm);
  ASSERT(OBJECT == block.type);
  memory_graph_set_field(vm->graph, block, IP_FIELD, create_int(ip));
}

Element vm_get_module(const VM *vm) {
  ASSERT_NOT_NULL(vm);
  Element block = current_block(vm);
  ASSERT(OBJECT == block.type);
  Element module = obj_get_field(block, MODULE_FIELD);
  ASSERT(OBJECT == module.type, MODULE == module.obj->type);
  return module;
}

void vm_throw_error(const VM *vm, Ins ins, const char fmt[], ...) {
  fflush(stdout);
  const Module *module = vm_get_module(vm).obj->module;
  const InsContainer *c = module_insc(module, vm_get_ip(vm));
  const Token *tok = c->token;
  const FileInfo *fi = module_fileinfo(module);
  const LineInfo *li =
      (NULL == fi || NULL == tok) ? NULL : file_info_lookup(fi, tok->line);
  const char *module_fn = module_filename(module);
  fprintf(stderr, "Error in '%s' at line %d: ",
      (NULL == module_fn) ? "?" : module_fn, (NULL == tok) ? -1 : tok->line);
  fflush(stderr);
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fflush(stderr);
  va_end(ap);
  fprintf(stderr, "\n");
  if (NULL != fi && NULL != li && NULL != tok) {
    int num_line_digits = (int) (log10(file_info_len(fi)) + 1);
    fprintf(stderr, "%*d:%s", num_line_digits, tok->line,
        (NULL == li) ? "No LineInfo" : li->line_text);
    if ('\n' != li->line_text[strlen(li->line_text) - 1]) {
      fprintf(stderr, "\n");
    }
    int i;
    for (i = 0; i < tok->col + num_line_digits; i++) {
      fprintf(stderr, " ");
    }
    for (i = 0; i < tok->len; i++) {
      fprintf(stderr, "^");
    }
    fprintf(stderr, "\n");
    fflush(stderr);
  }
  int i = -1;
  Array *saved_array = obj_get_field(vm->root, SAVED_BLOCKS).obj->array;
  Element current_block = obj_get_field(vm->root, CURRENT_BLOCK);
  while (true) {
    Element module = obj_get_field(current_block, MODULE_FIELD);
    vm_to_string(vm, obj_get_field(module, NAME_KEY), stderr);
    fflush(stderr);

    Element caller = obj_get_field(current_block, CALLER_KEY);
    if (NONE != caller.type) {
      if (ISTYPE(caller, class_method)) {
        fprintf(stderr, ".");
        fflush(stderr);
        vm_to_string(vm,
            obj_get_field(obj_get_field(caller, PARENT_CLASS), NAME_KEY),
            stderr);
        fflush(stderr);

      }
      fprintf(stderr, ".");
      fflush(stderr);
      vm_to_string(vm, obj_get_field(caller, NAME_KEY), stderr);
    }
    fprintf(stderr, "(");
    fflush(stderr);
    const char *fn = module_filename(module.obj->module);
    fprintf(stderr, "%s:", (NULL == fn) ? "?" : fn);
    fflush(stderr);

    uint32_t ip = obj_get_field(current_block, IP_FIELD).val.int_val;
    const InsContainer *ci = module_insc(module.obj->module, ip);
    const Token *toki = ci->token;
    fprintf(stderr, "%d)\n", (NULL == toki) ? -1 : toki->line);
    fflush(stderr);
//    vm_to_string(vm, obj_get_field(current_block, IP_FIELD), stderr);
    if (++i >= array_size(saved_array) - 1) {
      break;
    }
    current_block = array_get(saved_array, i);
  }

  fflush(stderr);
  exit(1);
}

Element vm_lookup_module(const VM *vm, const char module_name[]) {
  ASSERT(NOT_NULL(vm), NOT_NULL(module_name));
  Element module = obj_get_field(vm->modules, module_name);
  ASSERT(NONE != module.type);
  return module;
}

void vm_set_module(VM *vm, Element module_element, uint32_t ip) {
  ASSERT_NOT_NULL(vm);
  Element block = current_block(vm);
  ASSERT(OBJECT == block.type);
  memory_graph_set_field(vm->graph, block, MODULE_FIELD, module_element);
  memory_graph_set_field(vm->graph, block, IP_FIELD, create_int(ip));
}

void vm_shift_ip(VM *vm, int num_ins) {
  ASSERT_NOT_NULL(vm);
  Element block = current_block(vm);
  ASSERT(OBJECT == block.type);
  memory_graph_set_field(vm->graph, block, IP_FIELD,
      create_int(vm_get_ip(vm) + num_ins));
}

Ins vm_current_ins(const VM *vm) {
  ASSERT_NOT_NULL(vm);
  const Module *m = vm_get_module(vm).obj->module;
  uint32_t i = vm_get_ip(vm);
  ASSERT(NOT_NULL(m), i >= 0, i < module_size(m));
  return module_ins(m, i);
}

void vm_add_builtin(VM *vm) {
  Module *builtin = load_fn(strings_intern(BUILTIN_SRC));
  ASSERT(NOT_NULL(vm), NOT_NULL(builtin));
  Element builtin_element = create_module(vm, builtin);
  memory_graph_set_field(vm->graph, vm->modules, module_name(builtin),
      builtin_element);
  memory_graph_set_field(vm->graph, builtin_element, PARENT, vm->root);
  memory_graph_set_field(vm->graph, builtin_element, INITIALIZED,
      element_false(vm));
  void add_ref(Pair *pair) {
    Element function = create_function(vm, builtin_element,
        (uint32_t) pair->value, pair->key);
    memory_graph_set_field(vm->graph, builtin_element, pair->key, function);
    memory_graph_set_field(vm->graph, vm->root, pair->key, function);
  }
  map_iterate(module_refs(builtin), add_ref);
  void add_class(Pair *pair) {
    Element class;
    if ((class = obj_get_field(vm->root, pair->key)).type == NONE) {
      class = class_create(vm, pair->key, class_object);
    }
    memory_graph_set_field(vm->graph, builtin_element, pair->key, class);
    memory_graph_set_field(vm->graph, class, PARENT_MODULE, builtin_element);
    void add_method(Pair *pair2) {
      memory_graph_set_field(vm->graph, class, pair2->key,
          create_method(vm, builtin_element, (uint32_t) pair2->value, class,
              pair2->key));
    }
    map_iterate(pair->value, add_method);
  }
  map_iterate(module_classes(builtin), add_class);
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

  void add_ref(Pair *pair) {
    memory_graph_set_field(vm->graph, module_element, pair->key,
        create_function(vm, module_element, (uint32_t) pair->value, pair->key));
  }
  map_iterate(module_refs(module), add_ref);
  void add_class(Pair *pair) {
    Element class = class_create(vm, pair->key, class_object);
    memory_graph_set_field(vm->graph, module_element, pair->key, class);
    void add_method(Pair *pair2) {
      memory_graph_set_field(vm->graph, class, pair2->key,
          create_function(vm, module_element, (uint32_t) pair2->value,
              pair->key));
    }
    map_iterate(pair->value, add_method);
  }
  map_iterate(module_classes(module), add_class);
  return module_element;
}

VM *vm_create() {
  VM *vm = ALLOC(VM);
  vm->graph = memory_graph_create();
  vm->root = memory_graph_create_root_element(vm->graph);
  class_init(vm);
  memory_graph_set_field(vm->graph, vm->root, ROOT, vm->root);
  memory_graph_set_field(vm->graph, vm->root, CURRENT_BLOCK, vm->root);
  memory_graph_set_field(vm->graph, vm->root, THIS, vm->root);
  memory_graph_set_field(vm->graph, vm->root, RESULT_VAL, create_none());
  memory_graph_set_field(vm->graph, vm->root, OLD_RESVALS,
      create_array(vm->graph));
  memory_graph_set_field(vm->graph, vm->root, STACK,
      (vm->stack = create_array(vm->graph)));
  memory_graph_set_field(vm->graph, vm->root, SAVED_BLOCKS, (vm->saved_blocks =
      create_array(vm->graph)));
  memory_graph_set_field(vm->graph, vm->root, MODULES, (vm->modules =
      create_obj(vm->graph)));
  memory_graph_set_field(vm->graph, vm->root, NIL_KEYWORD, create_none());
  memory_graph_set_field(vm->graph, vm->root, FALSE_KEYWORD, create_none());
  memory_graph_set_field(vm->graph, vm->root, TRUE_KEYWORD, create_int(1));
  vm_add_builtin(vm);
  return vm;
}

void vm_delete(VM *vm) {
  ASSERT_NOT_NULL(vm->graph);
  module_delete(
      (Module *) vm_lookup_module(vm, strings_intern("builtin")).obj->module);
  memory_graph_delete(vm->graph);
  vm->graph = NULL;
  DEALLOC(vm);
}

void vm_pushstack(VM *vm, Element element) {
  ASSERT_NOT_NULL(vm);
  ASSERT(vm->stack.type == OBJECT);
  ASSERT(vm->stack.obj->type == ARRAY);
  ASSERT_NOT_NULL(vm->stack.obj->array);
  memory_graph_array_enqueue(vm->graph, vm->stack, element);
}

Element vm_popstack(VM *vm) {
  ASSERT_NOT_NULL(vm);
  ASSERT(vm->stack.type == OBJECT);
  ASSERT(vm->stack.obj->type == ARRAY);
  ASSERT_NOT_NULL(vm->stack.obj->array);
  ASSERT(!array_is_empty(vm->stack.obj->array));
  return memory_graph_array_dequeue(vm->graph, vm->stack);
}

Element vm_peekstack(VM *vm) {
  ASSERT_NOT_NULL(vm);
  ASSERT(vm->stack.type == OBJECT);
  ASSERT(vm->stack.obj->type == ARRAY);
  Array *array = vm->stack.obj->array;
  ASSERT_NOT_NULL(array);
  ASSERT(!array_is_empty(array));
  return array_get(array, array_size(array) - 1);
}

Element vm_new_block(VM *vm, Element parent, Element new_this) {
  ASSERT_NOT_NULL(vm);
  ASSERT(OBJECT == new_this.type);
  ASSERT_NOT_NULL(new_this.obj);
  Element old_block = obj_get_field(vm->root, CURRENT_BLOCK);

  memory_graph_array_push(vm->graph, vm->saved_blocks, old_block);

  Element new_block = create_obj(vm->graph);

  Element module = vm_get_module(vm);
  uint32_t ip = vm_get_ip(vm);
//  Element resval = vm_get_resval(vm);
  memory_graph_set_field(vm->graph, vm->root, CURRENT_BLOCK, new_block);
  memory_graph_set_field(vm->graph, new_block, PARENT, parent);
  memory_graph_set_field(vm->graph, new_block, THIS, new_this);
//  memory_graph_set_field(vm->graph, new_block, RESULT_VAL, resval);
  vm_set_module(vm, module, ip);
  return new_block;
}

void vm_back(VM *vm) {
  ASSERT_NOT_NULL(vm);
  Element parent_block = memory_graph_array_pop(vm->graph, vm->saved_blocks);
  memory_graph_set_field(vm->graph, vm->root, CURRENT_BLOCK, parent_block);
}

const MemoryGraph *vm_get_graph(const VM *vm) {
  return vm->graph;
}

Element vm_object_lookup(VM *vm, Element obj, const char name[]) {
  Element lookup;

  if (NONE == obj.type || VALUE == obj.type) {
    return create_none();
  }
  // THIS IS PROBABLY THE PROBLEM
  lookup = obj_get_field(obj, name);
  if (NONE != lookup.type && (!ISCLASS(obj) || !ISTYPE(lookup, class_method))) {
    return lookup;
  }

  Element class = obj_get_field(obj, CLASS_KEY);
  while (NONE != class.type
      && NONE == (lookup = obj_get_field(class, name)).type) {
    class = obj_get_field(class, PARENT_KEY);
  }
  return lookup;
}

Element vm_object_get(VM *vm, const char name[]) {
  Element resval = vm_get_resval(vm);
  if (OBJECT != resval.type) {
    vm_throw_error(vm, vm_current_ins(vm), "Cannot get field '%s' from Nil.",
        name);
//    ERROR("Cannot access field(%s) of Nil.", name);
  }
  return vm_object_lookup(vm, resval, name);
}

Element vm_lookup(VM *vm, const char name[]) {
//DEBUGF("Looking for '%s'", name);
  Element block = obj_get_field(vm->root, CURRENT_BLOCK);
  Element lookup;
  while (NONE != block.type
      && NONE == (lookup = obj_get_field(block, name)).type) {
    block = obj_get_field(block, PARENT);
    //DEBUGF("Looking for '%s'", name);
  }
  if (NONE == block.type) {
    return create_none();
  }
  return lookup;
}

void vm_set_resval(VM *vm, const Element elt) {
//  elt_to_str(elt, stdout);
//  printf(" <-- " RESULT_VAL "\n");
//  fflush(stdout);
  memory_graph_set_field(vm->graph, vm->root, RESULT_VAL, elt);
}

const Element vm_get_resval(VM *vm) {
  return obj_get_field(vm->root, RESULT_VAL);
}

const Element vm_get_old_resvals(VM *vm) {
  return obj_get_field(vm->root, OLD_RESVALS);
}

void call_fn(VM *vm, Element obj, Element func) {
  if (func.type != OBJECT || func.obj->type != FUNCTION) {
    vm_throw_error(vm, vm_current_ins(vm),
        "Attempted to call something not a function of Class.");
  }
  Element parent = obj_get_field(func, PARENT_MODULE);
  ASSERT(OBJECT == parent.type, MODULE == parent.obj->type);
  vm_maybe_initialize_and_execute(vm, parent.obj->module);

  Element new_block = vm_new_block(vm, parent, obj);
  memory_graph_set_field(vm->graph, new_block, CALLER_KEY, func);
//DEBUGF("new_line=%d", obj_get_field(elt, INS_INDEX).val.int_val);
  vm_set_module(vm, parent, obj_get_field(func, INS_INDEX).val.int_val - 1);
}

void call_new(VM *vm, Element class) {
  ASSERT(ISCLASS(class));
  Element new_obj = create_obj_of_class(vm->graph, class);
  Element new_func = obj_get_field(class, NEW_KEY);
  if (new_func.type == OBJECT && new_func.obj->type == FUNCTION) {
    call_fn(vm, new_obj, new_func);
  } else {
    vm_set_resval(vm, new_obj);
  }
}

void vm_to_string(const VM *vm, Element elt, FILE *target) {
//  if (OBJECT == elt.type) {
//    Element to_s = vm_object_lookup(vm, elt, "to_s");
//    if (OBJECT == to_s.type && FUNCTION == to_s.obj->type) {
//      // Since we need to call prnt again
//      vm_shift_ip(vm, -1);
//      call_fn(vm, elt, to_s);
//      return;
//    }
//  }
  elt_to_str(elt, target);
}

bool execute_no_param(VM *vm, Ins ins) {
  Element elt, index, new_val, class;
  switch (ins.op) {
  case NOP:
    return true;
  case EXIT:
    vm_set_resval(vm, vm_popstack(vm));
    return false;
  case PUSH:
    vm_pushstack(vm, (Element) vm_get_resval(vm));
    return true;
  case CALL:
    elt = vm_popstack(vm);
    if (elt.type == OBJECT && elt.obj->type == FUNCTION) {
      call_fn(vm, vm_lookup(vm, THIS), elt);
    } else if (elt.type == OBJECT && elt.obj->type == OBJ
        && class_class.obj == obj_get_field(elt, CLASS_KEY).obj) {
      call_new(vm, elt);
    } else {
      vm_throw_error(vm, ins,
          "Cannot execute call something not a Function or Class.");
    }
    return true;
  case ASET:
    elt = vm_popstack(vm);
    new_val = vm_popstack(vm);
    ASSERT(elt.type == OBJECT && elt.obj->type == ARRAY);
    index = vm_get_resval(vm);
    ASSERT(index.type == VALUE, index.val.type == INT)
    ;
    memory_graph_array_set(vm->graph, elt, index.val.int_val, new_val);
    vm_set_resval(vm, new_val);
    return true;
  case AIDX:
    elt = vm_popstack(vm);
    index = vm_get_resval(vm);
    if (index.type != VALUE || index.val.type != INT) {
      vm_throw_error(vm, ins, "Array indexing with something not an int.");
    }
    if (elt.type == OBJECT && elt.obj->type == TUPLE) {
      execute_tget(vm, ins, elt, index.val.int_val);
      return true;
    }
    if (elt.type != OBJECT || elt.obj->type != ARRAY) {
      vm_throw_error(vm, ins, "Array indexing on something not an Array.");
    }
    if (index.val.int_val < 0
        || index.val.int_val >= array_size(elt.obj->array)) {
      vm_throw_error(vm, ins,
          "Array Index out of bounds. Index=%d, Array.len=%d.",
          index.val.int_val, array_size(elt.obj->array));
    }
    vm_set_resval(vm, array_get(elt.obj->array, index.val.int_val));
    return true;
  case RES:
    vm_set_resval(vm, vm_popstack(vm));
    return true;
  case PEEK:
    vm_set_resval(vm, vm_peekstack(vm));
    return true;
  case DUP:
    elt = vm_peekstack(vm);
    vm_pushstack(vm, elt);
    return true;
  case RET:
    vm_back(vm);
    return true;
  case PRNT:
    elt = vm_get_resval(vm);
    vm_to_string(vm, elt, stdout);
    if (DBG) {
      fflush(stdout);
    }
    return true;
  case ANEW:
    vm_set_resval(vm, create_array(vm->graph));
    return true;
  case NOTC:
    vm_set_resval(vm, operator_notc(vm_get_resval(vm)));
    return true;
  case NOT:
    vm_set_resval(vm, element_not(vm, vm_get_resval(vm)));
    return true;
  default:
    break;
  }

  Element res;
  Element rhs = vm_popstack(vm);
  Element lhs = vm_popstack(vm);
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
    res = operator_eq(vm, lhs, rhs);
    break;
  case NEQ:
    res = operator_neq(vm, lhs, rhs);
    break;
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
    ASSERT(rhs.type == OBJECT, rhs.obj->type == OBJ,
        class_class.obj == obj_get_field(rhs, CLASS_KEY).obj)
    ;
//    fflush(stdout);
//    elt_to_str(rhs, stderr);
//    fprintf(stderr, "\n");
//    elt_to_str(lhs, stderr);
//    fprintf(stderr, "\n");
//    fflush(stderr);
    if (lhs.type != OBJECT) {
      res = element_false(vm);
      break;
    }
    class = obj_get_field(lhs, CLASS_KEY);
    while (class.type == OBJECT && class.obj->type == OBJ
        && class.obj != rhs.obj) {
      class = obj_get_field(class, PARENT_KEY);
    }
    if (class.type == OBJECT && class.obj->type == OBJ
        && class.obj == rhs.obj) {
      res = element_true(vm);
    } else {
      res = element_false(vm);
    }
    break;
  default:
    ERROR("Instruction op was not a no_param");
  }
  vm_set_resval(vm, res);

  return true;
}

bool execute_id_param(VM *vm, Ins ins) {
  ASSERT_NOT_NULL(ins.str);
  Element block = obj_get_field(vm->root, CURRENT_BLOCK);
  Element module, new_res_val, obj;
  int32_t ip;
  switch (ins.op) {
  case SET:
    memory_graph_set_field(vm->graph, block, ins.str, vm_get_resval(vm));
    break;
  case MDST:
    memory_graph_set_field(vm->graph, vm_get_module(vm), ins.str,
        vm_get_resval(vm));
    break;
  case FLD:
    new_res_val = vm_popstack(vm);
    memory_graph_set_field(vm->graph, vm_get_resval(vm), ins.str, new_res_val);
    vm_set_resval(vm, new_res_val);
    break;
  case PUSH:
    vm_pushstack(vm, vm_lookup(vm, ins.str));
    break;
  case PSRS:
    new_res_val = vm_lookup(vm, ins.str);
    vm_pushstack(vm, new_res_val);
    vm_set_resval(vm, new_res_val);
    break;
  case RES:
    vm_set_resval(vm, vm_lookup(vm, ins.str));
    break;
  case GET:
    vm_set_resval(vm, vm_object_get(vm, ins.str));
    break;
  case GTSH:
    vm_pushstack(vm, vm_object_get(vm, ins.str));
    break;
  case INC:
    new_res_val = vm_object_get(vm, ins.str);
    if (VALUE != new_res_val.type) {
      vm_throw_error(vm, ins,
          "Cannot increment '%s' because it is not a value-type.", ins.str);
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
    vm_set_resval(vm, new_res_val);
    break;
  case DEC:
    new_res_val = vm_object_get(vm, ins.str);
    if (VALUE != new_res_val.type) {
      vm_throw_error(vm, ins,
          "Cannot increment '%s' because it is not a value-type.", ins.str);
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
    vm_set_resval(vm, new_res_val);
    break;
  case CALL:
    obj = vm_popstack(vm);
    if (obj.type == NONE) {
      vm_throw_error(vm, ins, "Cannot deference Nil.");
    }
    if (obj.type != OBJECT) {
      vm_throw_error(vm, ins, "Cannot call a non-object.");
    }
    Element target = vm_object_lookup(vm, obj, ins.str);
    if (OBJECT != target.type) {
      vm_throw_error(vm, ins, "Object has no such function '%s'.", ins.str);
    }
    if (target.obj->type == FUNCTION) {
      call_fn(vm, obj, target);
    } else if (target.obj->type == OBJ
        && class_class.obj == obj_get_field(target, CLASS_KEY).obj) {
      call_new(vm, target);
    } else {
      vm_throw_error(vm, ins,
          "Cannot execute call something not a Function or Class.");
    }
    break;
  case RMDL:
    module = vm_lookup_module(vm, ins.str);
    vm_set_resval(vm, module);
    break;
  case MCLL:
    module = vm_popstack(vm);
    ASSERT(OBJECT == module.type, MODULE == module.obj->type)
    ;
    vm_new_block(vm, block, block);
    ip = module_ref(module.obj->module, ins.str);
    ASSERT(ip > 0);
    vm_set_module(vm, module, ip - 1);
    break;
  case PRNT:
    elt_to_str(vm_lookup(vm, ins.str), stdout);
    if (DBG) {
      fflush(stdout);
    }
    break;
  default:
    ERROR("Instruction op was not a id_param");
  }
  return true;
}

void execute_tget(VM *vm, Ins ins, Element tuple, int64_t index) {
  if (tuple.type != OBJECT || tuple.obj->type != TUPLE) {
    vm_throw_error(vm, ins, "Attempted to index something not a tuple.");
  }
  if (index < 0 || index >= tuple_size(tuple.obj->tuple)) {
    vm_throw_error(vm, ins,
        "Tuple Index out of bounds. Index=%d, Tuple.len=%d.", index,
        tuple_size(tuple.obj->tuple));
  }
  vm_set_resval(vm, tuple_get(tuple.obj->tuple, index));
}

bool execute_val_param(VM *vm, Ins ins) {
  Element elt = val_to_elt(ins.val);
  Element tuple, array;
  int i;
  switch (ins.op) {
  case EXIT:
    vm_set_resval(vm, elt);
    return false;
  case RES:
    vm_set_resval(vm, elt);
    break;
  case PUSH:
    vm_pushstack(vm, elt);
    break;
  case TUPL:
    ASSERT(elt.type == VALUE, elt.val.type == INT)
    ;
    tuple = create_tuple(vm->graph);
    vm_set_resval(vm, tuple);
    for (i = 0; i < elt.val.int_val; i++) {
      memory_graph_tuple_add(vm->graph, tuple, vm_popstack(vm));
    }
    break;
  case TGET:
    if (elt.type != VALUE || elt.val.type != INT) {
      vm_throw_error(vm, ins,
          "Attempted to index a tuple with something not an int.");
    }
    tuple = vm_get_resval(vm);
    execute_tget(vm, ins, tuple, elt.val.int_val);
    break;
  case JMP:
    ASSERT(elt.type == VALUE, elt.val.type == INT)
    ;
    vm_shift_ip(vm, elt.val.int_val);
    break;
  case IF:
    ASSERT(elt.type == VALUE, elt.val.type == INT)
    ;
    if (NONE != vm_get_resval(vm).type) {
      vm_shift_ip(vm, elt.val.int_val);
    }
    break;
  case IFN:
    ASSERT(elt.type == VALUE, elt.val.type == INT)
    ;
    if (NONE == vm_get_resval(vm).type) {
      vm_shift_ip(vm, elt.val.int_val);
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
    vm_set_resval(vm, array);
    for (i = 0; i < elt.val.int_val; i++) {
      memory_graph_array_enqueue(vm->graph, array, vm_popstack(vm));
    }
    break;
  default:
    ERROR("Instruction op was not a val_param. op=%s",
        instructions[(int ) ins.op]);
  }
  return true;
}

//bool execute_goto_param(VM *vm, Ins ins) {
//  Element block = obj_get_field(vm->root, CURRENT_BLOCK);
//  switch (ins.op) {
//  case CALL:
//    vm_new_block(vm, block, block);
//    vm_set_ip(vm, ins.go_to);
//    break;
//  case GOTO:
//    vm_set_ip(vm, ins.go_to);
//    break;
//  default:
//    ERROR("Instruction op was not a goto_param. op=%s", instructions[ins.op]);
//  }
//  return true;
//}

bool execute_str_param(VM *vm, Ins ins) {
  Element str_array = string_create(vm, ins.str);
  switch (ins.op) {
  case PUSH:
    vm_pushstack(vm, str_array);
    break;
  case RES:
    vm_set_resval(vm, str_array);
    break;
  case PRNT:
    elt_to_str(str_array, stdout);
    if (DBG) {
      fflush(stdout);
    }
    break;
  default:
    ERROR("Instruction op was not a str_param. op=%s",
        instructions[(int ) ins.op]);
  }
  return true;
}

// returns whether or not the program should continue
bool execute(VM *vm) {
  ASSERT_NOT_NULL(vm);
  Ins ins = vm_current_ins(vm);
//  fprintf(stdout, "module(%s) ", module_name(vm_get_module(vm).obj->module));
//  fflush(stdout);
//  ins_to_str(ins, stdout);
//  fprintf(stdout, "\n");
//  fflush(stdout);

  bool status;
  switch (ins.param) {
  case ID_PARAM:
    status = execute_id_param(vm, ins);
    break;
  case VAL_PARAM:
    status = execute_val_param(vm, ins);
    break;
//  case GOTO_PARAM:
//    status = execute_goto_param(vm, ins);
//    break;
  case STR_PARAM:
    status = execute_str_param(vm, ins);
    break;
  default:
    status = execute_no_param(vm, ins);
  }
  vm_shift_ip(vm, 1);
  return status;
}

void vm_maybe_initialize_and_execute(VM *vm, const Module *module) {
  Element module_element = vm_lookup_module(vm, module_name(module));
  if (is_true(obj_get_field(module_element, INITIALIZED))) {
    return;
  }
  memory_graph_set_field(vm->graph, module_element, INITIALIZED,
      element_true(vm));

  memory_graph_array_push(vm->graph, vm_get_old_resvals(vm), vm_get_resval(vm));

  ASSERT(NONE != module_element.type);
  vm_new_block(vm, module_element, module_element);
  vm_set_module(vm, module_element, 0);
//  int i = 0;
  while (execute(vm)) {
//    DEBUGF("EXECUTING INSTRUCTION #%d", i);
//    if (0 == ((i+1) % 100)) {
//      DEBUGF("FREEING SPACE YAY (%d)", i);
//      memory_graph_free_space(vm->graph);
//      DEBUGF("DONE FREEING SPACE", i);
//    }
//    i++;
  }
  vm_back(vm);

  vm_set_resval(vm, memory_graph_array_pop(vm->graph, vm_get_old_resvals(vm)));
}

