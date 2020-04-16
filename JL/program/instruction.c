/*
 * instruction.c
 *
 *  Created on: Dec 16, 2016
 *      Author: Jeff
 */

#include "instruction.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "../arena/strings.h"
#include "../error.h"
#include "../ltable/ltable.h"

const char *instructions[] = {
    "nop",  "exit", "res",  "tget", "tlen", "set",  "let",  "push", "peek",
    "psrs", "not",  "notc", "gt",   "lt",   "eq",   "neq",  "gte",  "lte",
    "and",  "or",   "xor",  "if",   "ifn",  "jmp",  "nblk", "bblk", "ret",
    "add",  "sub",  "mult", "div",  "mod",  "inc",  "dec",  "sinc", "call",
    "clln", "tupl", "tgte", "tlte", "teq",  "dup",  "goto", "prnt", "lmdl",
    "get",  "gtsh", "rnil", "pnil", "fld",  "fldc", "is",   "adr",  "rais",
    "ctch", "anew", "aidx", "aset", "cnst", "setc", "letc", "sget"};

Op op_type(const char word[]) {
  int i;
  for (i = 0; i < sizeof(instructions) / sizeof(instructions[0]); ++i) {
    if (strlen(word) != strlen(instructions[i])) {
      continue;
    }
    if (0 == strncmp(word, instructions[i], 4)) {
      return i;
    }
  }
  ERROR("Unknown instruction type. Was '%s'.", word);
  return NOP;
}

Ins instruction(Op op) {
  Ins ins;
  ins.param = NO_PARAM;
  ins.op = op;
  ins.id = NULL;
  return ins;
}

Ins instruction_val(Op op, Value val) {
  Ins ins;
  ins.param = VAL_PARAM;
  ins.op = op;
  ins.val = val;
  return ins;
}

Ins instruction_id(Op op, const char id[]) {
  Ins ins;
  ins.param = ID_PARAM;
  ins.op = op;
  ins.id = id;
  return ins;
}

Ins instruction_str(Op op, const char *str) {
  Ins ins;
  ins.param = STR_PARAM;
  ins.op = op;
  ins.str = strings_intern_range(str, 1, strlen(str) - 1);

  return ins;
}

Ins noop_instruction() {
  Ins ins = {.op = NOP, .param = NO_PARAM, .val = {.type = INT, .int_val = 0}};
  return ins;
}

void ins_to_str(Ins ins, FILE *file) {
  fprintf(file, "%s", instructions[(int)ins.op]);
  fflush(file);
  if (ins.param == VAL_PARAM) {
    fprintf(file, " ");
    fflush(file);
    val_to_str(ins.val, file);
    if (ins.op == SGET) {
      fprintf(file, "(%s)", CKey_lookup_str(ins.val.int_val));
    }
    fflush(stdout);
  } else if (ins.param == ID_PARAM) {
    fprintf(file, " %s", ins.id);
    fflush(file);
    //  } else if (ins.param == GOTO_PARAM) {
    //    fprintf(file, " adr(%d)", ins.go_to);
  } else if (ins.param == STR_PARAM) {
    fprintf(file, " '%s'", ins.str);
    fflush(file);
  }
}

Value token_to_val(Token *tok) {
  ASSERT_NOT_NULL(tok);
  Value val;
  switch (tok->type) {
    case INTEGER:
      val.type = INT;
      val.int_val = (int64_t)strtoll(tok->text, NULL, 10);
      break;
    case FLOATING:
      val.type = FLOAT;
      val.float_val = strtod(tok->text, NULL);
      break;
    default:
      ERROR("Attempted to create a Value from '%s'.", tok->text);
  }
  return val;
}

bool value_equals(const Value *v1, const Value *v2) {
  ASSERT(NOT_NULL(v1), NOT_NULL(v2));
  if (v1->type != v2->type) {
    return false;
  }
  if (v1->type == INT) {
    return v1->int_val == v2->int_val;
  } else if (v1->type == FLOAT) {
    return v1->float_val == v2->float_val;
  } else {
    return v1->char_val == v2->char_val;
  }
}
