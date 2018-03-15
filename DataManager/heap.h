/*
 * arraylist.h
 *
 *  Created on: Feb 3, 2018
 *      Author: Jeff
 */

#ifndef HEAP_H_
#define HEAP_H_

#include "element.h"

typedef struct _Heap Heap;

void heap_init(Heap * const heap);
void heap_finalize(Heap * const heap);

void heap_insert(Heap * const heap, const Element * const e);
void heap_remove(Heap * const heap, const Element * const e);

void heap_capacity(const Heap * const heap);
void heap_size(const Heap * const heap);

#endif /* HEAP_H_ */
