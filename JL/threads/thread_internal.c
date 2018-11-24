/*
 * thread_internal.c
 *
 *  Created on: Nov 9, 2018
 *      Author: Jeff
 */

#include <process.h>
#include <stddef.h>
#include <windows.h>
#include "thread_interface.h"

ThreadHandle create_thread(VoidFn fn, void *arg, ThreadId *id) {
  return (ThreadHandle) _beginthreadex(NULL, 0, fn, arg, 0, id);
}

void wait_for_thread(ThreadHandle id, ulong duration) {
  WaitForSingleObject(id, duration);
}
void close_thread(ThreadHandle id) {
  CloseHandle(id);
}

ThreadHandle create_mutex(const char *name) {
  return CreateMutex(NULL, FALSE, name);
}

void wait_for_mutex(ThreadHandle id, ulong duration) {
  WaitForSingleObject(id, duration);
}

void release_mutex(ThreadHandle id) {
  ReleaseMutex(id);
}

void close_mutex(ThreadHandle id) {
  CloseHandle(id);
}

RWLock create_rwlock() {
  RWLock rwlock = { .global = create_mutex(NULL), .reader = create_mutex(NULL),
      .blocking_readers_count = 0 };
  return rwlock;
}

void begin_read(RWLock* lock) {
  wait_for_mutex(lock->reader, INFINITE);
  if (++lock->blocking_readers_count == 1) {
    wait_for_mutex(lock->global, INFINITE);
  }
  release_mutex(lock->reader);
}

void end_read(RWLock* lock) {
  wait_for_mutex(lock->reader, INFINITE);
  if (--lock->blocking_readers_count == 1) {
    release_mutex(lock->global);
  }
  release_mutex(lock->reader);
}

void begin_write(RWLock *lock) {
  wait_for_mutex(lock->global, INFINITE);
}

void end_write(RWLock *lock) {
  release_mutex(lock->global);
}

void close_rwlock(RWLock *lock) {
  release_mutex(lock->reader);
  release_mutex(lock->global);
  close_mutex(lock->reader);
  close_mutex(lock->global);

}
