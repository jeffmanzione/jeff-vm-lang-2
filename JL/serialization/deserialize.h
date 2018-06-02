/*
 * deserialize.h
 *
 *  Created on: Nov 10, 2017
 *      Author: Jeff
 */

#ifndef DESERIALIZE_H_
#define DESERIALIZE_H_

#include <stdint.h>
#include <stdio.h>

#include "../datastructure/expando.h"
#include "../tape.h"

#define deserialize_type(file, type, p) deserialize_bytes(file, sizeof(type), ((char *)p), sizeof(type))

int deserialize_bytes(FILE *file, uint32_t num_bytes, char *buffer,
    uint32_t buffer_sz);
int deserialize_string(FILE *file, char *buffer, uint32_t buffer_sz);
int deserialize_ins(FILE *file, const Expando* const strings, InsContainer *c);

#endif /* DESERIALIZE_H_ */
