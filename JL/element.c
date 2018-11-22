/*
 * element.c
 *
 *  Created on: Sep 30, 2016
 *      Author: Jeff
 */

#include "element.h"

#include <inttypes.h>
#include <string.h>

#include "arena/arena.h"
#include "arena/strings.h"
#include "class.h"
#include "codegen/tokenizer.h"
#include "datastructure/array.h"
#include "datastructure/queue2.h"
#include "datastructure/tuple.h"
#include "error.h"
#include "external/external.h"
#include "external/strings.h"
#include "graph/memory_graph.h"
#include "module.h"
#include "vm.h"

Element create_obj_of_class_unsafe_inner(MemoryGraph *graph, Map *objs,
    Element class);
Element create_method_instance(MemoryGraph *graph, Element object,
    Element method);

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

Element element_for_obj(Object *obj) {
  Element e = { .type = OBJECT, .obj = obj };
  return e;
}

Element create_obj_unsafe_base(MemoryGraph *graph) {
  return memory_graph_new_node(graph);
}

void create_obj_unsafe_complete(MemoryGraph *graph, Map *objs, Element element,
    Element class) {
  memory_graph_set_field(graph, element, CLASS_KEY, class);
  Element parents = obj_lookup(class.obj, CKey_parent);
// Object class
  if (NONE == parents.type) {
    return;
  }

  ASSERT(OBJECT == parents.type, ARRAY == parents.obj->type);
  int i;
  for (i = 0; i < Array_size(parents.obj->array); ++i) {
    Element parent_class = Array_get(parents.obj->array, i);
    ASSERT(ISCLASS(parent_class));
    // Check if we already created a type of this class.
    Object *obj = map_lookup(objs, parent_class.obj);
    if (NULL == obj) {
      obj = create_obj_of_class_unsafe_inner(graph, objs, parent_class).obj;
      map_insert(objs, parent_class.obj, obj);
      // Add it to the list for lookup later.
      expando_append(element.obj->parent_objs, &obj);
    }
    // Add it as a member with the class name as the field name.
    Element name = obj_lookup(parent_class.obj, CKey_name);
    if (NONE != name.type) {
      memory_graph_set_field(graph, element, string_to_cstr(name),
          element_for_obj(obj));
    }
  }
}

void fill_object_unsafe(MemoryGraph *graph, Element element, Element class) {
  Map objs;
  map_init_default(&objs);
  create_obj_unsafe_complete(graph, &objs, element, class);
  map_finalize(&objs);
}

Element create_class_stub(MemoryGraph *graph) {
  return create_obj_unsafe_base(graph);
}

Element create_obj_of_class_unsafe_inner(MemoryGraph *graph, Map *objs,
    Element class) {
  Element to_return = create_obj_unsafe_base(graph);
  if (NONE == class.type) {
    return to_return;
  }
  create_obj_unsafe_complete(graph, objs, to_return, class);
  return to_return;
}

Element create_obj_of_class_unsafe(MemoryGraph *graph, Element class) {
  Map objs;
  map_init_default(&objs);
  Element obj = create_obj_of_class_unsafe_inner(graph, &objs, class);
  map_finalize(&objs);
  return obj;
}

Element create_external_obj(VM *vm, Element class) {
  Element elt = create_obj_of_class(vm->graph, class);
  elt.obj->is_external = true;
  elt.obj->external_data = externaldata_create(vm, elt, class);
  return elt;
}

Element create_obj_of_class(MemoryGraph *graph, Element class) {
  ASSERT(NOT_NULL(graph));
  Element elt = create_obj_of_class_unsafe(graph, class);

  if (class.obj == class_array.obj) {
    elt.obj->type = ARRAY;
    elt.obj->array = Array_create();
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

Element string_create_len_unescape(VM *vm, const char *str, size_t len) {
  Element elt = create_external_obj(vm, class_string);
  ASSERT(NONE != elt.type);
  string_constructor(vm, elt.obj->external_data, create_none());
  elt.obj->external_data->deconstructor = string_deconstructor;
  String *string = String_extract(elt);

  if (NULL != str) {
    String_append_unescape(string, str, len);
  }
  memory_graph_set_field(vm->graph, elt, LENGTH_KEY,
      create_int(String_size(string)));
  return elt;
}

Element string_create_len(VM *vm, const char *str, size_t len) {
  Element elt = create_external_obj(vm, class_string);
  ASSERT(NONE != elt.type);
  String_of(vm, elt.obj->external_data, str, len);
  elt.obj->external_data->deconstructor = string_deconstructor;
  return elt;
}

Element string_create(VM *vm, const char *str) {
  return string_create_len_unescape(vm, str, strlen(str));
}

Element string_add(VM *vm, Element str1, Element str2) {
  ASSERT(ISTYPE(str1, class_string), ISTYPE(str2, class_string));
  Element elt = string_create(vm, NULL);
  String *target = String_extract(elt);
  String_append(target, String_extract(str1));
  String_append(target, String_extract(str2));
  memory_graph_set_field(vm->graph, elt, LENGTH_KEY,
      create_int(String_size(target)));
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

void decorate_function(VM *vm, Element func, Element module, uint32_t ins,
    const char name[]) {
  memory_graph_set_field(vm->graph, func, NAME_KEY, string_create(vm, name));
  memory_graph_set_field(vm->graph, func, INS_INDEX, create_int(ins));
  memory_graph_set_field(vm->graph, func, PARENT_MODULE, module);
}

Element create_function(VM *vm, Element module, uint32_t ins, const char name[]) {
  Element elt = create_obj_of_class(vm->graph, class_function);
  decorate_function(vm, elt, module, ins, name);
  return elt;
}

Element create_external_function(VM *vm, Element module, const char name[],
    ExternalFunction external_fn) {
  Element elt = create_obj_of_class(vm->graph, class_external_function);
  elt.obj->external_fn = external_fn;

  Object *function_object = *((Object **) expando_get(elt.obj->parent_objs, 0));
  decorate_function(vm, element_for_obj(function_object), module, -1, name);
  return elt;
}

Element create_method(VM *vm, Element module, uint32_t ins, Element class,
    const char name[]) {
  Element elt = create_obj_of_class(vm->graph, class_method);
  Object *function_object = *((Object **) expando_get(elt.obj->parent_objs, 0));
  decorate_function(vm, element_for_obj(function_object), module, ins, name);
  memory_graph_set_field(vm->graph, elt, PARENT_CLASS, class);
  return elt;
}

Element create_external_method(VM *vm, Element class, const char name[],
    ExternalFunction external_fn) {
  DEBUGF("name=%s", name);
  Element elt = create_obj_of_class(vm->graph, class_external_method);
  elt.obj->external_fn = external_fn;

  // ExternalMethod -> ExternalFunction -> Function
  Object *function_object = *((Object **) expando_get(
      (*((Object **) expando_get(elt.obj->parent_objs, 0)))->parent_objs, 0));
  decorate_function(vm, element_for_obj(function_object),
      obj_get_field(class, PARENT_MODULE), -1, name);
  memory_graph_set_field(vm->graph, elt, PARENT_CLASS, class);
  return elt;
}

Element create_method_instance(MemoryGraph *graph, Element object,
    Element method) {
  ASSERT(ISOBJECT(object));
  ASSERT(ISTYPE(method, class_method));
  Element elt = create_obj_of_class(graph, class_methodinstance);
  memory_graph_set_field(graph, elt, OBJ_KEY, object);
  memory_graph_set_field(graph, elt, METHOD_KEY, method);
//  DEBUGF("B");
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

  CommonKey key = CKey_lookup_key(field_name);
  if (key >= 0) {
    elt.obj->ltable[key] = field_val;
  }

  Element *old;
  if (NULL != (old = map_lookup(&elt.obj->fields, field_name))) {
    *old = field_val;
    return;
  }
  Element *elt_ptr = ARENA_ALLOC(Element);
  *elt_ptr = field_val;
  map_insert(&elt.obj->fields, field_name, elt_ptr);
}

Element obj_lookup(Object *obj, CommonKey key) {
  return obj->ltable[key];
}

Element obj_get_field_obj(Object *obj, const char field_name[]) {
  ASSERT_NOT_NULL(obj);
//  CommonKey key = CKey_lookup_key(field_name);
//  if (key >= 0) {
//    return obj->ltable[key];
//  }
  Element *to_return = map_lookup(&obj->fields, field_name);

  if (NULL == to_return) {
    return create_none();
  }
  return *to_return;
}

Element obj_get_field(Element elt, const char field_name[]) {
  ASSERT(OBJECT == elt.type);
  return obj_get_field_obj(elt.obj, field_name);
}

void obj_delete_ptr(Object *obj, bool free_mem) {
  ASSERT(NOT_NULL(obj));
  if (obj->is_external) {
    ASSERT(NOT_NULL(obj->external_data));
    if (NULL != obj->external_data->deconstructor) {
      obj->external_data->deconstructor(externaldata_vm(obj->external_data),
          obj->external_data, create_none());
    }
    externaldata_delete(obj->external_data);
  }
  if (free_mem) {
    void dealloc_elts(Pair *kv) {
      ARENA_DEALLOC(Element, kv->value);
    }
    map_iterate(&obj->fields, dealloc_elts);
  }
  map_finalize(&obj->fields);

  ASSERT(NOT_NULL(obj->parent_objs));
  expando_delete(obj->parent_objs);

  if (ARRAY == obj->type) {
    Array_delete(obj->array);
  } else if (TUPLE == obj->type) {
    tuple_delete(obj->tuple);
  }
}

Element obj_deep_lookup(Object *obj, const char name[]) {
  Element to_return = create_none();

  Set checked;
  set_init_default(&checked);
  Q to_process;
  Q_init(&to_process);
  Q_enqueue(&to_process, obj);

  while (Q_size(&to_process) > 0) {
    Object *obj = Q_dequeue(&to_process);
    ASSERT(NOT_NULL(obj));
    // Check object.
    Element field = obj_get_field_obj(obj, name);
    if (NONE != field.type) {
      if (!(ISCLASS_OBJ(obj) && ISTYPE(field, class_method))) {
        to_return = field;
        break;
      }
    }
    // Check class.
    Element class = obj_get_field_obj(obj, CLASS_KEY);
    // Class Object
    if (NONE == class.type) {
      continue;
    }
    ASSERT(ISCLASS(class));
    field = obj_get_field(class, name);
    if (NONE != field.type) {
      to_return = field;
      break;
    }
    // Mark that we already checked this object.
    set_insert(&checked, obj);
    int i;
    // Look through all parent objects.
    for (i = 0; i < expando_len(obj->parent_objs); ++i) {
      Object *parent_obj = *((Object **) expando_get(obj->parent_objs, i));
      // Add them to the queue if we haven't already checked them.
      if (NULL == set_lookup(&checked, parent_obj)) {
        Q_enqueue(&to_process, parent_obj);
      }
    }
  }
  Q_finalize(&to_process);
  set_finalize(&checked);

  return to_return;
}

void class_parents_action(Element child_class, ObjectActionUntil process) {
  if (!ISCLASS(child_class)) {
    return;
  }
  Q to_process;
  Q_init(&to_process);
  Q_enqueue(&to_process, child_class.obj);

  while (Q_size(&to_process) > 0) {
    Object *class_obj = Q_dequeue(&to_process);
    ASSERT(NOT_NULL(class_obj));

    if (process(class_obj)) {
      break;
    }
    Element parents = obj_lookup(class_obj, CKey_parent);
    if (NONE == parents.type) {
      continue;
    }
    ASSERT(OBJECT == parents.type, ARRAY == parents.obj->type);
    int i;
    for (i = 0; i < Array_size(parents.obj->array); ++i) {
      Element parent = Array_get(parents.obj->array, i);
      ASSERT(NONE != parent.type);
      Q_enqueue(&to_process, parent.obj);
    }
  }
  Q_finalize(&to_process);
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

char *string_to_cstr(Element elt_str) {
  ASSERT(NONE != elt_str.type);
  String *string = String_extract(elt_str);
  char *str = ALLOC_ARRAY(char, String_size(string) + 1);
  memmove(str, String_cstr(string), String_size(string));
  str[String_size(string)] = '\0';
  char *to_return = strings_intern(str);
  DEALLOC(str);
  return to_return;
}

#define VALTYPE_NAME(val) (((val).type==INT) ? "Int" : (((val).type==FLOAT) ? "Float" : "Char"))

void obj_to_str(Object *obj, FILE *file) {
  Element name, class = obj_get_field_obj(obj, CLASS_KEY);
  int i;
  switch (obj->type) {
  case OBJ:
  case MODULE:
    if (NONE == class.type) {
      fprintf(file, "?");
      fflush(file);
    } else {
      if (class.obj == class_string.obj) {
        fprintf(file, "'%*s'==",
            String_size(String_extract(element_from_obj(obj))),
            String_cstr(String_extract(element_from_obj(obj))));
        fflush(file);
      }
      fprintf(file, "%s", string_to_cstr(obj_lookup(class.obj, CKey_name)));
    }
    fflush(file);
    name = obj_lookup(obj, CKey_name);
    if (NONE != name.type
        && OBJECT == name.type /*&& ARRAY == name.obj->type*/) {
      fprintf(file, "[%s]", string_to_cstr(name));
      fflush(file);
    }
    fprintf(file, "(");
    fflush(file);
    void print_field(Pair *pair) {
      Element *field_val = (Element *) pair->value;
      Element to_print = *field_val;
      if (ISOBJECT(*field_val)) {
        Element field_class = obj_lookup(field_val->obj, CKey_class);
        to_print = field_class;
        if (ISOBJECT(field_class)) {
          Element class_name = obj_lookup(field_class.obj, CKey_name);
          to_print = class_name;
        }
      }
      fprintf(file, "%s:%s, ", (char *) pair->key,
          ISOBJECT(to_print) ? string_to_cstr(to_print) :
          ISVALUE(to_print) ? VALTYPE_NAME(to_print.val) : "(None)");
      fflush(file);
    }
    map_iterate(&obj->fields, print_field);
    fprintf(file, ")");
    fflush(file);
    break;
  case TUPLE:
    tuple_print(obj->tuple, file);
    break;
  case ARRAY:
    fprintf(file, "[");
    fflush(file);
    if (Array_size(obj->array) > 0) {
      elt_to_str(Array_get(obj->array, 0), file);
    }
    for (i = 1; i < Array_size(obj->array); i++) {
      fprintf(file, ",");
      fflush(file);
      elt_to_str(Array_get(obj->array, i), file);
    }
    fprintf(file, "]");
    fflush(file);
    break;
  default:
    fprintf(file, "no_impl");
    fflush(file);
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
    fprintf(file, "(Nil)");
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

Element element_from_obj(Object * const obj) {
  Element e = { .type = OBJECT, .obj = obj };
  return e;
}
