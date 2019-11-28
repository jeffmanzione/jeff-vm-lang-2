/*
 * expando.h
 *
 *  Created on: Jan 14, 2018
 *      Author: Jeff
 */

#ifndef EXPANDO_H_
#define EXPANDO_H_

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h> // Keep

#include "../memory/memory.h" // Keep
#include "../shared.h" // Keep

typedef struct Expando_ Expando;

typedef void (*ESwapper)(void *, void *);
typedef void (*EAction)(void *);

#define DEFAULT_EXPANDO_SIZE 128

#define expando(type, table_sz) expando__(ALLOC_ARRAY(type, table_sz), sizeof(type), table_sz)

Expando * expando__(void *arr, size_t obj_sz, size_t table_sz);

int expando_append(Expando * const e, const void *v);

void expando_delete(Expando * const e);

void *expando_get(const Expando * const e, uint32_t i);

uint32_t expando_len(const Expando * const e);

void expando_sort(Expando * const e, Comparator c, ESwapper eswap);

void expando_iterate(const Expando * const e, EAction action);

#endif /* EXPANDO_H_ */
