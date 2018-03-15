/*
 * deserialize.c
 *
 *  Created on: Nov 10, 2017
 *      Author: Jeff
 */

#include "deserialize.h"

#include <stdio.h>

#include "buffer.h"
#include "error.h"
#include "instruction.h"
#include "memory.h"
#include "module.h"
#include "shared.h"

//#define deserialize_type(buffer, type, i) ((type)((type *)(buffer + *i)))
//
//int deserialize_ins(Buffer * const buffer, const Ins * const ins) {
//  return 0;
//}
//
//char *read_string(char *buf, int *i) {
//  int s = *i;
//  while (buf[(*i)++] != NULL)
//    ;
//  return string_copy_range(buf, s, *i - 1);
//}
//
//int deserialize_module(FILE * const file, Module * const module) {
//  ASSERT_NOT_NULL(file);
//  fseek(file, 0, SEEK_END);
//  int file_len = ftell(file);
//  unsigned char buf[file_len + 1];
//  fseek(file, 0, SEEK_SET);
//  fread(buf, file_len, 1, file);
//  int i = 0;
//  char *name = read_string(buf, i);
//
//  return i;
//}
