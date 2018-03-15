///*
// * write_ins.c
// *
// *  Created on: Dec 9, 2017
// *      Author: Jeff
// */
//
//#include <stdio.h>
//
//#include "error.h"
//#include "instruction.h"
//#include "tape.h"
//#include "tokenizer.h"
//
//#define OP_ARG_FMT    "  %-6s%s\n"
//#define OP_ARG_A_FMT  "  %-6s$anon_%d_%d\n"
//#define OP_ARG_I_FMT  "  %-6s%d\n"
//#define OP_NO_ARG_FMT "  %s\n"
//
////int write_ins(Tape *tape, FILE *target, Op op, const char arg[]) {
////  ASSERT(NOT_NULL(target), op < OP_BOUND);
////  if (NULL == arg) {
////    fprintf(target, OP_NO_ARG_FMT, instructions[op]);
////  } else {
////    fprintf(target, OP_ARG_FMT, instructions[op], arg);
////  }
////  return 1;
////}
//
//int write_ins(Tape *tape, FILE *target, Op op, Token *token) {
//  ASSERT(NOT_NULL(target), op < OP_BOUND);
//  fprintf(target, OP_ARG_FMT, instructions[op], token->text);
//  tape_ins(tape, token, op);
//  return 1;
//}
//
//int write_ins_text(Tape *tape, FILE *target, Op op, Token *token,
//    const char text[]) {
//  ASSERT(NOT_NULL(target), op < OP_BOUND);
//  fprintf(target, OP_ARG_FMT, instructions[op], text);
//  tape_ins_text(tape, token, op, text);
//  return 1;
//}
//
//int write_ins_int(Tape *tape, FILE *target, Op op, int val, Token *token) {
//  ASSERT(NOT_NULL(target), op < OP_BOUND);
//  fprintf(target, OP_ARG_I_FMT, instructions[op], val);
//  tape_ins_int(tape, token, op, val);
//  return 1;
//}
//
//int write_ins_no_arg(Tape *tape, FILE *target, Op op, Token *token) {
//  fprintf(target, OP_NO_ARG_FMT, instructions[op]);
//  tape_ins_no_arg(tape, token, op);
//  return 1;
//}
//
//int write_label(Tape *tape, FILE *target, Token *token) {
//  fprintf(target, "\n@%s\n", token->text);
//  tape_label(tape, token);
//  return 0;
//}
//
//int write_anon_label(Tape *tape, FILE *target, Token *token) {
//  fprintf(target, "\n@$anon_%d_%d\n", token->line, token->col);
//  tape_anon_label(tape, token);
//  return 0;
//}
//
//int write_ins_anon(Tape *tape, FILE *target, Op op, Token *token) {
//  fprintf(target, OP_ARG_A_FMT, instructions[op], token->line, token->col);
//  tape_ins_anon(tape, op, token);
//  return 1;
//}
//
//int write_module(Tape *tape, FILE *target, Token *token) {
//  fprintf(target, "module %s\n", token->text);
//  tape_module(tape, token);
//  return 0;
//}
//
//int write_class(Tape *tape, FILE *target, Token *token) {
//  fprintf(target, "class %s\n", token->text);
//  tape_class(tape, token);
//  return 0;
//}
//int write_endclass(Tape *tape, FILE *target, Token *token) {
//  fprintf(target, "endclass\n\n");
//  tape_endclass(tape, token);
//  return 0;
//}
