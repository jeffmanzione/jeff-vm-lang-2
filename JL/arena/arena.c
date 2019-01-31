/*
 * arena.c
 *
 *  Created on: Feb 10, 2018
 *      Author: Jeff
 */

#include "arena.h"

#include <math.h>
#include <stdlib.h>

#include "../codegen/tokenizer.h"
#include "../element.h"
#include "../error.h"
#include "../graph/memory.h"
#include "../graph/memory_graph.h"

ARENA_DECLARE(ElementContainer);
ARENA_DECLARE(Node);
ARENA_DECLARE(NodeEdge);
ARENA_DECLARE(Token);

#define DEFAULT_ELTS_IN_CHUNK 128

int descriptor_sz;

typedef struct {
  void *prev_freed;
} Descriptor;

struct Subarena_ {
  Subarena *prev;
  void *block;
  size_t block_sz;
};

void arenas_init() {
  ARENA_INIT(ElementContainer);
  ARENA_INIT(Node);
  ARENA_INIT(NodeEdge);
  ARENA_INIT(Token);
}

void arenas_finalize() {
  ARENA_FINALIZE(ElementContainer);
  ARENA_FINALIZE(Node);
  ARENA_FINALIZE(NodeEdge);
  ARENA_FINALIZE(Token);
}

Subarena *subarena_create(Subarena *prev, size_t sz) {
  Subarena *sa = ALLOC2(Subarena);
  sa->block_sz = sz * DEFAULT_ELTS_IN_CHUNK;
  sa->block = malloc(sa->block_sz);
  sa->prev = prev;
  return sa;
}

void subarena_delete(Subarena *sa) {
//  DEBUGF("Deleting subarena.");
  if (NULL != sa->prev) {
    subarena_delete(sa->prev);
  }
  free(sa->block);
  DEALLOC(sa);
}

void arena_init(Arena *arena, size_t sz) {
  ASSERT_NOT_NULL(arena);
  descriptor_sz = ((int) ceil(((float) sizeof(Descriptor)) / 4)) * 4;
//  printf("descripto_sz=%d\n", descriptor_sz);fflush(stdout);
  arena->mutex = mutex_create(NULL);
  arena->alloc_sz = sz + descriptor_sz;
  arena->last = subarena_create(NULL, arena->alloc_sz);
  arena->next = arena->last->block;
  arena->end = arena->last->block + arena->last->block_sz;
  arena->last_freed = NULL;
#ifdef DEBUG
  arena->requests = 0;
  arena->removes = 0;
#endif
}

void arena_finalize(Arena *arena) {
  ASSERT_NOT_NULL(arena);
  subarena_delete(arena->last);
  mutex_close(arena->mutex);
#ifdef DEBUG
  DEBUGF("Arena had %d requests, %d removals.", arena->requests,
      arena->removes);
#endif
}

void *arena_alloc(Arena *arena) {
  ASSERT_NOT_NULL(arena);
  mutex_await(arena->mutex, INFINITE);
#ifdef DEBUG
  arena->requests++;
#endif
  // Use up space that was already freed.
  if (NULL != arena->last_freed) {
    DEBUGF("REUSING FREED SPACE");
    void *free_spot = arena->last_freed;
    arena->last_freed = ((Descriptor *) free_spot)->prev_freed;
    mutex_release(arena->mutex);
    return free_spot + descriptor_sz;
  }
  // Allocate a new subarena if the current one is full.
  if (arena->next == arena->end) {
    Subarena *new_sa = subarena_create(arena->last, arena->alloc_sz);
    new_sa->prev = arena->last;
    arena->last = new_sa;
    arena->next = arena->last->block;
    arena->end = arena->last->block + arena->last->block_sz;
  }
  void *spot = arena->next;
  arena->next += arena->alloc_sz;
  mutex_release(arena->mutex);
  return spot + descriptor_sz;
}

void arena_dealloc(Arena *arena, void *ptr) {
  ASSERT(NOT_NULL(arena), NOT_NULL(ptr));
  mutex_await(arena->mutex, INFINITE);
#ifdef DEBUG
  arena->removes++;
#endif
  Descriptor *d = (Descriptor *) (ptr - descriptor_sz);
  d->prev_freed = arena->last_freed;
  arena->last_freed = (void *) d;
  mutex_release(arena->mutex);
}
