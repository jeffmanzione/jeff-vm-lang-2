/*
 * string_intern.h
 *
 *  Created on: Feb 11, 2018
 *      Author: Jeff
 */

#ifndef STRINGS_H_
#define STRINGS_H_

#include "../datastructure/set.h"
#include "../threads/thread_interface.h"
extern char *ADDRESS_KEY;
extern char *ARGS_KEY;
extern char *ARGS_NAME;
extern char *ARRAYLIKE_INDEX_KEY;
extern char *ARRAYLIKE_SET_KEY;
extern char *ARRAY_NAME;
extern char *BUILTIN_MODULE_NAME;
extern char *CALLER_KEY;
extern char *CALL_KEY;
extern char *CLASSES_KEY;
extern char *CLASS_KEY;
extern char *CLASS_NAME;
extern char *CONSTRUCTOR_KEY;
extern char *CURRENT_BLOCK;
extern char *DECONSTRUCTOR_KEY;
extern char *EMPTY_TUPLE_KEY;
extern char *EQ_FN_NAME;
extern char *ERROR_KEY;
extern char *ERROR_NAME;
extern char *EXTERNAL_FUNCTION_NAME;
extern char *EXTERNAL_METHOD_NAME;
extern char *FALSE_KEYWORD;
extern char *FUNCTION_NAME;
extern char *FUNCTIONS_KEY;
extern char *HAS_NEXT_FN_NAME;
extern char *INITIALIZED;
extern char *INS_INDEX;
extern char *IN_FN_NAME;
extern char *IP_FIELD;
extern char *IS_EXTERNAL_KEY;
extern char *IS_ITERATOR_BLOCK_KEY;
extern char *ITER_FN_NAME;
extern char *LENGTH_KEY;
extern char *METHODS_KEY;
extern char *METHOD_INSTANCE_NAME;
extern char *METHOD_KEY;
extern char *METHOD_NAME;
extern char *MODULES;
extern char *MODULE_FIELD;
extern char *MODULE_KEY;
extern char *MODULE_NAME;
extern char *NAME_KEY;
extern char *NEQ_FN_NAME;
extern char *NEXT_FN_NAME;
extern char *NIL_KEYWORD;
extern char *OBJECT_NAME;
extern char *OBJ_KEY;
extern char *OLD_RESVALS;
extern char *PARENT;
extern char *PARENTS_KEY;
extern char *PARENT_CLASS;
extern char *PARENT_MODULE;
extern char *RESULT_VAL;
extern char *ROOT;
extern char *SAVED_BLOCKS;
extern char *SELF;
extern char *STACK;
extern char *STACK_SIZE_NAME;
extern char *STRING_NAME;
extern char *THREADS_KEY;
extern char *TMP_VAL;
extern char *TRUE_KEYWORD;
extern char *TUPLE_NAME;

typedef struct Chunk_ Chunk;

typedef struct {
  char *tail, *end;
  Chunk *chunk, *last;
  Set strings;
  ThreadHandle mutex;
} Strings;

extern Strings strings;

void strings_init();
void strings_finalize();
char *strings_intern_range(const char str[], int start, int end);
char *strings_intern(const char str[]);

#endif /* STRINGS_H_ */
