/*
 * execute.h
 *
 *  Created on: Dec 16, 2016
 *      Author: Jeff
 */

#ifndef PROGRAM_INSTRUCTION_H_
#define PROGRAM_INSTRUCTION_H_

#include <stdint.h>
#include <stdio.h>

#include "../codegen/tokenizer.h"
#include "../element.h"

typedef enum {
  NOP,
  EXIT,
  RES,
  TGET,
  TLEN,
  SET,
  PUSH,
  PEEK,
  PSRS,  // PUSH+RES
  NOT,   // where !1 == Nil
  NOTC,  // C-like NOT, where !1 == 0
  GT,
  LT,
  EQ,
  NEQ,
  GTE,
  LTE,
  AND,
  OR,
  XOR,
  IF,
  IFN,
  JMP,
  NBLK,
  BBLK,
  RET,
  ADD,
  SUB,
  MULT,
  DIV,
  MOD,
  INC,
  DEC,
  SINC,
  CALL,
  TUPL,
  DUP,
  GOTO,
  PRNT,
  LMDL,
  GET,
  GTSH,  // GET+PUSH
  FLD,
  IS,
  ADR,
  RAIS,
  CTCH,
  // ARRAYS
  ANEW,
  AIDX,
  ASET,
  //
  CNST,
  SETC,
  SGET,
  // NOT A REAL OP
  OP_BOUND,
} Op;

typedef enum {
  NO_PARAM,
  VAL_PARAM,
  ID_PARAM,
  //  GOTO_PARAM,
  STR_PARAM,
} ParamType;

typedef struct {
  Op op;
  ParamType param;
  union {
    Value val;
    const char *id;
    const char *str;
  };
  uint16_t row, col;
} Ins;

Op op_type(const char word[]);
Ins instruction(Op);
Ins instruction_val(Op, Value);
Ins instruction_id(Op, const char[]);
// Ins instruction_goto(Op, uint32_t);
Ins instruction_str(Op, const char[]);
Ins noop_instruction();
void ins_to_str(Ins, FILE *);
Value token_to_val(Token *);

bool value_equals(const Value *v1, const Value *v2);

extern const char *instructions[];

#endif /* PROGRAM_INSTRUCTION_H_ */
