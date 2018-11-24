/*
 * thread_internal.h
 *
 *  Created on: Nov 9, 2018
 *      Author: Jeff
 */

#ifndef THREADS_THREAD_INTERFACE_H_
#define THREADS_THREAD_INTERFACE_H_

#ifndef INFINITE
#define INFINITE 0xFFFFFFFF
#endif
typedef unsigned __stdcall (*VoidFn)(void *);
typedef unsigned ThreadId;
typedef void *ThreadHandle;
typedef unsigned long ulong;

typedef struct {
  ThreadHandle global, reader;
  int blocking_readers_count;
} RWLock;

// THREADS
ThreadHandle create_thread(VoidFn fn, void *arg, ThreadId *id);
void wait_for_thread(ThreadHandle id, ulong duration);
void close_thread(ThreadHandle id);

// MUTEX
ThreadHandle create_mutex(const char *name);
void wait_for_mutex(ThreadHandle id, ulong duration);
void release_mutex(ThreadHandle id);
void close_mutex(ThreadHandle id);

// RW Lock
RWLock create_rwlock();
void begin_read(RWLock* lock);
void begin_write(RWLock *lock);
void end_read(RWLock* lock);
void end_write(RWLock *lock);
void close_rwlock(RWLock *lock);

#endif /* THREADS_THREAD_INTERFACE_H_ */
