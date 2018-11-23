/*
 * interpreter.c
 *
 *  Created on: Nov 23, 2018
 *      Author: Jeffrey
 */

#include "interpreter.h"

#include <stdbool.h>
#include <stdio.h>

#include "../codegen/codegen.h"
#include "../codegen/parse.h"
#include "../codegen/syntax.h"
#include "../codegen/tokenizer.h"
#include "../instruction.h"
#include "../module.h"
#include "../vm.h"

int interpret_from_file_statement(Parser *p, VM *vm, Element m, Tape *t,
    InterpretFn fn) {
  SyntaxTree st = file_level_statement(p);
  int num_ins = codegen(&st, t);
  fn(vm, m, t, num_ins);
  expression_tree_delete(&st);
  return num_ins;
}

int interpret_from_file(FILE *f, const char filename[], VM *vm, InterpretFn fn) {
  FileInfo *fi = file_info_file(f);
  file_info_set_name(fi, filename);
  Parser p;
  parser_init(&p, fi);
  Tape *tape = tape_create();
  Module *module = module_create_tape(fi, tape);
  Element module_element = create_module(vm, module);
  int num_ins = 0;
  while (true) {
    num_ins += interpret_from_file_statement(&p, vm, module_element, tape, fn);
    if (tape_get(tape, tape_len(tape) - 1)->ins.op == EXIT) {
      break;
    }
  }
  parser_finalize(&p);
  module_delete(module);
  return num_ins;
}

void interpret_statement(VM *vm, Element m, Tape *tape, int num_ins) {
  vm_set_module(vm, m, tape_len(tape) - num_ins);
#ifdef DEBUG
  tape_write_range(tape, tape_len(tape) - num_ins, tape_len(tape), stdout);
  fflush(stdout);
#endif
  do {
//#ifdef DEBUG
//    ins_to_str(vm_current_ins(vm), stdout);
//    printf("\n");
//    fflush(stdout);
//#endif
    if (!execute(vm)) {
      break;
    }
  } while (vm_get_ip(vm) < tape_len(module_tape(vm_get_module(vm).obj->module)));
  fflush(stdout);
  fflush(stderr);
  printf("<-- ");
  elt_to_str(vm_get_resval(vm), stdout);
  printf("\n");
  fflush(stdout);
}
