/*
 * tape.h
 *
 *  Created on: Feb 11, 2018
 *      Author: Jeff
 */

#ifndef PROGRAM_TAPE_H_
#define PROGRAM_TAPE_H_

#include <stddef.h>
#include <stdio.h>

#include "../codegen/tokenizer.h"
#include "../datastructure/expando.h"
#include "../datastructure/map.h"
#include "../datastructure/queue.h"
#include "../datastructure/queue2.h"
#include "instruction.h"

typedef struct {
  Ins ins;
  const Token *token;
} InsContainer;

typedef struct _Tape Tape;

typedef int (*TapeInsFn)(Tape *tape, Op op, const Token *token);
typedef int (*TapeInsTextFn)(Tape *tape, Op op, const char text[],
                             const Token *token);
typedef int (*TapeInsIntFn)(Tape *tape, Op op, int val, const Token *token);
typedef int (*TapeLabelFn)(Tape *tape, const Token *token);
typedef int (*TapeLabelTextFn)(Tape *tape, const char text[]);
typedef int (*TapeLabelQFn)(Tape *tape, const Token *token, Queue *tokens);
typedef int (*TapeLabelArgsFn)(Tape *tape, const Token *token, Q *args);

int tape_ins(Tape *tape, Op op, const Token *token);
int tape_ins_text(Tape *tape, Op op, const char text[], const Token *token);
int tape_ins_int(Tape *tape, Op op, int val, const Token *token);
int tape_ins_no_arg(Tape *tape, Op op, const Token *token);
int tape_ins_anon(Tape *tape, Op op, const Token *token);
int tape_ins_neg(Tape *tape, Op op, const Token *token);

int tape_label(Tape *tape, const Token *token);
int tape_label_text(Tape *tape, const char text[]);
int tape_anon_label(Tape *tape, const Token *token);
int tape_function_with_args(Tape *tape, const Token *token, Q *args);
int tape_anon_function_with_args(Tape *tape, const Token *token, Q *args);

int tape_module(Tape *tape, const Token *token);
int tape_class(Tape *tape, const Token *token);
int tape_class_with_parents(Tape *tape, const Token *token, Queue *tokens);
int tape_endclass(Tape *tape, const Token *token);

typedef struct {
  TapeInsFn ins, ins_no_arg, ins_neg, ins_anon;
  TapeInsTextFn ins_text;
  TapeInsIntFn ins_int;
  TapeLabelFn label, anon_label, module, class, endclass;
  TapeLabelTextFn label_text;
  TapeLabelQFn class_with_parents;
  TapeLabelArgsFn function_with_args, anon_function_with_args;
} TapeFns;

Tape *tape_create_fns(TapeFns *fns);
void tapefns_init(TapeFns *fns);

struct _Tape {
  Expando *instructions;
  const char *module_name;
  Map refs, classes, class_starts, class_ends, class_parents, fn_args;
  Queue class_prefs;

  TapeInsFn ins, ins_no_arg, ins_neg, ins_anon;
  TapeInsTextFn ins_text;
  TapeInsIntFn ins_int;
  TapeLabelFn label, anon_label, module, class, endclass;
  TapeLabelTextFn label_text;
  TapeLabelQFn class_with_parents;
  TapeLabelArgsFn function_with_args, anon_function_with_args;
};

void tape_init(Tape *tape, TapeFns *fns);
void tape_finalize(Tape *tape);

Tape *tape_create();
void tape_delete(Tape *tape);

int tape_insc(Tape *tape, const InsContainer *insc);
char *anon_fn_for_token(const Token *token);

InsContainer *tape_get_mutable(const Tape *tape, int i);
const InsContainer *tape_get(const Tape *tape, int i);
size_t tape_len(const Tape *const tape);

void insc_to_str(const InsContainer *c, FILE *file);
// Destroys tail.
void tape_append(Tape *head, Tape *tail);
void tape_read(Tape *const tape, Queue *tokens);
void tape_write_range(const Tape *const tape, int start, int end, FILE *file);
void tape_write(const Tape *const tape, FILE *file);
void tape_read_binary(Tape *const tape, FILE *file);
void tape_write_binary(const Tape *const tape, FILE *file);

const Map *tape_classes(const Tape *const tape);
const Map *tape_class_parents(const Tape *const tape);
const Map *tape_refs(const Tape *const tape);
const char *tape_modulename(const Tape *tape);

void tape_populate_mappings(const Tape *tape, Map *i_to_refs,
                            Map *i_to_class_starts, Map *i_to_class_ends);
void tape_clear_mappings(Map *i_to_refs, Map *i_to_class_starts,
                         Map *i_to_class_ends);

#endif /* PROGRAM_TAPE_H_ */
