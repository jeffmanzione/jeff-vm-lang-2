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

#ifndef WAIT_OBJECT_0
#define WAIT_OBJECT_0 0x00000000L
#endif
#ifndef WAIT_TIMEOUT
#define WAIT_TIMEOUT 0x00000102L
#endif

typedef unsigned __stdcall (*VoidFn)(void *);
typedef unsigned ThreadId;
typedef void *ThreadHandle;
typedef void *Mutex;
typedef void *Semaphore;
typedef void *Condition;
typedef unsigned long ulong;
typedef ulong WaitStatus;

typedef struct {
  Semaphore global;
  Mutex reader;
  int blocking_readers_count;
} RWLock;

// General functions
void sleep_thread(ulong duration);
int num_cpus();

// THREADS
ThreadHandle create_thread(VoidFn fn, void *arg, ThreadId *id);
WaitStatus thread_await(ThreadHandle id, ulong duration);
void thread_close(ThreadHandle id);

// MUTEX
Mutex mutex_create(const char *name);
WaitStatus mutex_await(Mutex id, ulong duration);
void mutex_release(Mutex id);
void mutex_close(Mutex id);

// Semaphore
Semaphore semaphore_create(int initial_count, int max_count);
WaitStatus semaphore_lock(Semaphore s, ulong duration);
void semaphore_unlock(Semaphore s);
void semaphore_close(Semaphore s);

//// Condition
// Condition condition_create(const char *name);
// void condition_await(Mutex id, ulong duration);
// void condition_signal(Mutex id);
// void condition_close(Mutex id);

// RW Lock
RWLock *create_rwlock();
void begin_read(RWLock *lock);
void begin_write(RWLock *lock);
void end_read(RWLock *lock);
void end_write(RWLock *lock);
void close_rwlock(RWLock *lock);

#endif /* THREADS_THREAD_INTERFACE_H_ */
