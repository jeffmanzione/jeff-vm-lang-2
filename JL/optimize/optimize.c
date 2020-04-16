/*
 * optimize.c
 *
 *  Created on: Jan 13, 2018
 *      Author: Jeff
 */

#include "optimize.h"

#include <stddef.h>
#include <stdint.h>

#include "../codegen/tokenizer.h"
#include "../datastructure/expando.h"
#include "../datastructure/map.h"
#include "../datastructure/queue.h"
#include "../datastructure/set.h"
#include "../element.h"
#include "../error.h"
#include "../program/instruction.h"
#include "optimizers.h"

#define is_goto(op) \
  (((op) == JMP) || ((op) == IFN) || ((op) == IF) || ((op) == CTCH))

static Expando *optimizers = NULL;

void optimize_init() {
  optimizers = expando(Optimizer, DEFAULT_EXPANDO_SIZE);
  register_optimizer("ResPush", optimizer_ResPush);
  register_optimizer("SetRes", optimizer_SetRes);
  register_optimizer("SetPush", optimizer_SetPush);
  register_optimizer("JmpRes", optimizer_JmpRes);
  register_optimizer("PushRes", optimizer_PushRes);
  register_optimizer("ResPush2", optimizer_ResPush2);
  register_optimizer("RetRet", optimizer_RetRet);
  register_optimizer("PeekRes", optimizer_PeekRes);
  //  register_optimizer("Increment", optimizer_Increment);
  register_optimizer("SetEmpty", optimizer_SetEmpty);
  register_optimizer("PeekPeek", optimizer_PeekPeek);
  register_optimizer("PushResEmpty", optimizer_PushResEmpty);
  register_optimizer("PeekRes-SecondPass", optimizer_PeekRes);
  register_optimizer("PushRes2", optimizer_PushRes2);
  register_optimizer("SimpleMath", optimizer_SimpleMath);
  register_optimizer("GetPush", optimizer_GetPush);
  register_optimizer("Nil", optimizer_Nil);
  register_optimizer("GetSpecial", optimizer_GetSpecial);
}

void optimize_finalize() {
  expando_delete(optimizers);
}

void populate_gotos(OptimizeHelper *oh) {
  map_init_default(&oh->i_gotos);
  int i, len = tape_len(oh->tape);
  for (i = 0; i < len; i++) {
    const InsContainer *c = tape_get(oh->tape, i);
    if (!is_goto(c->ins.op)) {
      continue;
    }
    int index = i + (int) c->ins.val.int_val;
    map_insert(&oh->i_gotos, (void*) index, (void*) i);
  }
}

void oh_init(OptimizeHelper *oh, const Tape *tape) {
  oh->tape = tape;
  oh->adjustments = expando(Adjustment, 1024);
  map_init_default(&oh->i_to_adj);
  map_init_default(&oh->inserts);
  populate_gotos(oh);
  tape_populate_mappings(oh->tape, &oh->i_to_refs, &oh->i_to_class_starts,
      &oh->i_to_class_ends);
}

void oh_resolve(OptimizeHelper *oh, Tape *new_tape) {
  const Tape *t = oh->tape;
  Token tok;
  token_fill(&tok, WORD, 0, 0, tape_modulename(t));
  new_tape->module(new_tape, &tok);
  int i, old_len = tape_len(t);
  Expando *old_index = expando(int, DEFAULT_EXPANDO_SIZE);
  Expando *new_index = expando(int, DEFAULT_EXPANDO_SIZE);
  for (i = 0; i < old_len; i++) {
    char *text = NULL;
    if (NULL != (text = map_lookup(&oh->i_to_class_ends, (void* )i))) {
      Token tok;
      token_fill(&tok, WORD, 0, 0, text);
      new_tape->endclass(new_tape, &tok);
    }
    if (NULL != (text = map_lookup(&oh->i_to_class_starts, (void* )i))) {
      Token tok;
      token_fill(&tok, WORD, 0, 0, text);
      Expando *parents;
      if (NULL == (parents = map_lookup(&t->class_parents, text))) {
        new_tape->class(new_tape, &tok);
      } else {
        Queue q_parents;
        queue_init(&q_parents);
        void add_parent_class(void *ptr) {
          queue_add(&q_parents, *((char**) ptr));
        }
        expando_iterate(parents, add_parent_class);
        new_tape->class_with_parents(new_tape, &tok, &q_parents);
        queue_shallow_delete(&q_parents);
      }
    }
    if (NULL != (text = map_lookup(&oh->i_to_refs, (void* )i))) {
      Token tok;
      token_fill(&tok, WORD, 0, 0, text);
      Q *args = map_lookup(&t->fn_args, (void* )i);
      if (NULL == args) {
        new_tape->label(new_tape, &tok);
      } else {
        new_tape->function_with_args(new_tape, &tok, Q_copy(args));
      }
    }
    const InsContainer *c = tape_get(t, i);
    int new_len = tape_len(new_tape);
    Adjustment *insert = map_lookup(&oh->inserts, (void* )i);
    if (NULL != insert) {
      int j;
      for (j = insert->start; j < insert->end; j++) {
        expando_append(new_index, &new_len);
        expando_append(old_index, &j);
        tape_insc(new_tape, tape_get(t, j));
      }
    }
    Adjustment *a = map_lookup(&oh->i_to_adj, (void* )i);
    if (NULL != a && REMOVE == a->type) {
      int new_index_val = new_len - 1;
      expando_append(new_index, &new_index_val);
      continue;
    } else {
      expando_append(new_index, &new_len);
    }
    expando_append(old_index, &i);
    if (NULL == a) {
      tape_insc(new_tape, c);
      continue;
    }
    InsContainer c_new = *c;
    if (SET_OP == a->type) {
      c_new.ins.op = a->op;
    } else if (SET_VAL == a->type) {
      c_new.ins.op = a->op;
      c_new.ins.param = VAL_PARAM;
      c_new.ins.val = a->val;
    } else if (REPLACE == a->type) {
      c_new.ins = a->ins;
    }
    tape_insc(new_tape, &c_new);
  }
  int new_len = tape_len(new_tape);
  for (i = 0; i < new_len; i++) {
    InsContainer *c = tape_get_mutable(new_tape, i);
    if (!is_goto(c->ins.op)) {
      continue;
    }
    ASSERT(VAL_PARAM == c->ins.param);
    int diff = c->ins.val.int_val;
    int old_i = *((int*) expando_get(old_index, i));
    int old_goto_i = old_i + diff;
    int new_goto_i = *((int*) expando_get(new_index, old_goto_i));
    c->ins.val = create_int(new_goto_i - i).val;
  }
  expando_delete(old_index);
  expando_delete(new_index);
  tape_clear_mappings(&oh->i_to_refs, &oh->i_to_class_starts,
      &oh->i_to_class_ends);
  map_finalize(&oh->i_to_adj);
  map_finalize(&oh->inserts);
  map_finalize(&oh->i_gotos);
  expando_delete(oh->adjustments);
}

Tape* optimize(Tape *const t) {
  Tape *tape = t;
  int i, opts_len = expando_len(optimizers);
  for (i = 0; i < opts_len; i++) {
    OptimizeHelper oh;
    oh_init(&oh, tape);
    (*((Optimizer*) expando_get(optimizers, i)))(&oh, tape, 0, tape_len(tape));
    Tape *new_tape = tape_create();
    oh_resolve(&oh, new_tape);
    tape_delete(tape);
    tape = new_tape;
  }
  return tape;
}

void register_optimizer(const char name[], const Optimizer o) {
  expando_append(optimizers, &o);
}

// void InsContainer_swap(Expando * const e, void *x, void *y) {
//  InsContainer *cx = x;
//  InsContainer *cy = y;
//  InsContainer tmp = *cx;
//  *cx = *cy;
//  *cy = tmp;
//}

void Int32_swap(void *x, void *y) {
  int32_t *cx = x;
  int32_t *cy = y;
  int32_t tmp = *cx;
  *cx = *cy;
  *cy = tmp;
}

int Int32_compare(void *x, void *y) {
  return *((int32_t*) x) - *((int32_t*) y);
}
