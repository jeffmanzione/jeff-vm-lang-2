/*
 * Array.h
 *
 *  Created on: Jan 20, 2016
 *      Author: Jeff
 */

#ifndef ARRAY_H_
#define ARRAY_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "../element.h"

#define DEFAULT_TABLE_SIZE 64

typedef struct Array_ Array;

// public functions:
Array *array_create();
void array_delete(Array*);

void array_clear(Array* const);

void array_print(const Array * const, FILE *);

void array_push(Array* const, Element);
Element array_pop(Array* const);

void array_enqueue(Array* const, Element);
Element array_dequeue(Array* const);
//
//void array_insert(Array* const, uint32_t, Element);
//Element array_remove(Array* const, uint32_t);
//
void array_set(Array* const, uint32_t, Element);
Element array_get(Array* const, uint32_t);

uint32_t array_size(const Array* const);
bool array_is_empty(const Array* const);

Array *array_copy(const Array* const);
void array_append(Array* const head, const Array* const tail);
//Array *array_join(const Array * const array1, const Array * const array2);

#endif /* ARRAY_H_ */
