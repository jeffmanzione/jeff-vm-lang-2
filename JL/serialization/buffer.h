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

typedef struct {
  FILE *file;
  unsigned char *buff;
  size_t pos, buff_size;
} WBuffer;

WBuffer *buffer_init(WBuffer * const buffer, FILE *file, size_t size);
void buffer_finalize(WBuffer * const buffer);
void buffer_flush(WBuffer * const buffer);
void buffer_write(WBuffer * const buffer, const char *start, int num_bytes);

#endif /* BUFFER_H_ */
