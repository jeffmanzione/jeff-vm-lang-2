/*
 * strings.c
 *
 *  Created on: Feb 11, 2018
 *      Author: Jeff
 */

#include "strings.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "error.h"
#include "memory.h"
#include "shared.h"

#define DEFAULT_CHUNK_SIZE 8122

Strings strings;

char *ROOT;
char *RESULT_VAL;
char *STACK;
char *INS_INDEX;
char *TMP_VAL;
char *IP_FIELD;
char *CALLER_KEY;
char *PARENT;
char *SAVED_BLOCKS;
char *INITIALIZED;
char *CURRENT_BLOCK;
char *OLD_RESVALS;
char *PARENT_MODULE;
char *PARENT_CLASS;
char *TRUE_KEYWORD;
char *FALSE_KEYWORD;
char *NIL_KEYWORD;
char *LENGTH_KEY;
char *THIS;
char *MODULE_FIELD;
char *MODULES;
char *CLASS_KEY;
char *NEW_KEY;
char *PARENT_KEY;
char *NAME_KEY;
char *CLASS_NAME;
char *OBJECT_NAME;
char *ARRAY_NAME;
char *STRING_NAME;
char *TUPLE_NAME;
char *FUNCTION_NAME;
char *METHOD_NAME;
char *MODULE_NAME;

struct Chunk_ {
  char *block;
  Chunk *next;
  size_t sz;
};

Chunk *chunk_create() {
  Chunk *chunk = ALLOC2(Chunk);
  chunk->sz = DEFAULT_CHUNK_SIZE;
  chunk->block = ALLOC_ARRAY2(char, chunk->sz);
  chunk->next = NULL;
  return chunk;
}

void chunk_delete(Chunk *chunk) {
  ASSERT_NOT_NULL(chunk);
  if (NULL != chunk->next) {
    chunk_delete(chunk->next);
  }
  DEALLOC(chunk->block);
  DEALLOC(chunk);
}

void strings_insert_constants() {
  ROOT = strings_intern("$root");
  RESULT_VAL = strings_intern("$resval");
  STACK = strings_intern("$stack");
  INS_INDEX = strings_intern("$ins");
  TMP_VAL = strings_intern("$tmp");
  IP_FIELD = strings_intern("$ip");
  CALLER_KEY = strings_intern("$caller");
  PARENT = strings_intern("$parent");
  SAVED_BLOCKS = strings_intern("$saved_blocks");
  INITIALIZED = strings_intern("$initialized");
  CURRENT_BLOCK = strings_intern("$block");
  OLD_RESVALS = strings_intern("$old_resvals");
  PARENT_MODULE = strings_intern("$module");
  PARENT_CLASS = strings_intern("parent_class");
  TRUE_KEYWORD = strings_intern("True");
  FALSE_KEYWORD = strings_intern("False");
  NIL_KEYWORD = strings_intern("Nil");
  LENGTH_KEY = strings_intern("len");
  THIS = strings_intern("self");
  MODULE_FIELD = strings_intern("module");
  MODULES = strings_intern("modules");
  CLASS_KEY = strings_intern("class");
  NEW_KEY = strings_intern("new");
  PARENT_KEY = strings_intern("parent");
  NAME_KEY = strings_intern("name");
  CLASS_NAME = strings_intern("Class");
  OBJECT_NAME = strings_intern("Object");
  ARRAY_NAME = strings_intern("Array");
  STRING_NAME = strings_intern("String");
  TUPLE_NAME = strings_intern("Tuple");
  FUNCTION_NAME = strings_intern("Function");
  METHOD_NAME = strings_intern("Method");
  MODULE_NAME = strings_intern("Module");
}

void strings_init() {
  strings.chunk = strings.last = chunk_create();
  strings.tail = strings.chunk->block;
  strings.end = strings.tail + strings.chunk->sz;
  set_init(&strings.strings, DEFAULT_TABLE_SZ, string_hasher,
      string_comparator);
  strings_insert_constants();
}

void strings_finalize() {
  set_finalize(&strings.strings);
  chunk_delete(strings.chunk);
}

char *strings_intern_range(const char str[], int start, int end) {
  char *tmp = ALLOC_ARRAY(char, end-start + 1);
  strncpy(tmp, str+start, end-start);
  tmp[end-start] = '\0';
  char *to_return = strings_intern(tmp);
  DEALLOC(tmp);
  return to_return;
}

char *strings_intern(const char str[]) {
  char *str_lookup = (char *) set_lookup(&strings.strings, str);
  if (NULL != str_lookup) {
    return str_lookup;
  }
  uint32_t len = strlen(str);
  if (strings.tail + len >= strings.end) {
    strings.last->next = chunk_create();
    strings.last = strings.last->next;
    strings.tail = strings.last->block;
    strings.end = strings.tail + strings.last->sz;
  }
  char *to_return = strings.tail;
  memmove(strings.tail, str, len + 1);
  strings.tail += (len + 1);
  set_insert(&strings.strings, to_return);
  return to_return;
}
