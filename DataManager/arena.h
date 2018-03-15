/*
 * arena.h
 *
 *  Created on: Feb 7, 2018
 *      Author: Jeff
 */

#ifndef ARENA_H_
#define ARENA_H_

#include <stddef.h>

#ifdef DEBUG
#include <stdint.h>
#endif

#define ARENA_DEFINE(typename) \
extern Arena ARENA__##typename;  \
void *arena_alloc__##typename()

#define ARENA_DECLARE(typename) Arena ARENA__##typename

#define ARENA_INIT(typename) arena_init(&ARENA__##typename, sizeof(typename))
#define ARENA_FINALIZE(typename) arena_finalize(&ARENA__##typename)
#define ARENA_ALLOC(typename) (typename *) arena_alloc(&ARENA__##typename)
#define ARENA_DEALLOC(typename, ptr) arena_dealloc(&ARENA__##typename, ptr)

typedef struct Subarena_ Subarena;
typedef struct {
  Subarena *last;
  size_t alloc_sz;
  void *next, *end;
//  void *last_freed;
#ifdef DEBUG
  uint32_t requests, removes;
#endif
} Arena;

ARENA_DEFINE(Element);
ARENA_DEFINE(Node);
ARENA_DEFINE(NodeEdge);
ARENA_DEFINE(InsContainer);
ARENA_DEFINE(Token);

void arena_init(Arena *arena, size_t sz);
void arena_finalize(Arena *arena);
void *arena_alloc(Arena *arena);
//void arena_dealloc(Arena *arena, void *ptr);


void arenas_init();
void arenas_finalize();

#endif /* ARENA_H_ */
