/*
 * thread.c
 *
 *  Created on: Nov 8, 2018
 *      Author: Jeff
 */

#include "thread.h"

#include <stddef.h>

#include "../arena/strings.h"
#include "../class.h"
#include "../datastructure/map.h"
#include "../datastructure/tuple.h"
#include "../error.h"
#include "../external/external.h"
#include "../external/strings.h"
#include "../graph/memory.h"
#include "../graph/memory_graph.h"
#include "../ltable/ltable.h"
#include "../vm.h"

static int64_t THREAD_COUNT = 0;

Thread *thread_create(Element self, VM *vm) {
  Thread *t = ALLOC(Thread);
  thread_init(t, self, vm);
  return t;
}

void thread_init(Thread *t, Element self, VM *vm) {
  t->self = self;
  t->vm = vm;
  t->id = THREAD_COUNT++;
  t->access_mutex = create_mutex(NULL);
  ASSERT(NOT_NULL(t), NOT_NULL(vm));
  memory_graph_set_field(vm->graph, self, strings_intern("id"),
      create_int(t->id));
  memory_graph_set_field(vm->graph, self, CURRENT_BLOCK, (t->current_block =
      self));
  memory_graph_set_field(vm->graph, self, ROOT, vm->root);
  memory_graph_set_field(vm->graph, self, SELF, self);
  memory_graph_set_field(vm->graph, self, strings_intern("$thread"), self);
  memory_graph_set_field(vm->graph, self, RESULT_VAL, create_none());
  memory_graph_set_field(vm->graph, self, OLD_RESVALS, create_array(vm->graph));
  memory_graph_set_field(vm->graph, self, STACK,
      (t->stack = create_array(vm->graph)));
  memory_graph_set_field(vm->graph, self, SAVED_BLOCKS, (t->saved_blocks =
      create_array(vm->graph)));
}

void thread_start(Thread *t) {
  ASSERT(NOT_NULL(t));
  Element fn = obj_get_field(t->self, strings_intern("fn"));
  Element arg = obj_get_field(t->self, strings_intern("arg"));
  vm_set_resval(t->vm, t, arg);
  Element current_block = vm_current_block(t->vm, t);

  if (inherits_from(obj_get_field(fn, CLASS_KEY),
      class_function) || ISTYPE(fn, class_methodinstance)) {
    Element parent_module = obj_get_field(fn, PARENT_MODULE);
    vm_set_module(t->vm, t, parent_module, 0);
    vm_call_fn(t->vm, t, parent_module, fn);
    vm_shift_ip(t->vm, t, 1);
  } else {
    ERROR("NOOOOOOOOOO");
//    if (ISTYPE(fn, class_class)) {
//    vm_set_module(t->vm, t, t->self, 0);
//    vm_call_new(t->vm, t, fn);
  }

  while (execute(t->vm, t)) {
    if (current_block.obj == vm_current_block(t->vm, t).obj) {
      break;
    }
  }
  Element result = vm_get_resval(t->vm, t);
  memory_graph_set_field(t->vm->graph, t->self, strings_intern("result"),
      result);
}

unsigned __stdcall thread_start_wrapper(void *ptr) {
  Thread *t = (Thread *) ptr;
  thread_start(t);
  return 0;
}

void thread_finalize(Thread *t) {
  ASSERT(NOT_NULL(t));
}

void thread_delete(Thread *t) {
  ASSERT(NOT_NULL(t));
  thread_finalize(t);
  DEALLOC(t);
}

Element Thread_constructor(VM *vm, Thread *t, ExternalData *data, Element arg) {
  if (!is_object_type(&arg, TUPLE)) {
    DEBUGF("NEVER HERE.");
    return throw_error(vm, t, "Arguments to Thread must be a tuple.");
  }
  Tuple *args = arg.obj->tuple;
  if (tuple_size(args) < 2) {
    DEBUGF("NEVER HERE.");
    return throw_error(vm, t, "Too few arguments for Thread constructor.");
  }
  Element e_fn = tuple_get(args, 0);
  if (!ISTYPE(e_fn, class_function) && !ISTYPE(e_fn, class_methodinstance)
  && !ISTYPE(e_fn, class_method) && !ISTYPE(e_fn, class_class)) {
//    Element class_name = obj_lookup(obj_lookup(e_fn.obj, CKey_class).obj,
//        CKey_name);
//    DEBUGF("NEVER HERE. class_name=%*s",
//        String_size(String_extract(class_name)),
//        String_cstr(String_extract(class_name)));
    return throw_error(vm, t, "Thread must be passed a function or class.");
  }
  Element e_arg = tuple_get(args, 1);

  memory_graph_set_field(vm->graph, data->object, strings_intern("fn"), e_fn);
  memory_graph_set_field(vm->graph, data->object, strings_intern("arg"), e_arg);

  Thread *thread = thread_create(data->object, vm);
  map_insert(&data->state, strings_intern("thread"), thread);
  return data->object;
}

Element Thread_deconstructor(VM *vm, Thread *t, ExternalData *data, Element arg) {
  Thread *thread = map_lookup(&data->state, strings_intern("thread"));
  ThreadHandle handle = map_lookup(&data->state, strings_intern("handle"));
  if (NULL != handle) {
    close_thread(handle);
  }
  if (NULL != thread) {
    thread_delete(thread);
  }
  return create_none();
}

Element Thread_start(VM *vm, Thread *t, ExternalData *data, Element arg) {
  Thread *thread = map_lookup(&data->state, strings_intern("thread"));
  ASSERT(NOT_NULL(thread));

  ThreadId id;
  ThreadHandle handle = create_thread(thread_start_wrapper, thread, &id);
  map_insert(&data->state, strings_intern("handle"), handle);
  map_insert(&data->state, strings_intern("id"), (void *) id);
  return data->object;
}

Element Thread_get_result(VM *vm, Thread *t, ExternalData *data, Element arg) {
  ThreadHandle handle = map_lookup(&data->state, strings_intern("handle"));
  if (NULL == handle) {
    return throw_error(vm, t,
        "Attempting to get result from before calling start().");
  }
  wait_for_thread(handle, INFINITE);
  return obj_get_field(data->object, strings_intern("result"));
}

Element Thread_wait(VM *vm, Thread *t, ExternalData *data, Element arg) {
  ThreadHandle handle = map_lookup(&data->state, strings_intern("handle"));
  if (NULL == handle) {
    return throw_error(vm, t,
        "Attempting to wait for Thread from before calling start().");
  }
  wait_for_thread(handle, INFINITE);
  return data->object;
}

Element add_thread_class(VM *vm, Element module) {
  Element thread_class = create_external_class(vm, module,
      strings_intern("Thread"), Thread_constructor, Thread_deconstructor);
  add_external_function(vm, thread_class, strings_intern("start"),
      Thread_start);
  add_external_function(vm, thread_class, strings_intern("wait"), Thread_wait);
  add_external_function(vm, thread_class, strings_intern("get_result"),
      Thread_get_result);
  return thread_class;
}

Element create_thread_object(VM *vm, Element fn, Element arg) {
  Element elt = create_external_obj(vm, class_thread);
  ASSERT(NONE != elt.type);

  Element tuple_args = create_tuple(vm->graph);
  tuple_add(tuple_args.obj->tuple, fn);
  tuple_add(tuple_args.obj->tuple, arg);

  Thread_constructor(vm, NONE, elt.obj->external_data, tuple_args);
  elt.obj->external_data->deconstructor = Thread_deconstructor;
  return elt;
}

Thread *Thread_extract(Element e) {
  ASSERT(obj_get_field(e, CLASS_KEY).obj == class_thread.obj);
  Thread *thread = map_lookup(&e.obj->external_data->state,
      strings_intern("thread"));
  ASSERT(NOT_NULL(thread));
  return thread;
}
