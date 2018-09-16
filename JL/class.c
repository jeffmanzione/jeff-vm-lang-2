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
Element class_external_function;
Element class_method;
Element class_module;
Element class_error;

void class_fill(VM *vm, Element class, const char class_name[],
    Element parent_class);

void class_init(VM *vm) {
  // Create dummy objects that are needed for VM construction first since we
  // need them for the VM to work =)
  //DEBUGF("A");
  class_class = create_class_stub(vm->graph);
  class_object = create_class_stub(vm->graph);
  class_string = create_class_stub(vm->graph);
  class_array = create_class_stub(vm->graph);
  class_tuple = create_class_stub(vm->graph);
  class_function = create_class_stub(vm->graph);
  class_external_function = create_class_stub(vm->graph);
  class_method = create_class_stub(vm->graph);
  class_module = create_class_stub(vm->graph);
  class_error = create_class_stub(vm->graph);


  //DEBUGF("B");
  class_fill(vm, class_object, OBJECT_NAME, create_none());
  //DEBUGF("F");
  class_fill(vm, class_class, CLASS_NAME, class_object);
  //DEBUGF("G");
  class_fill(vm, class_string, STRING_NAME, class_object);
  //DEBUGF("H");
  class_fill(vm, class_array, ARRAY_NAME, class_object);
  //DEBUGF("I");
  // Time to fill them
  class_fill(vm, class_tuple, TUPLE_NAME, class_object);
  class_fill(vm, class_function, FUNCTION_NAME, class_object);
  class_fill(vm, class_external_function, EXTERNAL_FUNCTION_NAME,
      class_function);
  class_fill(vm, class_method, METHOD_NAME, class_function);
  class_fill(vm, class_module, MODULE_NAME, class_object);
  class_fill(vm, class_error, ERROR_NAME, class_object);
}

void class_fill_base(VM *vm, Element class, const char class_name[],
    Element parents_array) {
  //DEBUGF("class_fill_base begin %s", class_name);
  memory_graph_set_field(vm->graph, class, PARENT_KEY, parents_array);
  memory_graph_set_field(vm->graph, class, NAME_KEY,
      string_create(vm, class_name));
  memory_graph_set_field(vm->graph, vm->root, class_name, class);
  fill_object_unsafe(vm->graph, class, class_class);
  //DEBUGF("class_fill_base end");
}

void class_fill(VM *vm, Element class, const char class_name[],
    Element parent_class) {
  Element parents = create_none();
  if (NONE != parent_class.type) {
    parents = create_array(vm->graph);
    memory_graph_array_enqueue(vm->graph, parents, parent_class);
  }
  class_fill_base(vm, class, class_name, parents);
}

void class_fill_list(VM *vm, Element class, const char class_name[],
    Expando *parent_classes) {
  Element parents = create_array(vm->graph);
  if (NONE == parent_classes) {
    return;
  }
  void add_parent_class(void *ptr) {
    Element parent_class = element_from_obj(*((Object **) ptr));
    memory_graph_array_enqueue(vm->graph, parents, parent_class);
  }
  expando_iterate(parent_classes, add_parent_class);

  class_fill_base(vm, class, class_name, parents);
}

Element class_create(VM *vm, const char class_name[], Element parent_class) {
  ASSERT(OBJECT == parent_class.type);
  Element elt = create_class_stub(vm->graph);
  class_fill(vm, elt, class_name, parent_class);
  return elt;
}

Element class_create_list(VM *vm, const char class_name[],
    Expando *parent_classes) {
  ASSERT(NULL !=parent_classes);
  Element elt = create_class_stub(vm->graph);
  class_fill_list(vm, elt, class_name, parent_classes);
  return elt;
}

bool inherits_from(Element class, Element super) {
//  elt_to_str(class, stdout);
//  printf("\n");
//  elt_to_str(super, stdout);
//  printf("\n\n");
//  fflush(stdout);
  ASSERT(ISCLASS(class), ISCLASS(super));
  if (class.obj == super.obj) {
    return true;
  }

  bool inherits = false;
  bool find_super(Object *class) {
    inherits = (class == super.obj);
    return inherits;
  }
  class_parents_action(class, find_super);
  return inherits;
}
