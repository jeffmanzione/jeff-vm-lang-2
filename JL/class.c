/*
 * class.c
 *
 *  Created on: Oct 4, 2016
 *      Author: Jeff
 */

#include "class.h"

#include "arena/strings.h"
#include "element.h"
#include "error.h"
#include "graph/memory_graph.h"
#include "vm.h"

Element class_class;
Element class_object;
Element class_array;
Element class_string;

Element class_tuple;
Element class_function;
Element class_method;
Element class_module;

void class_init(VM *vm) {
  // Create dummy objects that are needed for VM construction first since we
  // need them for the VM to work =)
  class_class = create_obj_unsafe(vm->graph);
  class_object = create_obj_unsafe(vm->graph);
  class_array = create_obj_unsafe(vm->graph);
  class_string = create_obj_unsafe(vm->graph);
  class_fill(vm, class_class, CLASS_NAME, create_none());
  class_fill(vm, class_object, OBJECT_NAME, create_none());
  memory_graph_set_field(vm->graph, class_class, PARENT_KEY, class_object);
  class_fill(vm, class_array, ARRAY_NAME, class_object);
  class_fill(vm, class_string, STRING_NAME, class_array);

  class_tuple = create_obj(vm->graph);
  class_function = create_obj(vm->graph);
  class_method = create_obj(vm->graph);
  class_module = create_obj(vm->graph);
  // Time to fill them
  class_fill(vm, class_tuple, TUPLE_NAME, class_object);
  class_fill(vm, class_function, FUNCTION_NAME, class_object);
  class_fill(vm, class_method, METHOD_NAME, class_function);
  class_fill(vm, class_module, MODULE_NAME, class_object);
}

void class_fill(VM *vm, Element class, const char class_name[],
    Element parent_class) {
  memory_graph_set_field(vm->graph, class, CLASS_KEY, class_class);
  memory_graph_set_field(vm->graph, class, PARENT_KEY, parent_class);
  memory_graph_set_field(vm->graph, class, NAME_KEY,
      string_create(vm, class_name));
  memory_graph_set_field(vm->graph, vm->root, class_name, class);
}

Element class_create(VM *vm, const char class_name[], Element parent_class) {
  ASSERT(OBJECT == parent_class.type);
  Element elt = create_obj(vm->graph);
  class_fill(vm, elt, class_name, parent_class);
  return elt;
}

bool inherits_from(Element class, Element super) {
  ASSERT(ISCLASS(class), ISCLASS(super));
  Element parent;
  while (NONE != (parent = obj_get_field(class, PARENT_KEY)).type) {
    if (parent.obj == super.obj) {
      return true;
    }
    class = parent;
  }
  return false;
}
