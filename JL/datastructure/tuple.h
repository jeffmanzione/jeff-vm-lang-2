/*
 * tuple.h
 *
 *  Created on: Jan 1, 2017
 *      Author: Dad
 */

#ifndef TUPLE_H_
#define TUPLE_H_

#include <stdint.h>
#include <stdio.h>

#include "../element.h"

typedef struct Tuple_ Tuple;

Tuple *tuple_create();
void tuple_add(Tuple *t, Element e);
Element tuple_get(const Tuple *t, uint32_t index);
uint32_t tuple_size(const Tuple *t);
void tuple_delete(Tuple *t);
void tuple_print(const Tuple *t, FILE *file);

#endif /* TUPLE_H_ */
