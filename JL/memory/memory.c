/*
 * memory.c
 *
 *  Created on: Sep 28, 2016
 *      Author: Jeff
 */

#include "../memory/memory.h"

#include <math.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "../datastructure/set.h"
#include "../error.h"
#include "../shared.h"
#include "../threads/thread_interface.h"

typedef struct {
  uint32_t elt_size;
  uint32_t count;
  uint32_t line;
  char *type_name;
  char *func;
  char *file;
} AllocInfo;

static bool is_verbose = false;
static Set *in_mem = NULL;
static bool alloc_busy = false;
static ThreadHandle alloc_mutex = NULL;

void alloc_init() {
  alloc_busy = true;
  ASSERT_NULL(in_mem);
  in_mem = set_create(32781, default_hasher, default_comparator);
  alloc_busy = false;
  alloc_mutex = mutex_create(NULL);
}

AllocInfo alloc_info(uint32_t elt_size, uint32_t count, uint32_t line,
    const char type_name[], const char func[], const char file[]) {
  AllocInfo info =
      { .elt_size = elt_size, .count = count, .line = line, .type_name = strdup(
          type_name), .func = strdup(func), .file = strdup(file) };
  return info;
}

void alloc_info_delete(AllocInfo *info, uint32_t line, const char func[],
    const char file[]) {
  if (NULL == info || NULL == info->type_name || NULL == info->file) {
    error(line, func, file, "Memory management error.");
  }
  ASSERT(NOT_NULL(info), NOT_NULL(info->type_name), NOT_NULL(info->file),
      NOT_NULL(info->func));
  free(info->type_name);
  free(info->file);
  free(info->func);
}

int alloc_info_size() {
  return ((int) ceil(((float) sizeof(AllocInfo)) / 8)) * 8;
}

void alloc_finalize() {
  bool alloc_val = alloc_busy;
  alloc_busy = true;
  ASSERT_NOT_NULL(in_mem);
  void embarrass_me(void *ptr) {
    ASSERT_NOT_NULL(ptr);
    AllocInfo *info = (AllocInfo *) (ptr - alloc_info_size());
    fprintf(stderr, "Forgot to free %p(%sx%d) allocated at %s:%d in %s(...)\n",
        ptr, info->type_name, info->count, info->file, info->line, info->func);
    fflush(stderr);
    DEALLOC(ptr);
  }
  set_iterate(in_mem, embarrass_me);
  set_delete(in_mem);
  mutex_close(alloc_mutex);
  alloc_busy = alloc_val;
}

void alloc_register(void *ptr, uint32_t elt_size, uint32_t count, uint32_t line,
    const char func[], const char file[], const char type_name[]) {
  mutex_await(alloc_mutex, INFINITE);
  if (!alloc_busy) {
    alloc_busy = true;
    if (!set_insert(in_mem, ptr)) {
      error(line, func, file,
          "Attempting to allocate %p(%sx%d), but it is already allocated.\n",
          ptr, type_name, count);
    }
    alloc_busy = false;
  }
  mutex_release(alloc_mutex);
}

void alloc_unregister(void *ptr, uint32_t line, const char func[],
    const char file[]) {
  mutex_await(alloc_mutex, INFINITE);
  if (!alloc_busy) {
    alloc_busy = true;
    if (!set_remove(in_mem, ptr)) {
      error(line, func, file,
          "Attempting to free %p, but it is not allocated.\n", ptr);
    }
    alloc_busy = false;
  }
  mutex_release(alloc_mutex);
}

void alloc_set_verbose(bool verbose) {
  is_verbose = verbose;
}

void log_alloc(uint32_t line, const char func[], const char file[],
    char format[], ...) {
  if (is_verbose) {
    va_list args;
    va_start(args, format);
    fprintf(stdout, "At %s:%d in %s(...):\t", file, line, func);
    vfprintf(stdout, format, args);
    fprintf(stdout, "\n");
    fflush(stdout);
    va_end(args);
  }
}

void *alloc__(uint32_t elt_size, uint32_t count, uint32_t line,
    const char func[], const char file[], const char type_name[]) {
  if (0 == elt_size || 0 == count) {
    error(line, func, file, "Either allocated array is of 0 elements or it is"
        " an array of type sizeof(0).");
  }
  int info_space = alloc_info_size();

  void *info_ptr = calloc(1, info_space + count * elt_size);
  ASSERT(NOT_NULL(info_ptr));

  *((AllocInfo *) info_ptr) = alloc_info(elt_size, count, line, type_name, func,
      file);

  if (NULL == info_ptr) {
    error(line, func, file, "Failed to allocate memory.");
  }
  void *ptr = info_ptr + info_space;

  alloc_register(ptr, elt_size, count, line, func, file, type_name);
  log_alloc(line, func, file, "Allocated a %s[%d] at %p", type_name, count,
      ptr);
  return ptr;
}

void *realloc__(void *ptr, uint32_t elt_size, uint32_t count, uint32_t line,
    const char func[], const char file[]) {
  mutex_await(alloc_mutex, INFINITE);
  if (NULL == ptr) {
    error(line, func, file, "Pointer argument was null.");
  }

  int new_size = elt_size * count;

  if (0 == new_size) {
    error(line, func, file, "Tried to realloc to an empty array.");
  }

  int info_space = alloc_info_size();
  void *info_ptr = ptr - info_space;

  AllocInfo old_info = *((AllocInfo *) (ptr - info_space));
  int old_size = old_info.elt_size * old_info.count;

  void *new_info_ptr = realloc(info_ptr, info_space + new_size);

  if (NULL == new_info_ptr) {
    error(line, func, file, "Failed to reallocate memory.");
  }

  *((AllocInfo *) new_info_ptr) = alloc_info(elt_size, count, line,
      old_info.type_name, func, file);

  void *new_ptr = new_info_ptr + info_space;

  if (new_size > old_size) {
    size_t diff = new_size - old_size;
    void* start = ((char *) new_ptr) + old_size;
    memset(start, 0, diff);
  }

  alloc_info_delete(&old_info, line, func, file);

  alloc_unregister(ptr, line, func, file);
  alloc_register(new_ptr, elt_size, count, line, func, file,
      ((AllocInfo *) new_info_ptr)->type_name);
  log_alloc(line, func, file, "Reallocated memory from %p to %p.", ptr,
      new_ptr);
  mutex_release(alloc_mutex);
  return new_ptr;
}

void dealloc__(void **ptr, uint32_t line, const char func[], const char file[]) {
  mutex_await(alloc_mutex, INFINITE);
  if (NULL == ptr || NULL == *ptr) {
    error(line, func, file, "Pointer argument was null.");
  }

  int info_space = alloc_info_size();
  void *info_ptr = *ptr - info_space;

  alloc_info_delete((AllocInfo *) info_ptr, line, func, file);
  free(info_ptr);

  alloc_unregister(*ptr, line, func, file);

  log_alloc(line, func, file, "Deallocated memory from %p", *ptr);
  *ptr = NULL;
  mutex_release(alloc_mutex);
}
