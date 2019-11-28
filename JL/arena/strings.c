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

#include "../error.h"
#include "../memory/memory.h"
#include "../shared.h"

#define DEFAULT_CHUNK_SIZE 32488
#define DEFAULT_HASHTABLE_SIZE 4091

Strings strings;

char *ADDRESS_KEY;
char *ARGS_KEY;
char *ARGS_NAME;
char *ARRAYLIKE_INDEX_KEY;
char *ARRAYLIKE_SET_KEY;
char *ARRAY_NAME;
char *BUILTIN_MODULE_NAME;
char *CALLER_KEY;
char *CALL_KEY;
char *CLASSES_KEY;
char *CLASS_KEY;
char *CLASS_NAME;
char *CONSTRUCTOR_KEY;
char *CURRENT_BLOCK;
char *DECONSTRUCTOR_KEY;
char *EQ_FN_NAME;
char *ERROR_KEY;
char *ERROR_NAME;
char *EXTERNAL_FUNCTION_NAME;
char *EXTERNAL_METHOD_NAME;
char *FALSE_KEYWORD;
char *FUNCTION_NAME;
char *FUNCTIONS_KEY;
char *HAS_NEXT_FN_NAME;
char *INITIALIZED;
char *INS_INDEX;
char *IN_FN_NAME;
char *IP_FIELD;
char *IS_EXTERNAL_KEY;
char *IS_ITERATOR_BLOCK_KEY;
char *ITER_FN_NAME;
char *LENGTH_KEY;
char *METHODS_KEY;
char *METHOD_INSTANCE_NAME;
char *METHOD_KEY;
char *METHOD_NAME;
char *MODULES;
char *MODULE_FIELD;
char *MODULE_NAME;
char *NAME_KEY;
char *NEQ_FN_NAME;
char *NEXT_FN_NAME;
char *NIL_KEYWORD;
char *OBJECT_NAME;
char *OBJ_KEY;
char *OLD_RESVALS;
char *PARENT;
char *PARENTS_KEY;
char *PARENT_CLASS;
char *PARENT_MODULE;
char *RESULT_VAL;
char *ROOT;
char *SAVED_BLOCKS;
char *SELF;
char *STACK;
char *STACK_SIZE_NAME;
char *STRING_NAME;
char *THREADS_KEY;
char *TMP_VAL;
char *TRUE_KEYWORD;
char *TUPLE_NAME;

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
  ADDRESS_KEY = strings_intern("$adr");
  ARGS_KEY = strings_intern("$args");
  ARGS_NAME = strings_intern("args");
  ARRAYLIKE_INDEX_KEY = strings_intern("__index__");
  ARRAYLIKE_SET_KEY = strings_intern("__set__");
  ARRAY_NAME = strings_intern("Array");
  BUILTIN_MODULE_NAME = strings_intern("builtin");
  CALLER_KEY = strings_intern("$caller");
  CALL_KEY = strings_intern("call");
  CLASSES_KEY = strings_intern("classes");
  CLASS_KEY = strings_intern("class");
  CLASS_NAME = strings_intern("Class");
  CONSTRUCTOR_KEY = strings_intern("new");
  CURRENT_BLOCK = strings_intern("$block");
  DECONSTRUCTOR_KEY = strings_intern("$deconstructor");
  EQ_FN_NAME = strings_intern("eq");
  ERROR_KEY = strings_intern("$has_error");
  ERROR_NAME = strings_intern("Error");
  EXTERNAL_FUNCTION_NAME = strings_intern("ExternalFunction");
  EXTERNAL_METHOD_NAME = strings_intern("ExternalMethod");
  FALSE_KEYWORD = strings_intern("False");
  FUNCTION_NAME = strings_intern("Function");
  FUNCTIONS_KEY = strings_intern("functions");
  HAS_NEXT_FN_NAME = strings_intern("has_next");
  INITIALIZED = strings_intern("$initialized");
  INS_INDEX = strings_intern("$ins");
  IN_FN_NAME = strings_intern("__in__");
  IP_FIELD = strings_intern("$ip");
  IS_EXTERNAL_KEY = strings_intern("$is_external");
  IS_ITERATOR_BLOCK_KEY = strings_intern("$is_iterator_block");
  ITER_FN_NAME = strings_intern("iter");
  LENGTH_KEY = strings_intern("len");
  METHODS_KEY = strings_intern("$methods");
  METHOD_INSTANCE_NAME = strings_intern("MethodInstance");
  METHOD_KEY = strings_intern("method");
  METHOD_NAME = strings_intern("Method");
  MODULES = strings_intern("$modules");
  MODULE_FIELD = strings_intern("$module");
  MODULE_NAME = strings_intern("Module");
  NAME_KEY = strings_intern("name");
  NEQ_FN_NAME = strings_intern("neq");
  NEXT_FN_NAME = strings_intern("next");
  NIL_KEYWORD = strings_intern("Nil");
  OBJECT_NAME = strings_intern("Object");
  OBJ_KEY = strings_intern("obj");
  OLD_RESVALS = strings_intern("$old_resvals");
  PARENT = strings_intern("$parent");
  PARENTS_KEY = strings_intern("parents");
  PARENT_CLASS = strings_intern("parent_class");
  PARENT_MODULE = strings_intern("module");
  RESULT_VAL = strings_intern("$resval");
  ROOT = strings_intern("$root");
  SAVED_BLOCKS = strings_intern("$saved_blocks");
  SELF = strings_intern("self");
  STACK = strings_intern("$stack");
  STACK_SIZE_NAME = strings_intern("$stack_size");
  STRING_NAME = strings_intern("String");
  THREADS_KEY = strings_intern("$threads");
  TMP_VAL = strings_intern("$tmp");
  TRUE_KEYWORD = strings_intern("True");
  TUPLE_NAME = strings_intern("Tuple");
}

void strings_init() {
  strings.mutex = mutex_create(NULL);
  strings.chunk = strings.last = chunk_create();
  strings.tail = strings.chunk->block;
  strings.end = strings.tail + strings.chunk->sz;
  set_init(&strings.strings, DEFAULT_HASHTABLE_SIZE, string_hasher,
           string_comparator);
  strings_insert_constants();
}

void strings_finalize() {
  set_finalize(&strings.strings);
  chunk_delete(strings.chunk);
  mutex_close(strings.mutex);
}

char *strings_intern_range(const char str[], int start, int end) {
  char *tmp = ALLOC_ARRAY(char, end - start + 1);
  strncpy(tmp, str + start, end - start);
  tmp[end - start] = '\0';
  char *to_return = strings_intern(tmp);
  DEALLOC(tmp);
  return to_return;
}

char *strings_intern(const char str[]) {
  mutex_await(strings.mutex, INFINITE);
  char *str_lookup = (char *)set_lookup(&strings.strings, str);
  if (NULL != str_lookup) {
    mutex_release(strings.mutex);
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
  mutex_release(strings.mutex);
  return to_return;
}
