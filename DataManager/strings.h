/*
 * string_intern.h
 *
 *  Created on: Feb 11, 2018
 *      Author: Jeff
 */

#ifndef STRINGS_H_
#define STRINGS_H_

#include "set.h"

extern char *ROOT;
extern char *RESULT_VAL;
extern char *STACK;
extern char *INS_INDEX;
extern char *TMP_VAL;
extern char *IP_FIELD;
extern char *CALLER_KEY;
extern char *PARENT;
extern char *SAVED_BLOCKS;
extern char *INITIALIZED;
extern char *CURRENT_BLOCK;
extern char *OLD_RESVALS;
extern char *PARENT_MODULE;
extern char *PARENT_CLASS;
extern char *TRUE_KEYWORD;
extern char *FALSE_KEYWORD;
extern char *NIL_KEYWORD;
extern char *LENGTH_KEY;
extern char *THIS;
extern char *MODULE_FIELD;
extern char *MODULES;
extern char *CLASS_KEY;
extern char *NEW_KEY;
extern char *PARENT_KEY;
extern char *NAME_KEY;
extern char *CLASS_NAME;
extern char *OBJECT_NAME;
extern char *ARRAY_NAME;
extern char *STRING_NAME;
extern char *TUPLE_NAME;
extern char *FUNCTION_NAME;
extern char *METHOD_NAME;
extern char *MODULE_NAME;

typedef struct Chunk_ Chunk;

typedef struct {
  char *tail, *end;
  Chunk *chunk, *last;
  Set strings;
} Strings;

extern Strings strings;

void strings_init();
void strings_finalize();
char *strings_intern_range(const char str[], int start, int end);
char *strings_intern(const char str[]);

#endif /* STRINGS_H_ */
