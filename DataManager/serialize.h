/*
 * serialize.h
 *
 *  Created on: Jul 21, 2017
 *      Author: Jeff
 */

#ifndef SERIALIZE_H_
#define SERIALIZE_H_

#include "buffer.h"
#include "element.h"
#include "map.h"
#include "tape.h"

#define serialize_type(buffer, type, val) serialize_bytes(buffer, ((char *) (&(val))), sizeof(type))

// Returns number of chars written
int serialize_bytes(WBuffer *buffer, const char *start, int num_bytes);
int serialize_val(WBuffer *buffer, Value val);
int serialize_str(WBuffer *buffer, const char *str);
int serialize_ins(WBuffer * const buffer, const InsContainer *c,
    const Map * const string_index);
//int serialize_module(Buffer * const buffer, const Module * const module);

#endif /* SERIALIZE_H_ */
