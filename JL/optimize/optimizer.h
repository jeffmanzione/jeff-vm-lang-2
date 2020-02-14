/*
 * optimizer.h
 *
 *  Created on: Jan 13, 2018
 *      Author: Jeff
 */

#ifndef OPTIMIZER_H_
#define OPTIMIZER_H_

#include <stdint.h>

#include "../datastructure/expando.h"
#include "../datastructure/map.h"
#include "../program/instruction.h"
#include "../program/tape.h"

typedef struct {
  enum { SET_OP, REMOVE, SHIFT, SET_VAL, REPLACE } type;
  union {
    Op op;
    Ins ins;
  };
  struct {
    uint32_t start, end, insert_pos;
  };
  Value val;
} Adjustment;

typedef struct {
  const Tape *tape;
  Map i_to_adj, i_gotos, inserts;
  Map i_to_refs, i_to_class_starts, i_to_class_ends;
  Expando *adjustments;
} OptimizeHelper;

typedef void (*Optimizer)(OptimizeHelper *, const Tape const *, int, int);

void o_Remove(OptimizeHelper *oh, int index);
void o_Replace(OptimizeHelper *oh, int index, Ins ins);
void o_SetOp(OptimizeHelper *oh, int index, Op op);
void o_SetVal(OptimizeHelper *oh, int index, Op op, Value val);
void o_Shift(OptimizeHelper *oh, int start_index, int end_index, int new_index);

#endif /* OPTIMIZER_H_ */
