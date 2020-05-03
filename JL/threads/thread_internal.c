/*
 * thread_internal.c
 *
 *  Created on: Nov 9, 2018
 *      Author: Jeff
 */
#include "thread_interface.h"

#include <process.h>
#include <stddef.h>
#include <windows.h>

#include "../memory/memory.h"

void sleep_thread(ulong duration) { Sleep(duration); }

int num_cpus() {
  SYSTEM_INFO sysinfo;
  GetSystemInfo(&sysinfo);
  return sysinfo.dwNumberOfProcessors;
}

ThreadHandle create_thread(VoidFn fn, void *arg, ThreadId *id) {
  return (ThreadHandle)_beginthreadex(NULL, 0, fn, arg, 0, id);
}

WaitStatus thread_await(ThreadHandle id, ulong duration) {
  return WaitForSingleObject(id, duration);
}
void thread_close(ThreadHandle id) { CloseHandle(id); }

Mutex mutex_create(const char *name) { return CreateMutex(NULL, FALSE, name); }

WaitStatus mutex_await(Mutex id, ulong duration) {
  return WaitForSingleObject(id, duration);
}

void mutex_release(Mutex id) { ReleaseMutex(id); }

void mutex_close(Mutex id) { CloseHandle(id); }

Semaphore semaphore_create(int initial_count, int max_count) {
  return CreateSemaphore(NULL, initial_count, max_count, NULL);
}

WaitStatus semaphore_lock(Semaphore s, ulong duration) {
  return WaitForSingleObject(s, duration);
}

void semaphore_unlock(Semaphore s) { ReleaseSemaphore(s, 1, NULL); }

void semaphore_close(Semaphore s) { CloseHandle(s); }

RWLock *create_rwlock() {
  RWLock tmp_rwlock = {.global = semaphore_create(1, 1),
                       .reader = mutex_create(NULL),
                       .blocking_readers_count = 0};
  RWLock *rwlock = ALLOC2(RWLock);
  *rwlock = tmp_rwlock;
  return rwlock;
}

void begin_read(RWLock *lock) {
  mutex_await(lock->reader, INFINITE);
  if (++lock->blocking_readers_count == 1) {
    while (semaphore_lock(lock->global, INFINITE) != WAIT_OBJECT_0)
      ;
  }
  mutex_release(lock->reader);
}

void end_read(RWLock *lock) {
  mutex_await(lock->reader, INFINITE);
  if (--lock->blocking_readers_count == 0) {
    semaphore_unlock(lock->global);
  }
  mutex_release(lock->reader);
}
void begin_write(RWLock *lock) {
  while (semaphore_lock(lock->global, INFINITE) != WAIT_OBJECT_0)
    ;
}

void end_write(RWLock *lock) { semaphore_unlock(lock->global); }

void close_rwlock(RWLock *lock) {
  mutex_release(lock->reader);
  semaphore_unlock(lock->global);
  mutex_close(lock->reader);
  semaphore_close(lock->global);
  DEALLOC(lock);
}
