/*
 * element.c
 *
 *  Created on: Sep 30, 2016
 *      Author: Jeff
 */

#include "element.h"

#include <inttypes.h>
#include <string.h>

#include "arena.h"
#include "array.h"
#include "class.h"
#include "error.h"
#include "memory_graph.h"
#include "module.h"
#include "strings.h"
#include "tuple.h"
#include "vm.h"

Element create_int(int64_t val) {
  Element to_return;
  to_return.type = VALUE;
  to_return.val.type = INT;
  to_return.val.int_val = val;
  return to_return;
}

Element create_float(double val) {
  Element to_return;
  to_return.type = VALUE;
  to_return.val.type = FLOAT;
  to_return.val.float_val = val;
  return to_return;
}

Element create_char(int8_t val) {
  Element to_return;
  to_return.type = VALUE;
  to_return.val.type = CHAR;
  to_return.val.char_val = val;
  return to_return;
}

Element create_obj_of_class_unsafe(MemoryGraph *graph, Element class) {
  Element to_return = memory_graph_new_node(graph);
  memory_graph_set_field(graph, to_return, CLASS_KEY, class);
  return to_return;
}

Element create_obj_of_class(MemoryGraph *graph, Element class) {
  ASSERT(NOT_NULL(graph), class.type == OBJECT);
  Element elt = create_obj_of_class_unsafe(graph, class);
  if (class.obj == class_array.obj || class.obj == class_string.obj
      || inherits_from(class, class_array)) {
    elt.obj->type = ARRAY;
    elt.obj->array = array_create();
    memory_graph_set_field(graph, elt, LENGTH_KEY, create_int(0));
  }
  return elt;
}

Element create_obj_unsafe(MemoryGraph *graph) {
  return create_obj_of_class_unsafe(graph, class_object);
}

Element create_obj(MemoryGraph *graph) {
  return create_obj_of_class(graph, class_object);
}

Element create_array(MemoryGraph *graph) {
  return create_obj_of_class(graph, class_array);
}

Element string_create(VM *vm, const char *str) {
  Element elt = create_obj_of_class(vm->graph, class_string);
  if (NULL != str) {
    int i;
    for (i = 0; i < strlen(str); i++) {
      array_enqueue(elt.obj->array, create_char(str[i]));
      //memory_graph_array_enqueue(vm->graph, elt, create_char(str[i]));
    }
    memory_graph_set_field(vm->graph, elt, LENGTH_KEY,
        create_int(array_size(elt.obj->array)));
  }
  return elt;
}

Element string_add(VM *vm, Element str1, Element str2) {
  ASSERT(ISTYPE(str1, class_string), ISTYPE(str2, class_string));
  Element elt = memory_graph_array_join(vm->graph, str1, str2);
  memory_graph_set_field(vm->graph, elt, CLASS_KEY, class_string);
  return elt;
}

Element create_tuple(MemoryGraph *graph) {
  Element elt = create_obj_of_class(graph, class_tuple);
  elt.obj->type = TUPLE;
  elt.obj->tuple = tuple_create();
  memory_graph_set_field(graph, elt, LENGTH_KEY, create_int(0));
  return elt;
}

Element create_module(VM *vm, const Module *module) {
  Element elt = create_obj_of_class(vm->graph, class_module);
  memory_graph_set_field(vm->graph, elt, NAME_KEY,
      string_create(vm, module_name(module)));
  elt.obj->type = MODULE;
  elt.obj->module = module;
  return elt;
}

Element create_function(VM *vm, Element module, uint32_t ins, const char name[]) {
  Element elt = create_obj_of_class(vm->graph, class_function);
  elt.obj->type = FUNCTION;
  memory_graph_set_field(vm->graph, elt, NAME_KEY, string_create(vm, name));
  memory_graph_set_field(vm->graph, elt, INS_INDEX, create_int(ins));
  memory_graph_set_field(vm->graph, elt, PARENT_MODULE, module);
  return elt;
}

Element create_method(VM *vm, Element module, uint32_t ins, Element class,
    const char name[]) {
  Element elt = create_function(vm, module, ins, name);
  memory_graph_set_field(vm->graph, elt, CLASS_KEY, class_method);
  memory_graph_set_field(vm->graph, elt, PARENT_CLASS, class);
  memory_graph_set_field(vm->graph, elt, PARENT_MODULE, module);
  return elt;
}

Element create_none() {
  Element to_return;
  to_return.type = NONE;
  return to_return;
}

Element val_to_elt(Value val) {
  Element elt = { .type = VALUE, .val = val };
  return elt;
}

void obj_set_field(Element elt, const char field_name[], Element field_val) {
  ASSERT(elt.type == OBJECT);
  ASSERT_NOT_NULL(elt.obj);
  Element *old;
  if (NULL != (old = map_lookup(&elt.obj->fields, field_name))) {
    *old = field_val;
    return;
  }
  Element *elt_ptr = ARENA_ALLOC(Element);
  *elt_ptr = field_val;
  map_insert(&elt.obj->fields, field_name, elt_ptr);
}

Element obj_get_field_helper(Object *obj, const char field_name[]) {
  ASSERT_NOT_NULL(obj);
  Element *to_return = map_lookup(&obj->fields, field_name);

  if (NULL == to_return) {
    return create_none();
  }
  return *to_return;
}

Element obj_get_field(Element elt, const char field_name[]) {
  ASSERT(elt.type == OBJECT);
  return obj_get_field_helper(elt.obj, field_name);
}

void obj_delete_ptr(Object *obj) {
  void dealloc_elts(Pair *kv) {
    ARENA_DEALLOC(Element, kv->value);
  }
  map_iterate(&obj->fields, dealloc_elts);
  map_finalize(&obj->fields);

  if (ARRAY == obj->type) {
    array_delete(obj->array);
  } else if (TUPLE == obj->type) {
    tuple_delete(obj->tuple);
  }
}

// shallow object delete
void obj_delete(Element elt) {
  ASSERT(elt.type == OBJECT);
  ASSERT_NOT_NULL(elt.obj);
  obj_delete_ptr(elt.obj);
}

void val_to_str(Value val, FILE *file) {
  switch (val.type) {
  case INT:
    fprintf(file, "%" PRId64, val.int_val);
    break;
  case FLOAT:
    fprintf(file, "%f", val.float_val);
    break;
  default /*CHAR*/:
    fprintf(file, "%c", val.char_val);
    break;
  }
}

Value value_negate(Value val) {
  switch (val.type) {
  case INT:
    val.int_val = -val.int_val;
    break;
  case FLOAT:
    val.float_val = -val.float_val;
    break;
  default:
    ERROR("Unable to value_negate(val)");
  }
  return val;
}

void obj_to_str(Object *obj, FILE *file) {
  Element class = obj_get_field_helper(obj, CLASS_KEY);
  int i;
  switch (obj->type) {
  case OBJ:
  case MODULE:
  case FUNCTION:
    fprintf(file, "obj(");
    void print_field(Pair *pair) {
      fprintf(file, "%s,", (char *) pair->key);
    }
    map_iterate(&obj->fields, print_field);
    fprintf(file, ")");
    break;
  case TUPLE:
    tuple_print(obj->tuple, file);
    break;
  case ARRAY:
    if (class.type != NONE && class.obj == class_string.obj) {
      for (i = 0; i < array_size(obj->array); i++) {
        elt_to_str(array_get(obj->array, i), file);
      }
    } else {
      array_print(obj->array, file);
    }
    break;
  default:
    fprintf(file, "no_impl");
    break;
  }
}

void elt_to_str(Element elt, FILE *file) {
  switch (elt.type) {
  case OBJECT:
    obj_to_str(elt.obj, file);
    break;
  case VALUE:
    val_to_str(elt.val, file);
    break;
  default:
    fprintf(file, "Nil");
  }
}

Element element_true(VM *vm) {
  return vm_lookup(vm, TRUE_KEYWORD);
}

Element element_false(VM *vm) {
  return vm_lookup(vm, FALSE_KEYWORD);
}

Element element_not(VM *vm, Element elt) {
  return (NONE == elt.type) ? element_true(vm) : element_false(vm);
}

bool is_true(Element elt) {
  return NONE != elt.type;
}

bool is_false(Element elt) {
  return NONE == elt.type;
}
