/*
 * tape.c
 *
 *  Created on: Feb 11, 2018
 *      Author: Jeff
 */

#include "tape.h"

#include <stdint.h>
#include <string.h>

#include "arena.h"
#include "element.h"
#include "error.h"
#include "memory.h"
#include "strings.h"

#define DEFAULT_TAPE_SIZE 1024

#define MODULE_KEYWORD "module"
#define CLASS_KEYWORD "class"
#define CLASSEND_KEYWORD "endclass"

#define OP_ARG_FMT    "  %-6s%s\n"
#define OP_ARG_A_FMT  "  %-6s$anon_%d_%d\n"
#define OP_ARG_I_FMT  "  %-6s%d\n"
#define OP_NO_ARG_FMT "  %s\n"

void tape_init(Tape *tape) {
//  DEBUGF("NEW TAPE");
  tape->ins = expando(InsContainer, DEFAULT_TAPE_SIZE);
  tape->module_name = NULL;
  map_init_default(&tape->refs);
  map_init_default(&tape->classes);
  map_init_default(&tape->class_starts);
  map_init_default(&tape->class_ends);
  queue_init(&tape->class_prefs);
}

Tape *tape_create() {
  Tape *tape = ALLOC2(Tape);
  tape_init(tape);
  return tape;
}

void tape_finalize(Tape *tape) {
  ASSERT_NOT_NULL(tape);
  expando_delete(tape->ins);
  map_finalize(&tape->refs);
  void delete_class_map(Pair *kv) {
    map_delete((Map *) kv->value);
  }
  map_iterate(&tape->classes, delete_class_map);
  map_finalize(&tape->classes);
  map_finalize(&tape->class_starts);
  map_finalize(&tape->class_ends);
  queue_shallow_delete(&tape->class_prefs);
}

void tape_delete(Tape *tape) {
  tape_finalize(tape);
  DEALLOC(tape);
}

InsContainer insc_create(Token *token) {
  InsContainer c; // = ARENA_ALLOC(InsContainer);
  c.token = token;
  return c;
}

int tape_insc(Tape *tape, const InsContainer *insc) {
  expando_append(tape->ins, insc);
  return 1;
}

int tape_ins(Tape *tape, Op op, Token *token) {
//  DEBUGF("tape_ins");
  InsContainer c = insc_create(token);
  switch (token->type) {
  case INTEGER:
  case FLOATING:
    c.ins = instruction_val(op, token_to_val(token));
    break;
  case STR:
    c.ins = instruction_str(op,
        strings_intern_range(token->text, 1, strlen(token->text) - 1));
    break;
  case WORD:
  default:
    c.ins = instruction_id(op, token->text);
    break;
//    DEBUGF("%s %s %d", instructions[op], token->text, token->type);
//    ERROR("Trying to make an instruction arg out of something"
//        " not a WORD, STR, INT, or FLOAT.");
  }
  ASSERT(NOT_NULL(token));
  return tape_insc(tape, &c);
}

int tape_ins_neg(Tape *tape, Op op, Token *token) {
  InsContainer c = insc_create(token);
  c.ins = instruction_val(op, value_negate(token_to_val(token)));
  ASSERT(NOT_NULL(token));
  return tape_insc(tape, &c);
}

int tape_ins_text(Tape *tape, Op op, const char text[], Token *token) {
//  DEBUGF("tape_ins_text");
  InsContainer c = insc_create(token);
  c.ins = instruction_id(op, text);
  ASSERT(NOT_NULL(token));
  return tape_insc(tape, &c);
}

int tape_ins_int(Tape *tape, Op op, int val, Token *token) {
//  DEBUGF("tape_ins_int");
  InsContainer c = insc_create(token);
  c.ins = instruction_val(op, create_int(val).val);
  return tape_insc(tape, &c);
}

int tape_ins_no_arg(Tape *tape, Op op, Token *token) {
//  DEBUGF("tape_ins_no_arg");
  InsContainer c = insc_create(token);
  c.ins = instruction(op);
  ASSERT(NOT_NULL(token));
  return tape_insc(tape, &c);
}

void insert_label_index(Tape * const tape, const char key[], int index) {
  if (queue_size(&tape->class_prefs) < 1) {
    map_insert(&tape->refs, key, (void *) index);
    return;
  }
  Map *methods = map_lookup(&tape->classes, queue_peek(&tape->class_prefs));
  map_insert(methods, key, (void *) index);
}

void insert_label(Tape * const tape, const char key[]) {
  insert_label_index(tape, key, expando_len(tape->ins));
}

int tape_label(Tape *tape, Token *token) {
//  DEBUGF("tape_label");
  insert_label(tape, token->text);
  return 0;
}

int tape_anon_label(Tape *tape, Token *token) {
//  DEBUGF("tape_anon_label");
  size_t needed = snprintf(NULL, 0, "$anon_%d_%d", token->line, token->col) + 1;
  char *buffer = ALLOC_ARRAY2(char, needed);
  snprintf(buffer, needed, "$anon_%d_%d", token->line, token->col);
  char *label = strings_intern(buffer);
  DEALLOC(buffer);

  insert_label(tape, label);
  return 0;
}

int tape_ins_anon(Tape *tape, Op op, Token *token) {
//  DEBUGF("tape_ins_anon");
  size_t needed = snprintf(NULL, 0, "$anon_%d_%d", token->line, token->col) + 1;
  char *buffer = ALLOC_ARRAY2(char, needed);
  snprintf(buffer, needed, "$anon_%d_%d", token->line, token->col);
  InsContainer c = insc_create(token);
  char *label = strings_intern(buffer);
  c.ins = instruction_id(op, label);
  ASSERT(NOT_NULL(token));
  DEALLOC(buffer);
  return tape_insc(tape, &c);
}

int tape_module(Tape *tape, Token *token) {
//  DEBUGF("tape_module");
  tape->module_name = token->text;
  return 0;
}

int tape_class(Tape *tape, Token *token) {
//  DEBUGF("tape_class(%s)", token->text);
  map_insert(&tape->class_starts, token->text, (void *) expando_len(tape->ins));
  map_insert(&tape->classes, token->text, map_create_default());
  queue_add_front(&tape->class_prefs, token->text);
  return 0;
}

int tape_endclass(Tape *tape, Token *token) {
//  DEBUGF("tape_endclass");
  char *cn = (char *) queue_remove(&tape->class_prefs);
  map_insert(&tape->class_ends, cn, (void *) expando_len(tape->ins));
  return 0;
}

InsContainer *tape_get_mutable(const Tape *tape, int i) {
  ASSERT(NOT_NULL(tape), i >= 0, i < expando_len(tape->ins));
  return (InsContainer *) expando_get(tape->ins, i);
}

const InsContainer *tape_get(const Tape *tape, int i) {
  ASSERT(NOT_NULL(tape), i >= 0, i < expando_len(tape->ins));
  return tape_get_mutable(tape, i);
}

// Destroys tail.
void tape_append(Tape *head, Tape *tail) {
//  DEBUGF("TAPE APPEND");
  ASSERT(NOT_NULL(head), NOT_NULL(tail));
  int i, cur_len = expando_len(head->ins), tail_len = expando_len(tail->ins);
  for (i = 0; i < tail_len; i++) {
    tape_insc(head, expando_get(tail->ins, i));
  }
  void insert_labels(Pair *kv) {
    insert_label_index(head, kv->key, ((uint32_t) kv->value) + cur_len);
  }
  map_iterate(&tail->refs, insert_labels);
  void insert_class_methods(Pair *kv) {
    Map *new_methods = map_create_default();
    map_insert(&head->classes, kv->key, new_methods);
    void insert_method(Pair *kv2) {
      map_insert(new_methods, kv2->key,
          (void*) ((uint32_t) kv2->value) + cur_len);
    }
    map_iterate(map_lookup(&tail->classes, kv->key), insert_method);
  }
  map_iterate(&tail->classes, insert_class_methods);
  void insert_class_starts(Pair *kv) {
    map_insert(&head->class_starts, kv->key,
        (void *) (((uint32_t) kv->value) + cur_len));
  }
  map_iterate(&tail->class_starts, insert_class_starts);
  void insert_class_ends(Pair *kv) {
    map_insert(&head->class_ends, kv->key,
        (void *) (((uint32_t) kv->value) + cur_len));
  }
  map_iterate(&tail->class_ends, insert_class_ends);

  ASSERT(queue_size(&tail->class_prefs) == 0);
}

size_t tape_len(const Tape * const tape) {
  return expando_len(tape->ins);
}

void insc_to_str(const InsContainer *c, FILE *file) {
//  printf("insc_to_str (%d %d)\n", c->ins.op, c->ins.param);fflush(stdout);

  fprintf(file, "  %-6s", instructions[(int) c->ins.op]);
  switch (c->ins.param) {
  case ID_PARAM:
    fprintf(file, "%s", c->ins.id);
    break;
  case STR_PARAM:
    fprintf(file, "'%s'", c->ins.str);
    break;
  case VAL_PARAM:
    val_to_str(c->ins.val, file);
    break;
  default:
    break;
  }
}

void tape_populate_mappings(const Tape *tape, Map *i_to_refs,
    Map *i_to_class_starts, Map *i_to_class_ends) {
  map_init_default(i_to_refs);
  map_init_default(i_to_class_starts);
  map_init_default(i_to_class_ends);
  void refs_reverse_index(Pair *kv) {
    map_insert(i_to_refs, kv->value, kv->key);
  }
  void class_starts_reverse_index(Pair *kv) {
    map_insert(i_to_class_starts, kv->value, kv->key);
  }
  void class_ends_reverse_index(Pair *kv) {
    map_insert(i_to_class_ends, kv->value, kv->key);
  }
  map_iterate(&tape->refs, refs_reverse_index);
  map_iterate(&tape->class_starts, class_starts_reverse_index);
  map_iterate(&tape->class_ends, class_ends_reverse_index);

  void class_refs_index(Pair *kv) {
    void class_refs_inner(Pair *kv) {
      map_insert(i_to_refs, kv->value, kv->key);
    }
    map_iterate(kv->value, class_refs_inner);
  }
  map_iterate(&tape->classes, class_refs_index);
}

void tape_clear_mappings(Map *i_to_refs,
    Map *i_to_class_starts, Map *i_to_class_ends) {
  map_finalize(i_to_refs);
  map_finalize(i_to_class_starts);
  map_finalize(i_to_class_ends);
}

void tape_write(const Tape * const tape, FILE *file) {
  ASSERT(NOT_NULL(tape), NOT_NULL(file));
  Map i_to_refs, i_to_class_starts, i_to_class_ends;
  tape_populate_mappings(tape, &i_to_refs, &i_to_class_starts,
      &i_to_class_ends);
  if (tape->module_name && 0 != strcmp("$", tape->module_name)) {
    fprintf(file, "module %s\n", tape->module_name);
  }
//  printf("BBBB\n"); fflush(stdout);
  uint32_t i;
  for (i = 0; i < tape_len(tape); i++) {
    char *text = NULL;
    if (map_lookup(&i_to_class_ends, (void *) i)) {
      fprintf(file, "endclass\n");
    }
    if (NULL != (text = map_lookup(&i_to_class_starts, (void *) i))) {
      fprintf(file, "\nclass %s\n", text);
    }
    if (NULL != (text = map_lookup(&i_to_refs, (void *) i))) {
      fprintf(file, "\n@%s\n", text);
    }
    const InsContainer *c = tape_get(tape, i);
    insc_to_str(c, file);
    fprintf(file, "\n");
  }
  tape_clear_mappings(&i_to_refs, &i_to_class_starts, &i_to_class_ends);
}

Token *next_token_skip_ln(Queue *queue) {
  ASSERT_NOT_NULL(queue);
  ASSERT(queue->size > 0);
  Token *first = (Token *) queue_remove(queue);
  ASSERT_NOT_NULL(first);
  while (first->type == ENDLINE) {
    first = (Token *) queue_remove(queue);
    if (NULL == first) {
      return NULL;
    }
  }
  return first;
}

void tape_read_ins(Tape * const tape, Queue *tokens) {
  ASSERT_NOT_NULL(tokens);
  if (queue_size(tokens) < 1) {
    return;
  }
  Token *first = next_token_skip_ln(tokens);
  if (NULL == first) {
    return;
  }
  if (AT == first->type) {
    tape_label(tape, queue_remove(tokens));
    return;
  }
  if (0 == strcmp(CLASS_KEYWORD, first->text)) {
    tape_class(tape, queue_remove(tokens));
    return;
  }
  if (0 == strcmp(CLASSEND_KEYWORD, first->text)) {
    tape_endclass(tape, first);
    return;
  }
  Op op = op_type(first->text);
  Token *next = (Token *) queue_remove(tokens);
  if (ENDLINE == next->type) {
    tape_ins_no_arg(tape, op, first);
    return;
  }
  if (MINUS == next->type) {
    tape_ins_neg(tape, op, queue_remove(tokens));
    return;
  }
  tape_ins(tape, op, next);
}

void tape_read(Tape * const tape, Queue *tokens) {
  if (0 == strcmp(MODULE_KEYWORD, ((Token *) queue_peek(tokens))->text)) {
    queue_remove(tokens);
    Token *module_name = (Token *) queue_remove(tokens);
    tape->module_name = module_name->text;
  } else {
    tape->module_name = strings_intern("$");
  }

  while (queue_size(tokens) > 0) {
    tape_read_ins(tape, tokens);
  }
}

const Map *tape_classes(const Tape * const tape) {
  ASSERT(NOT_NULL(tape));
  return &tape->classes;
}

const Map *tape_refs(const Tape * const tape) {
  ASSERT(NOT_NULL(tape));
  return &tape->refs;
}

const char *tape_modulename(const Tape * tape) {
  if (NULL == tape->module_name) {
    return strings_intern("$");
  }
  return tape->module_name;
}
