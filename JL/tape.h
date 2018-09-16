/*
 * tape.h
 *
 *  Created on: Feb 11, 2018
 *      Author: Jeff
 */

#ifndef TAPE_H_
#define TAPE_H_

#include <stddef.h>
#include <stdio.h>

#include "codegen/tokenizer.h"
#include "datastructure/expando.h"
#include "datastructure/map.h"
#include "datastructure/queue.h"
#include "instruction.h"

typedef struct {
  Ins ins;
  const Token *token;
} InsContainer;

typedef struct {
  Expando *ins;
  const char *module_name;
  Map refs, classes, class_starts, class_ends, class_parents;
  Queue class_prefs;
} Tape;

void tape_init(Tape *tape);
void tape_finalize(Tape *tape);

Tape *tape_create();
void tape_delete(Tape *tape);

int tape_insc(Tape *tape, const InsContainer *insc);
int tape_ins(Tape *tape, Op op, Token *token);
int tape_ins_text(Tape *tape, Op op, const char text[], Token *token);
int tape_ins_int(Tape *tape, Op op, int val, Token *token);
int tape_ins_no_arg(Tape *tape, Op op, Token *token);
int tape_ins_anon(Tape *tape, Op op, Token *token);

int tape_label(Tape *tape, Token *token);
int tape_anon_label(Tape *tape, Token *token);

int tape_module(Tape *tape, Token *token);
int tape_class(Tape *tape, Token *token);
int tape_class_with_parents(Tape *tape, Token *token, Queue *tokens);
int tape_endclass(Tape *tape, Token *token);

InsContainer *tape_get_mutable(const Tape *tape, int i);
const InsContainer *tape_get(const Tape *tape, int i);
size_t tape_len(const Tape * const tape);

void insc_to_str(const InsContainer *c, FILE *file);
// Destroys tail.
void tape_append(Tape *head, Tape *tail);
void tape_read(Tape * const tape, Queue *tokens);
void tape_write(const Tape * const tape, FILE *file);
void tape_read_binary(Tape * const tape, FILE *file);
void tape_write_binary(const Tape * const tape, FILE *file);

const Map *tape_classes(const Tape * const tape);
const Map *tape_class_parents(const Tape * const tape);
const Map *tape_refs(const Tape * const tape);
const char *tape_modulename(const Tape * tape);

void tape_populate_mappings(const Tape *tape, Map *i_to_refs,
    Map *i_to_class_starts, Map *i_to_class_ends);
void tape_clear_mappings(Map *i_to_refs, Map *i_to_class_starts,
    Map *i_to_class_ends);

#endif /* TAPE_H_ */
