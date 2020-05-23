/*
 * external.c
 *
 *  Created on: Jun 2, 2018
 *      Author: Jeff
 */

#include "external.h"

#include <stdint.h>
#include <stdlib.h>

#include "../arena/strings.h"
#include "../class.h"
#include "../datastructure/array.h"
#include "../datastructure/tuple.h"
#include "../error.h"
#include "../memory/memory.h"
#include "../memory/memory_graph.h"
#include "../threads/thread.h"
#include "error.h"
#include "file.h"
#include "math.h"
#include "modules.h"
#include "strings.h"

Element Int__(VM *vm, Thread *t, ExternalData *data, Element *arg);
Element Float__(VM *vm, Thread *t, ExternalData *data, Element *arg);
Element Char__(VM *vm, Thread *t, ExternalData *data, Element *arg);
Element rand__(VM *vm, Thread *t, ExternalData *data, Element *arg);
Element srand__(VM *vm, Thread *t, ExternalData *data, Element *arg);
Element collect_garbage__(VM *vm, Thread *t, ExternalData *data, Element *arg);

ExternalData *externaldata_create(VM *vm, Element obj, Element class) {
  ExternalData *ed = ALLOC2(ExternalData);
  map_init_default(&ed->state);
  ed->vm = vm;
  ed->object = obj;
  Element deconstructor = obj_get_field(class, DECONSTRUCTOR_KEY);
  if (NONE == deconstructor.type) {
    ed->deconstructor = NULL;
  } else {
    ed->deconstructor = deconstructor.obj->external_fn;
  }
  return ed;
}

VM *externaldata_vm(const ExternalData *const ed) { return ed->vm; }

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

void add_global_builtin_external(VM *vm, Element builtin) {
  add_global_external_function(vm, builtin, "Int", Int__);
  add_global_external_function(vm, builtin, "Float", Float__);
  add_global_external_function(vm, builtin, "Char", Char__);
  add_global_external_function(vm, builtin, "srand", srand__);
  add_global_external_function(vm, builtin, "rand", rand__);
  add_global_external_function(vm, builtin, "collect_garbage",
                               collect_garbage__);
}

Element add_external_function(VM *vm, Element parent, const char fn_name[],
                              ExternalFunction fn) {
  const char *fn_name_str = strings_intern(fn_name);
  Element external_function =
      create_external_function(vm, parent, fn_name_str, fn);
  memory_graph_set_field(vm->graph, parent, fn_name_str, external_function);
  return external_function;
}

Element add_external_method(VM *vm, Element class, const char fn_name[],
                            ExternalFunction fn) {
  const char *fn_name_str = strings_intern(fn_name);
  Element external_function =
      create_external_method(vm, class, fn_name_str, fn);
  memory_graph_set_field(vm->graph, class, fn_name_str, external_function);

  Element methods_array;
  if (NONE == (methods_array = obj_get_field(class, METHODS_KEY)).type) {
    memory_graph_set_field(vm->graph, class, METHODS_KEY,
                           (methods_array = create_array(vm->graph)));
  }
  memory_graph_array_enqueue(vm->graph, methods_array, external_function);

  return external_function;
}

Element add_global_external_function(VM *vm, Element parent,
                                     const char fn_name[],
                                     ExternalFunction fn) {
  const char *fn_name_str = strings_intern(fn_name);
  Element external_function =
      add_external_function(vm, parent, fn_name_str, fn);
  memory_graph_set_field(vm->graph, vm->root, fn_name_str, external_function);
  return external_function;
}

void merge_external_class(VM *vm, Element class, ExternalFunction constructor,
                          ExternalFunction deconstructor) {
  memory_graph_set_field(vm->graph, class, IS_EXTERNAL_KEY, create_int(1));
  add_external_method(vm, class, CONSTRUCTOR_KEY, constructor);
  add_external_method(vm, class, DECONSTRUCTOR_KEY, deconstructor);
}

Element create_external_class(VM *vm, Element module, const char class_name[],
                              ExternalFunction constructor,
                              ExternalFunction deconstructor) {
  const char *name = strings_intern(class_name);
  Element class = class_create(vm, name, class_object, module);
  merge_external_class(vm, class, constructor, deconstructor);
  memory_graph_set_field(vm->graph, module, name, class);
  memory_graph_array_enqueue(vm->graph, obj_get_field(module, CLASSES_KEY),
                             class);
  return class;
}

Element object_lookup(VM *vm, Thread *t, ExternalData *data, Element *arg) {
  if (!ISTYPE(*arg, class_string)) {
    return throw_error(vm, t, "Cannot call $lookup with a non-String.");
  }
  return vm_object_lookup(vm, t, data->object, string_to_cstr(*arg));
}

void merge_object_class(VM *vm) {
  add_external_method(vm, class_object, "$lookup", object_lookup);
}

Element array_pop(VM *vm, Thread *t, ExternalData *data, Element *arg) {
  if (NONE == arg->type ||
      (OBJECT == arg->type && arg->obj == vm->empty_tuple.obj)) {
    return memory_graph_array_pop(t->graph, data->object.obj);
  }
  if (!is_value_type(arg, INT)) {  // @suppress("Symbol is not resolved")
    return throw_error(vm, t, "Cannot call pop with something not an Int.");
  }
  int num_to_pop = VALUE_OF(arg->val);  // @suppress("Symbol is not resolved")
  Element retval = create_array(t->graph);
  int i;
  for (i = 0; i < num_to_pop; ++i) {
    memory_graph_array_enqueue(
        t->graph, retval, memory_graph_array_pop(t->graph, data->object.obj));
  }
  return retval;
}

Element array_remove(VM *vm, Thread *t, ExternalData *data, Element *arg) {
  if (NONE == arg->type) {
    return memory_graph_array_dequeue(t->graph, data->object);
  }
  if (!is_value_type(arg, INT)) {  // @suppress("Symbol is not resolved")
    return throw_error(vm, t, "Cannot call remove with something not an Int.");
  }
  int num_to_pop = VALUE_OF(arg->val);  // @suppress("Symbol is not resolved")
  Element retval = create_array(t->graph);
  int i;
  for (i = 0; i < num_to_pop; ++i) {
    Element element = memory_graph_array_dequeue(t->graph, data->object);
    memory_graph_array_push(t->graph, retval.obj, &element);
  }
  return retval;
}

Element array_remove_at(VM *vm, Thread *t, ExternalData *data, Element *arg) {
  if (!is_value_type(arg, INT)) {  // @suppress("Symbol is not resolved")
    return throw_error(vm, t,
                       "Cannot call remove_at with something not an Int.");
  }
  int index = VALUE_OF(arg->val);  // @suppress("Symbol is not resolved")
  if (index < 0 || index > Array_size(extract_array(data->object))) {
    return throw_error(vm, t, "Cannot remove element at index outside bounds.");
  }
  Element retval = memory_graph_array_remove(t->graph, data->object, index);
  return retval;
}

Element array_swap(VM *vm, Thread *t, ExternalData *data, Element *arg) {
  if (!is_object_type(arg, TUPLE)) {
    return throw_error(vm, t, "Expected more than one arg.");
  }
  Tuple *args = arg->obj->tuple;
  if (tuple_size(args) != 2) {
    return throw_error(vm, t, "Expected 2 arguments.");
  }
  Element first = tuple_get(args, 0);
  Element second = tuple_get(args, 1);
  if (!is_value_type(&first, INT)) {
    return throw_error(vm, t, "First arg to swap must be an Int.");
  }
  if (!is_value_type(&second, INT)) {
    return throw_error(vm, t, "Second arg to swap must be an Int.");
  }
  if (first.val.int_val == second.val.int_val) {
    return create_none();
  }
  Array *arr = data->object.obj->array;
  Element tmp = Array_get(arr, first.val.int_val);
  Array_set(arr, first.val.int_val, Array_get(arr, second.val.int_val));
  Array_set(arr, second.val.int_val, tmp);
  return create_none();
}

Element array_shift(VM *vm, Thread *t, ExternalData *data, Element *arg) {
  if (!is_object_type(arg, TUPLE)) {
    return throw_error(vm, t, "Expected more than one arg.");
  }
  Tuple *args = arg->obj->tuple;
  if (tuple_size(args) != 3) {
    return throw_error(vm, t, "Expected 3 arguments.");
  }
  Element e_start = tuple_get(args, 0);
  Element e_count = tuple_get(args, 1);
  Element e_shift = tuple_get(args, 2);
  if (!is_value_type(&e_start, INT)) {
    return throw_error(vm, t, "First arg to shift must be an Int.");
  }
  if (!is_value_type(&e_count, INT)) {
    return throw_error(vm, t, "Second arg to shift must be an Int.");
  }
  if (!is_value_type(&e_shift, INT)) {
    return throw_error(vm, t, "Third arg to shift must be an Int.");
  }
  int64_t start = e_start.val.int_val;
  int64_t count = e_count.val.int_val;
  int64_t shift = e_shift.val.int_val;
  if (start + shift < 0) {
    return throw_error(vm, t, "Shift goes beyond lower bound.");
  }
  //  if (start + count + shift > Array_size(arr)) {
  //    return throw_error(vm, t, "Shift goes beyond upper bound.");
  //  }
  memory_graph_array_shift(vm->graph, data->object, start, count, shift);
  return create_none();
}

void merge_array_class(VM *vm) {
  add_external_method(vm, class_array, "pop", array_pop);
  add_external_method(vm, class_array, "remove", array_remove);
  add_external_method(vm, class_array, "remove_at", array_remove_at);
  add_external_method(vm, class_array, "swap", array_swap);
  add_external_method(vm, class_array, "shift", array_shift);
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

Element throw_error(VM *vm, Thread *t, const char msg[]) {
  DEBUGF(msg);
  vm_throw_error(vm, t, t_current_ins(t), msg);
  return t_get_resval(t);
}

Element Int__(VM *vm, Thread *t, ExternalData *data, Element *arg) {
  int64_t int_val;
  if (NONE == arg->type) {
    int_val = 0;
  } else if (OBJECT == arg->type) {
    int_val = obj_get_field_ptr(arg->obj, ADDRESS_KEY)->val.int_val;
  } else if (VALUE == arg->type) {
    int_val = (int64_t)VALUE_OF(arg->val);
  } else {
    return throw_error(vm, t, "Weird input to Int()");
  }
  return create_int(int_val);
}

Element Float__(VM *vm, Thread *t, ExternalData *data, Element *arg) {
  int8_t double_val;
  if (NONE == arg->type) {
    double_val = 0;
  } else if (OBJECT == arg->type) {
    double_val = (double)obj_get_field_ptr(arg->obj, ADDRESS_KEY)->val.int_val;
  } else if (VALUE == arg->type) {
    double_val = (double)VALUE_OF(arg->val);
  } else {
    return throw_error(vm, t, "Weird input to Float()");
  }
  return create_float(double_val);
}

Element Char__(VM *vm, Thread *t, ExternalData *data, Element *arg) {
  int8_t char_val;
  if (NONE == arg->type) {
    char_val = 0;
  } else if (OBJECT == arg->type) {
    char_val = (int8_t)obj_get_field_ptr(arg->obj, ADDRESS_KEY)->val.int_val;
  } else if (VALUE == arg->type) {
    char_val = (int8_t)VALUE_OF(arg->val);
  } else {
    return throw_error(vm, t, "Weird input to Char()");
  }
  return create_char(char_val);
}

Element srand__(VM *vm, Thread *t, ExternalData *data, Element *arg) {
  if (!is_value_type(arg, INT)) {
    return throw_error(vm, t, "srand() requires input of type Int.");
  }
  srand((unsigned int)arg->val.int_val);
  return create_none();
}

Element rand__(VM *vm, Thread *t, ExternalData *data, Element *arg) {
  int v = rand();
  return create_int(v);
}

Element collect_garbage__(VM *vm, Thread *t, ExternalData *data, Element *arg) {
  return create_int(memory_graph_free_space(vm->graph));
}
