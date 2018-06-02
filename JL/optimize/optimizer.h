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
#include "../instruction.h"
#include "../tape.h"

typedef struct {
  enum {
    SET_OP, REMOVE, SHIFT
  } type;
  union {
    Op op;
    struct {
      uint32_t start, end, insert_pos;
    };
  };
} Adjustment;

typedef struct {
  const Tape *tape;
  Map i_to_adj, i_gotos, inserts;
  Map i_to_refs, i_to_class_starts, i_to_class_ends;
  Expando *adjustments;
} OptimizeHelper;

typedef void (*Optimizer)(OptimizeHelper *, const Tape const *, int, int);

void o_Remove(OptimizeHelper *oh, int index);
void o_SetOp(OptimizeHelper *oh, int index, Op op);
void o_Shift(OptimizeHelper* oh, int start_index, int end_index, int new_index);

#endif /* OPTIMIZER_H_ */
