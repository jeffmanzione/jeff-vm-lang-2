/*
 * module.c
 *
 *  Created on: Jan 1, 2017
 *      Author: Dad
 */

#ifndef NEW_MODULE

#include "module.h"

#include <stdbool.h>
#include <string.h>

#include "element.h"
#include "error.h"
#include "expando.h"
#include "memory.h"
#include "queue.h"
#include "shared.h"
#include "strings.h"

#define DEFAULT_PROGRAM_SIZE 256
#define MODULE_KEYWORD "module"
#define CLASS_KEYWORD "class"
#define CLASSEND_KEYWORD "endclass"

struct Module_ {
  char *fn, *name;
  FileInfo *fi;
  Queue queue;
  Map *classes;
  Map *class_ends;
  Queue class_prefs;
  Map *refs;
  Set *literals, *vars;
  Expando *program;
};
// Not sure why I need these...
void module_set_filename(Module *m, const char fn[]);
void module_set_name(Module *m, const char name[]);
void module_append_instruction(Module *m, Ins ins);

Module *module_create(const char fn[]) {
  FILE *file = FILE_FN(fn, "r");
  Module *m = module_create_file(file);
  module_set_filename(m, fn);
  return m;
}

Module *module_create_file(FILE *file) {
  Module *m = ALLOC2(Module);
  m->name = NULL;
  m->classes = map_create(DEFAULT_MAP_SZ, default_hasher, default_comparator);
  m->class_ends = map_create(DEFAULT_MAP_SZ, default_hasher, default_comparator);
  m->refs = map_create(DEFAULT_MAP_SZ, default_hasher, default_comparator);
  m->literals = set_create(DEFAULT_TABLE_SZ, default_hasher, default_comparator);
  m->vars = set_create(DEFAULT_TABLE_SZ, default_hasher, default_comparator);
  m->program = NULL;

  queue_init(&(m->queue));
  queue_init(&(m->class_prefs));

  // Read file and tokenize it
  m->fi = file_info_file(file);
  tokenize(m->fi, &(m->queue), true);

  // gets the module name if present
  if (0 == strcmp(MODULE_KEYWORD, ((Token *) queue_peek(&(m->queue)))->text)) {
    token_delete((Token *) queue_remove(&(m->queue)));
    Token *module_name = (Token *) queue_remove(&(m->queue));
    module_set_name(m, module_name->text);
    token_delete(module_name);
  } else {
    module_set_name(m, "$");
  }
//  DEBUGF("num_tokens=%d", m->queue.size);
  return m;
}

void module_set_filename(Module *m, const char fn[]) {
  ASSERT(NOT_NULL(m), NOT_NULL(fn));
  m->fn = strings_intern(fn);
  file_info_set_name(m->fi, fn);
}

const char *module_filename(const Module const *m) {
  ASSERT_NOT_NULL(m);
  return m->fn;
}

FileInfo *module_fileinfo(const Module const *m) {
  return m->fi;
}

void module_set_name(Module *m, const char name[]) {
  ASSERT_NOT_NULL(m);
  m->name = strings_intern(name);
}

const char *module_name(const Module const *m) {
  ASSERT_NOT_NULL(m);
  return m->name;
}

Token *skip_endlines(Queue *queue, uint32_t *row, uint32_t *col) {
  ASSERT_NOT_NULL(queue);
  ASSERT(queue->size > 0);
  Token *first = (Token *) queue_remove(queue);
  ASSERT_NOT_NULL(first);
  while (first->type == ENDLINE) {
    *row = first->line, *col = first->col;
    token_delete(first);
    first = (Token *) queue_remove(queue);
    if (NULL == first) {
      return NULL;
    }
  }
  return first;
}

Ins noop_with_ind(uint32_t row, uint32_t col) {
  Ins ins = noop_instruction();
  ins.row = row, ins.col = col;
  return ins;
}

Ins module_instruction(Module *m, Queue *queue, uint32_t index) {
  ASSERT_NOT_NULL(queue);
  ASSERT(queue->size > 0);
  uint32_t row = 0, col = 0;
  Token *first = (Token *) skip_endlines(queue, &row, &col);

  if (NULL == first) {
    return noop_with_ind(row, col);
  }
  ASSERT_NOT_NULL(first->text);
  // Handle named ins
  if (first->type == AT) {
    token_delete(first);
    Token *fname = (Token *) queue_remove(queue);
    ASSERT_NOT_NULL(fname);
    char *fncpy = fname->text;
    if (m->class_prefs.size > 0) {
      Map *obj_functions = map_lookup(m->classes,
          ((Token *) queue_peek(&(m->class_prefs)))->text);
      ASSERT_NOT_NULL(obj_functions);
      S_ASSERT(map_insert(obj_functions, fncpy, (void * ) index));
    } else {
      S_ASSERT(map_insert(m->refs, fncpy, (void * ) index));
    }
    token_delete(fname);

    first = (Token *) skip_endlines(queue, &row, &col);
    if (NULL == first) {
      return noop_with_ind(row, col);
    }
  }
  if (0 == strcmp(CLASS_KEYWORD, first->text)) {
    token_delete(first);
    Token *class_name = (Token *) queue_remove(queue);
    ASSERT(NOT_NULL(class_name), class_name->type == WORD);
    queue_add_front(&(m->class_prefs), class_name);
    map_insert(m->classes, class_name->text,
        map_create(DEFAULT_MAP_SZ, default_hasher, default_comparator));
    return noop_with_ind(row, col);
  }
  if (0 == strcmp(CLASSEND_KEYWORD, first->text)) {
    token_delete(first);
    Token *class_name = (Token *) queue_remove_last(&(m->class_prefs));
    map_insert(m->class_ends, class_name->text, (void *) index);
    token_delete(class_name);
    return noop_with_ind(row, col);
  }
  Op op = op_type(first->text);
  row = first->line, col = first->col;
  token_delete(first);
  Token *next = (Token *) queue_remove(queue);
  ASSERT_NOT_NULL(next);
  if (next->type == ENDLINE) {
    token_delete(next);
    Ins ins = instruction(op);
    ins.row = row, ins.col = col;
    return ins;
  }
  ASSERT_NOT_NULL(next->text);
  Ins ins;
  if (next->type == MINUS) {
    token_delete(next);
    next = (Token *) queue_remove(queue);
    ins = instruction_val(op, value_negate(token_to_val(next)));
  } else if (next->type == INTEGER || next->type == FLOATING) {
    ins = instruction_val(op, token_to_val(next));
  } else if (next->type == STR) {
//    DEBUGF("range=[%d,%d]", 1, max(1, strlen(next->text) - 1));
    char *inside_quotes_tmp = string_copy_range(next->text, 1,
        max(1, strlen(next->text) - 1));
    char *inside_quotes = strings_intern(inside_quotes_tmp);
    DEALLOC(inside_quotes_tmp);
//    DEBUGF("%s SRC='%s' '%s'", instructions[op], next->text, inside_quotes);
    char *ptr = set_lookup(m->literals, inside_quotes);
    if (NULL == ptr) {
      set_insert(m->literals, inside_quotes);
      ptr = inside_quotes;
    }
    ins = instruction_str(op, ptr);
  } else {
    const char *id = set_lookup(m->vars, next->text);
    if (NULL == id) {
      id = next->text;
    }
    set_insert(m->vars, id);
    ins = instruction_id(op, id);
  }
  token_delete(next);
  next = (Token *) queue_remove(queue);
  ASSERT_NOT_NULL(next);
  ASSERT(next->type == ENDLINE);
  token_delete(next);
  ins.row = row, ins.col = col;
  return ins;
}

void module_append_instruction(Module *m, Ins ins) {
  expando_append(m->program, &ins);
}

void module_load(Module *m) {
  m->program = expando(Ins, DEFAULT_PROGRAM_SIZE);
//  Ins noop = noop_with_ind(0, 0);
//  expando_append(m->program, &noop);
  // Iterate through tokens and interpret string instructions
  while (m->queue.size > 0) {
    Ins ins = module_instruction(m, &(m->queue), expando_len(m->program));
    if (NOP == ins.op) {
      continue;
    }
    module_append_instruction(m, ins);
    ;
  }
  // Delete all unsets. If. there are any, error
  bool is_error = false;
//  void delete_set(Pair *pair) {
//    ASSERT_NOT_NULL(pair);
//    ASSERT_NOT_NULL(pair->key);
//    ASSERT_NOT_NULL(pair->value);
//    void complain_not_set(void *ptr) {
//      is_error = true;
//      const LineInfo *info = file_info_lookup(m->fi, (uint32_t) ptr);
//      ASSERT_NOT_NULL(info);
//      fprintf(stderr, "At line %d, name '%s' is not declared:\n%s\n\n",
//          info->line_num, (char *) pair->key, info->line_text);
//    }
//    set_iterate((Set *) pair->value, complain_not_set);
//    set_delete((Set *) pair->value);
//  }
  ASSERT(m->queue.size == 0);
  // Safety first
  queue_deep_delete(&(m->queue), (Deleter) token_delete);
  queue_shallow_delete(&(m->class_prefs));

  if (is_error) {
    ERROR("There are errors.");
  }
}

Ins module_ins(const Module *m, uint32_t index) {
  ASSERT(NOT_NULL(m), NOT_NULL(m->program));
  return *((Ins *) expando_get(m->program, index));
}

int32_t module_ref(const Module *m, const char ref_name[]) {
  ASSERT(NOT_NULL(m), NOT_NULL(m->refs), NOT_NULL(ref_name));
  void *ptr = map_lookup(m->refs, ref_name);
  if (NULL == ptr) {
    return -1;
  }
  return (int32_t) (uint32_t) ptr;
}

const Map *module_refs(const Module *m) {
  ASSERT(NOT_NULL(m), NOT_NULL(m->refs));
  return m->refs;
}

const Map *module_classes(const Module *m) {
  ASSERT(NOT_NULL(m), NOT_NULL(m->classes));
  return m->classes;
}

Set *module_literals(const Module *m) {
  ASSERT(NOT_NULL(m), NOT_NULL(m->literals));
  return m->literals;
}

Set *module_vars(const Module *m) {
  ASSERT(NOT_NULL(m), NOT_NULL(m->vars));
  return m->vars;
}

uint32_t module_size(const Module *m) {
  ASSERT(NOT_NULL(m));
  return expando_len(m->program);
}

void module_delete(Module *m) {
  ASSERT_NOT_NULL(m);
  ASSERT_NOT_NULL(m->program);
  // Delete file and assert no more tokens
  file_info_delete(m->fi);

  void delete_inner(Pair *pair) {
    map_delete((Map *) pair->value);
  }
  map_iterate(m->classes, delete_inner);
  map_delete(m->classes);
  map_delete(m->class_ends);
  map_delete(m->refs);
  set_delete(m->literals);
  set_delete(m->vars);
  expando_delete(m->program);
  DEALLOC(m);
}

#endif
