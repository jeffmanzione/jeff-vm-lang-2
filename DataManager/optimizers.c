/*
 * optimizers.c
 *
 *  Created on: Mar 3, 2018
 *      Author: Jeff
 */

#include "instruction.h"
#include "error.h" // Keep
#include "optimizer.h"
#include "tape.h"

void optimizer_ResPush(OptimizeHelper *oh, const Tape * const tape, int start,
    int end) {
  int i;
  for (i = start + 1; i < end; i++) {
    const InsContainer *first = tape_get(tape, i - 1);
    const InsContainer *second = tape_get(tape, i);
    if (RES == first->ins.op && NO_PARAM != first->ins.param
        && PUSH == second->ins.op && NO_PARAM == second->ins.param
        && NULL == map_lookup(&oh->i_gotos, (void *) (i - 1))) {
      o_Remove(oh, i);
      o_SetOp(oh, i - 1, PUSH);
    }
  }
}

void optimizer_SetRes(OptimizeHelper *oh, const Tape * const tape, int start,
    int end) {
  int i;
  for (i = start + 1; i < end; i++) {
    const InsContainer *first = tape_get(tape, i - 1);
    const InsContainer *second = tape_get(tape, i);
    if (SET == first->ins.op && ID_PARAM == first->ins.param
        && RES == second->ins.op && ID_PARAM == second->ins.param
        && first->token->text == second->token->text // same pointer because string interning
        && NULL == map_lookup(&oh->i_gotos, (void *) (i))) {
      o_Remove(oh, i);
    }
  }
}

void optimizer_GetPush(OptimizeHelper *oh, const Tape * const tape, int start,
    int end) {
  int i;
  for (i = start + 1; i < end; i++) {
    const InsContainer *first = tape_get(tape, i - 1);
    const InsContainer *second = tape_get(tape, i);
    if (GET == first->ins.op && NO_PARAM != first->ins.param
        && PUSH == second->ins.op && NO_PARAM == second->ins.param
        && NULL == map_lookup(&oh->i_gotos, (void *) (i - 1))) {
      o_Remove(oh, i);
      o_SetOp(oh, i - 1, GTSH);
    }
  }
}

void optimizer_JmpRes(OptimizeHelper *oh, const Tape * const tape, int start,
    int end) {
  int i;
  for (i = start + 1; i < end; i++) {
    const InsContainer *first = tape_get(tape, i - 1);
    const InsContainer *second = tape_get(tape, i);
    if (SET != first->ins.op || JMP != second->ins.op) {
      continue;
    }
    int32_t jmp_val = second->ins.val.int_val;
    if (jmp_val >= 0) {
      continue;
    }
    const InsContainer *jump_to_parent = tape_get(tape, i + jmp_val - 1);
    const InsContainer *jump_to = tape_get(tape, i + jmp_val);
    if (SET != jump_to_parent->ins.op || jump_to_parent->ins.id != first->ins.id
        || RES != jump_to->ins.op || ID_PARAM != jump_to->ins.param
        || first->ins.id != jump_to->ins.id) { // same pointer because string interning
      continue;
    }
    o_Remove(oh, i + jmp_val);
  }
}

void optimizer_PushRes(OptimizeHelper *oh, const Tape * const tape, int start,
    int end) {
  int i;
  for (i = start + 1; i < end; i++) {
    const InsContainer *first = tape_get(tape, i - 1);
    const InsContainer *second = tape_get(tape, i);
    if (PUSH == first->ins.op && RES == second->ins.op
        && first->ins.param == second->ins.param
        && first->token->text == second->token->text &&
        NULL == map_lookup(&oh->i_gotos, (void *) (i))) {
      o_Remove(oh, i);
      o_SetOp(oh, i - 1, PSRS);
    }
  }
}

void optimizer_ResPush2(OptimizeHelper *oh, const Tape * const tape, int start,
    int end) {
  int i;
  for (i = start + 1; i < end; i++) {
    const InsContainer *first = tape_get(tape, i - 1);
    const InsContainer *second = tape_get(tape, i);
    if (RES == first->ins.op && NO_PARAM == first->ins.param
        && PUSH == second->ins.op && NO_PARAM == second->ins.param
        && NULL == map_lookup(&oh->i_gotos, (void *) (i-1))) {
      o_Remove(oh, i);
      o_SetOp(oh, i - 1, PEEK);
    }
  }
}

void optimizer_GroupStatics(OptimizeHelper *oh, const Tape * const tape,
    int start, int end) {

}
