/*
 * interpreter.c
 *
 *  Created on: Nov 23, 2018
 *      Author: Jeffrey
 */

#include "interpreter.h"

#include <stdbool.h>

#include "../arena/strings.h"
#include "../codegen/codegen.h"
#include "../codegen/parse.h"
#include "../codegen/syntax.h"
#include "../codegen/tokenizer.h"
#include "../graph/memory_graph.h"
#include "../instruction.h"
#include "../module.h"
#include "../threads/thread.h"
#include "../vm.h"

int interpret_from_file_statement(Parser *p, VM *vm, Thread *th, Element m,
    Tape *t, InterpretFn fn) {
  SyntaxTree st = file_level_statement(p);
  int num_ins = codegen(&st, t);
  fn(vm, th, m, t, num_ins);
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

  Element main_function = create_function(vm, module_element, 0,
      strings_intern("main"));
  // Set to true so we don't double-run it.
  memory_graph_set_field(vm->graph, module_element, INITIALIZED,
      element_true(vm));
  Element thread = create_thread_object(vm, main_function, create_none());

  int num_ins = 0;
  while (true) {
    num_ins += interpret_from_file_statement(&p, vm, Thread_extract(thread),
        module_element, tape, fn);
    if (tape_get(tape, tape_len(tape) - 1)->ins.op == EXIT) {
      break;
    }
  }
  parser_finalize(&p);
  module_delete(module);
  return num_ins;
}

void interpret_statement(VM *vm, Thread *t, Element m, Tape *tape, int num_ins) {
  t_set_module(t, m, tape_len(tape) - num_ins);
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
    if (!execute(vm, t)) {
      break;
    }
  } while (t_get_ip(t)
      < tape_len(module_tape(t_get_module(t).obj->module)));
  fflush(stdout);
  fflush(stderr);
  printf("<-- ");
  elt_to_str(t_get_resval(t), stdout);
  printf("\n");
  fflush(stdout);
}
