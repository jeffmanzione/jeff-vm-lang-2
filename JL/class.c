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
#include "external/external.h"
#include "memory/memory_graph.h"
#include "vm/vm.h"

Element class_class;
Element class_object;
Element class_array;
Element class_string;

Element class_tuple;
Element class_function;
Element class_external_function;
Element class_method;
Element class_external_method;
Element class_methodinstance;
Element class_module;
Element class_error;
Element class_thread;

void class_fill(VM *vm, Element class, const char class_name[],
                Element parent_class, Element module_or_parent);

void class_init(VM *vm) {
  // Create dummy objects that are needed for VM construction first since we
  // need them for the VM to work =)
  class_class = create_class_stub(vm->graph);
  class_object = create_class_stub(vm->graph);
  class_string = create_class_stub(vm->graph);
  class_array = create_class_stub(vm->graph);
  class_tuple = create_class_stub(vm->graph);
  class_function = create_class_stub(vm->graph);
  class_external_function = create_class_stub(vm->graph);
  class_external_method = create_class_stub(vm->graph);
  class_method = create_class_stub(vm->graph);
  class_methodinstance = create_class_stub(vm->graph);
  class_module = create_class_stub(vm->graph);
  class_error = create_class_stub(vm->graph);
  class_thread = create_class_stub(vm->graph);

  class_fill(vm, class_object, OBJECT_NAME, create_none(), vm->root);
  class_fill(vm, class_class, CLASS_NAME, class_object, vm->root);
  class_fill(vm, class_string, STRING_NAME, class_object, vm->root);
  class_fill(vm, class_array, ARRAY_NAME, class_object, vm->root);
  // Time to fill them
  class_fill(vm, class_tuple, TUPLE_NAME, class_object, vm->root);
  class_fill(vm, class_function, FUNCTION_NAME, class_object, vm->root);
  class_fill(vm, class_external_function, EXTERNAL_FUNCTION_NAME,
             class_function, vm->root);
  class_fill(vm, class_external_method, EXTERNAL_METHOD_NAME,
             class_external_function, vm->root);
  class_fill(vm, class_method, METHOD_NAME, class_function, vm->root);
  class_fill(vm, class_methodinstance, METHOD_INSTANCE_NAME, class_object,
             vm->root);
  class_fill(vm, class_module, MODULE_NAME, class_object, vm->root);
  class_fill(vm, class_error, ERROR_NAME, class_object, vm->root);
  class_fill(vm, class_thread, strings_intern("Thread"), class_object,
             vm->root);
  merge_object_class(vm);
  merge_array_class(vm);
}

void class_fill_base(VM *vm, Element class, const char class_name[],
                     Element parents_array, Element module_or_parent) {
  memory_graph_set_field(vm->graph, class, PARENT, module_or_parent);
  memory_graph_set_field(vm->graph, class, PARENTS_KEY, parents_array);
  memory_graph_set_field(vm->graph, class, NAME_KEY,
                         string_create(vm, class_name));
  memory_graph_set_field(vm->graph, vm->root, class_name, class);
  fill_object_unsafe(vm->graph, class, class_class);
}

void class_fill(VM *vm, Element class, const char class_name[],
                Element parent_class, Element module_or_parent) {
  Element parents = create_none();
  if (NONE != parent_class.type) {
    parents = create_array(vm->graph);
    memory_graph_array_enqueue(vm->graph, parents, parent_class);
  }
  class_fill_base(vm, class, class_name, parents, module_or_parent);
}

void class_fill_list(VM *vm, Element class, const char class_name[],
                     Expando *parent_classes, Element module_or_parent) {
  Element parents = create_array(vm->graph);
  if (NULL == parent_classes) {
    return;
  }
  void add_parent_class(void *ptr) {
    Element parent_class = element_from_obj(*((Object **)ptr));
    memory_graph_array_enqueue(vm->graph, parents, parent_class);
  }
  expando_iterate(parent_classes, add_parent_class);

  class_fill_base(vm, class, class_name, parents, module_or_parent);
}

Element class_create(VM *vm, const char class_name[], Element parent_class,
                     Element module_or_parent) {
  ASSERT(OBJECT == parent_class.type);
  Element elt = create_class_stub(vm->graph);
  class_fill(vm, elt, class_name, parent_class, module_or_parent);
  return elt;
}

Element class_create_list(VM *vm, const char class_name[],
                          Expando *parent_classes, Element module_or_parent) {
  ASSERT(NULL != parent_classes);
  Element elt = create_class_stub(vm->graph);
  class_fill_list(vm, elt, class_name, parent_classes, module_or_parent);
  return elt;
}

bool inherits_from(Element class, Element super) {
  ASSERT(ISCLASS(class), ISCLASS(super));
  if (class.obj == super.obj) {
    return true;
  }

  bool inherits = false;
  bool find_super(Object * class) {
    inherits = (class == super.obj);
    return inherits;
  }
  class_parents_action(class, find_super);
  return inherits;
}
