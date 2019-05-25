/*
 * thread.c
 *
 *  Created on: Nov 8, 2018
 *      Author: Jeff
 */

#include "thread.h"

#include <stdio.h>

#include "../arena/strings.h"
#include "../class.h"
#include "../datastructure/array.h"
#include "../datastructure/map.h"
#include "../datastructure/tuple.h"
#include "../external/external.h"
#include "../graph/memory.h"
#include "../graph/memory_graph.h"
#include "../ltable/ltable.h"
#include "../module.h"
#include "../vm.h"

void t_set_module_for_block(Thread *t, Element module_element, uint32_t ip,
                            Element block);

static int64_t THREAD_COUNT = 0;

typedef struct {
  Thread *thread;
  VM *vm;
} ThreadStartArgs;

Thread *thread_create(Element self, MemoryGraph *graph, Element root) {
  Thread *t = ALLOC(Thread);
  thread_init(t, self, graph, root);
  return t;
}

void thread_init(Thread *t, Element self, MemoryGraph *graph, Element root) {
  t->self = self;
  t->graph = graph;
  t->id = THREAD_COUNT++;
  t->access_mutex = mutex_create(NULL);
  ASSERT(NOT_NULL(t), NOT_NULL(graph));
  memory_graph_set_field(graph, self, strings_intern("id"), create_int(t->id));
  memory_graph_set_field(graph, self, CURRENT_BLOCK, (t->current_block = self));
  memory_graph_set_field(graph, self, ROOT, root);
  memory_graph_array_enqueue(graph, obj_get_field(root, THREADS_KEY), self);
  memory_graph_set_field(graph, self, SELF, self);
  memory_graph_set_field(graph, self, strings_intern("$thread"), self);
  memory_graph_set_field(graph, self, RESULT_VAL, create_none());
  memory_graph_set_field(graph, self, OLD_RESVALS, create_array(graph));
  memory_graph_set_field(graph, self, STACK, (t->stack = create_array(graph)));
  memory_graph_set_field(graph, self, SAVED_BLOCKS,
                         (t->saved_blocks = create_array(graph)));
}

void thread_start(Thread *t, VM *vm) {
  ASSERT(NOT_NULL(t));
  //  DEBUGF("thread_start(%d)", (int ) t->id);
  Element fn = obj_get_field(t->self, strings_intern("fn"));
  Element arg = obj_get_field(t->self, strings_intern("arg"));
  t_set_resval(t, arg);

  Element current_block = t_current_block(t);

  if (inherits_from(obj_get_field(fn, CLASS_KEY), class_function)) {
    Element parent_module = obj_get_field(fn, PARENT_MODULE);
    t_set_module(t, parent_module, 0);
    vm_call_fn(vm, t, parent_module, fn);
    t_shift_ip(t, 1);
  } else if (ISTYPE(fn, class_methodinstance)) {
    Element parent_module =
        obj_deep_lookup(obj_get_field(fn, METHOD_KEY).obj, PARENT_MODULE);
    t_set_module(t, parent_module, 0);
    vm_call_fn(vm, t, parent_module, fn);
    t_shift_ip(t, 1);
  } else {
    ERROR("NOOOOOOOOOO");
    //    if (ISTYPE(fn, class_class)) {
    //    vm_set_module(t->vm, t, t->self, 0);
    //    vm_call_new(t->vm, t, fn);
  }

  while (execute(vm, t)) {
    if (current_block.obj == t_current_block(t).obj) {
      break;
    }
  }
  Element result = t_get_resval(t);
  memory_graph_set_field(vm->graph, t->self, strings_intern("result"), result);
  //  DEBUGF("Thread end (%d).", (int ) t->id);
}

unsigned __stdcall thread_start_wrapper(void *ptr) {
  ThreadStartArgs *t = (ThreadStartArgs *)ptr;
  thread_start(t->thread, t->vm);
  return 0;
}

void thread_finalize(Thread *t) { ASSERT(NOT_NULL(t)); }

void thread_delete(Thread *t) {
  ASSERT(NOT_NULL(t));
  thread_finalize(t);
  DEALLOC(t);
}

Element Thread_constructor(VM *vm, Thread *t, ExternalData *data,
                           Element *arg) {
  if (!is_object_type(arg, TUPLE)) {
    DEBUGF("NEVER HERE.");
    return throw_error(vm, t, "Arguments to Thread must be a tuple.");
  }
  Tuple *args = arg->obj->tuple;
  if (tuple_size(args) < 2) {
    DEBUGF("NEVER HERE.");
    return throw_error(vm, t, "Too few arguments for Thread constructor.");
  }
  Element e_fn = tuple_get(args, 0);
  if (!ISTYPE(e_fn, class_function) && !ISTYPE(e_fn, class_methodinstance) &&
      !ISTYPE(e_fn, class_method) && !ISTYPE(e_fn, class_class)) {
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

  Thread *thread = thread_create(data->object, vm->graph, vm->root);
  map_insert(&data->state, strings_intern("thread"), thread);
  return data->object;
}

Element Thread_deconstructor(VM *vm, Thread *t, ExternalData *data,
                             Element *arg) {
  Thread *thread = map_lookup(&data->state, strings_intern("thread"));
  ThreadHandle handle = map_lookup(&data->state, strings_intern("handle"));
  ThreadStartArgs *args = map_lookup(&data->state, strings_intern("args"));
  if (NULL != handle) {
    thread_close(handle);
  }
  if (NULL != thread) {
    thread_delete(thread);
  }
  if (NULL != args) {
    DEALLOC(args);
  }
  return create_none();
}

Element Thread_start(VM *vm, Thread *t, ExternalData *data, Element *arg) {
  Thread *thread = map_lookup(&data->state, strings_intern("thread"));
  ASSERT(NOT_NULL(thread));

  ThreadId id;
  ThreadStartArgs args = {.thread = thread, .vm = vm};
  ThreadStartArgs *cpy = ALLOC(ThreadStartArgs);
  *cpy = args;
  ThreadHandle handle = create_thread(thread_start_wrapper, cpy, &id);
  map_insert(&data->state, strings_intern("handle"), handle);
  map_insert(&data->state, strings_intern("id"), (void *)id);
  map_insert(&data->state, strings_intern("args"), (void *)cpy);
  return data->object;
}

Element Thread_get_result(VM *vm, Thread *t, ExternalData *data, Element *arg) {
  ThreadHandle handle = map_lookup(&data->state, strings_intern("handle"));
  if (NULL == handle) {
    return throw_error(vm, t,
                       "Attempting to get result from before calling start().");
  }
  ulong duration = INFINITE;        // @suppress("Symbol is not resolved")
  if (is_value_type(arg, INT)) {    // @suppress("Symbol is not resolved")
    duration = VALUE_OF(arg->val);  // @suppress("Symbol is not resolved")
  } else if (NONE != arg->type) {
    return throw_error(vm, t, "Thread.wait() requires type Int.");
  }
  WaitStatus status = thread_await(handle, duration);
  if (status != WAIT_OBJECT_0) {   // @suppress("Symbol is not resolved")
    if (status == WAIT_TIMEOUT) {  // @suppress("Symbol is not resolved")
      return throw_error(vm, t, "Thread.get_result() timed out.");
    }
    return throw_error(vm, t, "Thread.get_result() failed.");
  }
  //  DEBUGF("Thread_get_result=%d", status);
  return obj_get_field(data->object, strings_intern("result"));
}

Element Thread_wait(VM *vm, Thread *t, ExternalData *data, Element *arg) {
  ThreadHandle handle = map_lookup(&data->state, strings_intern("handle"));
  if (NULL == handle) {
    return throw_error(
        vm, t, "Attempting to wait for Thread from before calling start().");
  }
  ulong duration = INFINITE;        // @suppress("Symbol is not resolved")
  if (is_value_type(arg, INT)) {    // @suppress("Symbol is not resolved")
    duration = VALUE_OF(arg->val);  // @suppress("Symbol is not resolved")
  } else if (NONE != arg->type) {
    return throw_error(vm, t, "Thread.wait() requires type Int.");
  }
  WaitStatus status = thread_await(handle, duration);
  if (status != WAIT_OBJECT_0) {   // @suppress("Symbol is not resolved")
    if (status == WAIT_TIMEOUT) {  // @suppress("Symbol is not resolved")
      return throw_error(vm, t, "Thread.wait() timed out.");
    }
    return throw_error(vm, t, "Thread.wait() failed.");
  }
  return create_int(status);
}

Element add_thread_class(VM *vm, Element module) {
  Element thread_class =
      create_external_class(vm, module, strings_intern("Thread"),
                            Thread_constructor, Thread_deconstructor);
  add_external_method(vm, thread_class, strings_intern("start"), Thread_start);
  add_external_method(vm, thread_class, strings_intern("wait"), Thread_wait);
  add_external_method(vm, thread_class, strings_intern("get_result"),
                      Thread_get_result);
  return thread_class;
}

Element create_thread_object(VM *vm, Element fn, Element arg) {
  Element elt = create_external_obj(vm, class_thread);
  ASSERT(NONE != elt.type);

  Element tuple_args = create_tuple(vm->graph);
  tuple_add(tuple_args.obj->tuple, fn);
  tuple_add(tuple_args.obj->tuple, arg);

  Thread_constructor(vm, NONE, elt.obj->external_data, &tuple_args);
  elt.obj->external_data->deconstructor = Thread_deconstructor;
  return elt;
}

Thread *Thread_extract(Element e) {
  ASSERT(obj_get_field(e, CLASS_KEY).obj == class_thread.obj);
  Thread *thread =
      map_lookup(&e.obj->external_data->state, strings_intern("thread"));
  ASSERT(NOT_NULL(thread));
  return thread;
}

void t_pushstack(Thread *t, Element element) {
  ASSERT_NOT_NULL(t);
  ASSERT(t->stack.type == OBJECT);
  ASSERT(t->stack.obj->type == ARRAY);
  ASSERT_NOT_NULL(t->stack.obj->array);
  memory_graph_array_enqueue(t->graph, t->stack, element);
}

Element t_popstack(Thread *t, bool *has_error) {
  ASSERT_NOT_NULL(t);
  ASSERT(t->stack.type == OBJECT);
  ASSERT(t->stack.obj->type == ARRAY);
  ASSERT_NOT_NULL(t->stack.obj->array);
  if (Array_is_empty(t->stack.obj->array)) {
    *has_error = true;
    //    vm_throw_error(vm, vm_current_ins(vm),
    //        "Attempted to pop from the empty stack.");
    return create_none();
  }
  return memory_graph_array_dequeue(t->graph, t->stack);
}

Element t_peekstack(Thread *t, int distance) {
  ASSERT_NOT_NULL(t);
  ASSERT(t->stack.type == OBJECT);
  ASSERT(t->stack.obj->type == ARRAY);
  Array *array = t->stack.obj->array;
  ASSERT_NOT_NULL(array);
  ASSERT(!Array_is_empty(array), (Array_size(array) - 1 - distance) >= 0);
  return Array_get(array, Array_size(array) - 1 - distance);
}

Element t_create_block(Thread *t, Element parent, Element new_this,
                       Element module_element, uint32_t ip) {
  Element new_block = create_obj(t->graph);
  memory_graph_set_field(t->graph, new_block, PARENT, parent);
  memory_graph_set_field(t->graph, new_block, SELF, new_this);
  t_set_module_for_block(t, module_element, ip, new_block);
  return new_block;
}

Element t_new_block(Thread *t, Element parent, Element new_this) {
  ASSERT_NOT_NULL(t);
  ASSERT(OBJECT == new_this.type);
  ASSERT_NOT_NULL(new_this.obj);
  Element old_block = t_current_block(t);

  // Save stack size for later for cleanup on ret.
  memory_graph_set_field(t->graph, old_block, STACK_SIZE_NAME,
                         create_int(Array_size(t->stack.obj->array)));

  memory_graph_array_push(t->graph, t->saved_blocks, old_block);

  Element new_block;

  Element module = t_get_module(t);
  uint32_t ip = t_get_ip(t);

  t->current_block = new_block =
      t_create_block(t, parent, new_this, module, ip);
  memory_graph_set_field(t->graph, t->self, CURRENT_BLOCK, new_block);
  return new_block;
}

// Returns false if there is an error.
bool t_back(Thread *t) {
  ASSERT_NOT_NULL(t);
  Element parent_block = memory_graph_array_pop(t->graph, t->saved_blocks);
  // Remove accumulated stack.
  // TODO: Maybe consider an increased stack a bug in the future.
  Element old_stack_size = obj_get_field(parent_block, STACK_SIZE_NAME);
  bool has_error = false;
  if (NONE != old_stack_size.type) {
    ASSERT(
        ISVALUE(old_stack_size),
        INT == old_stack_size.val.type);  // @suppress("Symbol is not resolved")
    int i;
    for (i = 0;
         i < Array_size(t->stack.obj->array) - old_stack_size.val.int_val;
         ++i) {
      t_popstack(t, &has_error);
      // TODO: Maybe return instead.
      if (has_error) {
        return false;
      }
    }
  }
  t->current_block = parent_block;
  memory_graph_set_field(t->graph, t->self, CURRENT_BLOCK, parent_block);
  return true;
}

void t_shift_ip(Thread *t, int num_ins) {
  ASSERT_NOT_NULL(t);
  Element block = t_current_block(t);
  ASSERT(OBJECT == block.type);
  memory_graph_set_field(t->graph, block, IP_FIELD,
                         create_int(t_get_ip(t) + num_ins));
}

Ins t_current_ins(const Thread *t) {
  ASSERT_NOT_NULL(t);
  const Module *m = t_get_module(t).obj->module;
  uint32_t i = t_get_ip(t);
  ASSERT(NOT_NULL(m), i >= 0, i < module_size(m));
  return module_ins(m, i);
}

Element t_current_block(const Thread *t) { return t->current_block; }

uint32_t t_get_ip(const Thread *t) {
  ASSERT_NOT_NULL(t);
  Element block = t_current_block(t);
  ASSERT(OBJECT == block.type);
  Element ip = obj_get_field(block, IP_FIELD);
  ASSERT(VALUE == ip.type,
         INT == ip.val.type);  // @suppress("Symbol is not resolved")
  return (uint32_t)ip.val.int_val;
}

void t_set_ip(Thread *t, uint32_t ip) {
  ASSERT_NOT_NULL(t);
  Element block = t_current_block(t);
  ASSERT(OBJECT == block.type);
  memory_graph_set_field(t->graph, block, IP_FIELD, create_int(ip));
}

void t_set_module_for_block(Thread *t, Element module_element, uint32_t ip,
                            Element block) {
  ASSERT(NOT_NULL(t));
  ASSERT(OBJECT == block.type);
  memory_graph_set_field(t->graph, block, MODULE_FIELD, module_element);
  memory_graph_set_field(t->graph, block, IP_FIELD, create_int(ip));
}

void t_set_module(Thread *t, Element module_element, uint32_t ip) {
  ASSERT_NOT_NULL(t);
  t_set_module_for_block(t, module_element, ip, t_current_block(t));
}

DEB_FN(Element, t_get_module, const Thread *t) {
  ASSERT_NOT_NULL(t);
  Element block = t_current_block(t);
  ASSERT(OBJECT == block.type);
  Element module = obj_get_field(block, MODULE_FIELD);
  ASSERT(OBJECT == module.type, MODULE == module.obj->type);
  return module;
}

void t_set_resval(Thread *t, const Element elt) {
  memory_graph_set_field(t->graph, t->self, RESULT_VAL, elt);
}

const Element t_get_resval(const Thread *t) {
  return obj_lookup(t->self.obj, CKey_resval);
}
