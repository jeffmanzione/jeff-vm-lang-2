#ifndef THREADS_THREAD_H_
#define THREADS_THREAD_H_

#include <stdint.h>

#include "../element.h"
#include "thread_interface.h"

typedef struct VM_ VM;

typedef struct Thread_ {
  int64_t id;
  VM *vm;
  Element self, stack, saved_blocks, current_block;
  ThreadHandle access_mutex;
} Thread;

// Merges Thread class into external C type.
Element add_thread_class(VM *vm, Element module);

Thread *thread_create(Element self, VM *vm);
void thread_start(Thread *t);
Element create_thread_object(VM *vm, Element fn, Element arg);
Thread *Thread_extract(Element e);

void thread_init(Thread *t, Element self, VM *vm);
void thread_finalize(Thread *t);
void thread_delete(Thread *t);


#endif /* THREADS_THREAD_H_ */
