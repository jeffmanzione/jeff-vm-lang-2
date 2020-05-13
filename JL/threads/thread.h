#ifndef THREADS_THREAD_H_
#define THREADS_THREAD_H_

#include <stdbool.h>
#include <stdint.h>

#include "../element.h"
#include "../error.h"
#include "../program/instruction.h"
#include "thread_interface.h"

typedef struct VM_ VM;

typedef struct Thread_ {
  int64_t id;
  MemoryGraph *graph;
  Element self, stack, saved_blocks, current_block;
  ThreadHandle access_mutex;
} Thread;

// Merges Thread class into external C type.
Element add_thread_class(VM *vm, Element module);

Thread *thread_create(Element self, MemoryGraph *graph, Element root);
void thread_start(Thread *t, VM *vm);
Element create_thread_object(VM *vm, Element fn, Element arg);
Thread *Thread_extract(Element e);

void thread_init(Thread *t, Element self, MemoryGraph *graph, Element root);
void thread_finalize(Thread *t);
void thread_delete(Thread *t);

void t_pushstack(Thread *t, Element element);
Element t_peekstack(Thread *t, int distance);
Element t_popstack(Thread *t, bool *has_error);

Ins t_current_ins(const Thread *t);

uint32_t t_get_ip(const Thread *t);
void t_set_ip(Thread *t, uint32_t ip);
void t_shift_ip(Thread *t, int32_t);

const Element t_get_resval(const Thread *t);
const Element *t_get_resval_ptr(const Thread *t);
void t_set_resval(Thread *t, const Element elt);

Element t_current_block(const Thread *t);
Element *t_current_block_ptr(const Thread *t);
// Creates a block with the specified obj as the parent
Element t_new_block(Thread *t, Element parent, Element new_this);
// goes back one block
bool t_back(Thread *t);

DEB_FN(Element, t_get_module, const Thread *t);
#define t_get_module(...) CALL_FN(t_get_module__, __VA_ARGS__)
void t_set_module(Thread *t, Element module_element, uint32_t ip);

bool is_block(Element elt);

#endif /* THREADS_THREAD_H_ */
