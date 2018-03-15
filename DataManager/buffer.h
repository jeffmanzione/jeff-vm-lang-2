/*
 * buffer.h
 *
 *  Created on: Nov 10, 2017
 *      Author: Jeff
 */

#ifndef BUFFER_H_
#define BUFFER_H_

#include <stddef.h>
#include <stdio.h>

typedef struct Buffer_ Buffer;

Buffer *buffer_create(FILE *file, size_t size);
void buffer_delete(Buffer * const buffer);
void buffer_flush(Buffer * const buffer);
void buffer_write(Buffer * const buffer, const char *start, int num_bytes);

#endif /* BUFFER_H_ */
