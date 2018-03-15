/*
 * deserialize.h
 *
 *  Created on: Nov 10, 2017
 *      Author: Jeff
 */

#ifndef DESERIALIZE_H_
#define DESERIALIZE_H_

#include <stdint.h>

#include "buffer.h"
#include "instruction.h"

int32_t deserialize_ins(Buffer * const buffer, const Ins * const ins);
int deserialize_module(FILE * const file, Module * const module);

#endif /* DESERIALIZE_H_ */
