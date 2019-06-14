/*
 * interpreter.c
 *
 *  Created on: Nov 23, 2018
 *      Author: Jeffrey
 */

#include "interpreter.h"

#include <stdbool.h>
#include <stdint.h>

#include "../arena/strings.h"
#include "../codegen/codegen.h"
#include "../codegen/parse.h"
#include "../codegen/syntax.h"
#include "../codegen/tokenizer.h"
#include "../datastructure/map.h"
#include "../datastructure/queue2.h"
#include "../graph/memory_graph.h"
#include "../instruction.h"
#include "../module.h"
#include "../threads/thread.h"
#include "../vm.h"

static VM *global_vm;
static Element global_main_module;

int interpret_from_file_statement(Parser *p, VM *vm, Thread *th, Element m,
                                  Tape *t, InterpretFn fn) {
  SyntaxTree st = file_level_statement(p);
  int num_ins = codegen(&st, t);
  fn(vm, th, m, t, num_ins);
  expression_tree_delete(&st);
  return num_ins;
}

int label_fn(Tape *tape, Token *token) {
  int result = tape_label(tape, token);
  uint32_t fn_pos = (uint32_t)map_lookup(&tape->refs, token->text);
  Q *fn_args = (Q *)map_lookup(&tape->fn_args, (void *)fn_pos);
  memory_graph_set_field(global_vm->graph, global_main_module, token->text,
                         create_function(global_vm, global_main_module, fn_pos,
                                         token->text, fn_args));
  return result;
}

int anon_label_fn(Tape *tape, Token *token) {
  int result = tape_anon_label(tape, token);
  char *fn_name = anon_fn_for_token(token);
  uint32_t fn_pos = (uint32_t)map_lookup(&tape->refs, fn_name);
  Q *fn_args = (Q *)map_lookup(&tape->fn_args, (void *)fn_pos);
  memory_graph_set_field(
      global_vm->graph, global_main_module, fn_name,
      create_function(global_vm, global_main_module, fn_pos, fn_name, fn_args));
  return result;
}

void setup_tapefns(TapeFns *fns) {
  tapefns_init(fns);

  fns->label = label_fn;
  fns->anon_label = anon_label_fn;
}

int interpret_from_file(FILE *f, const char filename[], VM *vm,
                        InterpretFn fn) {
  global_vm = vm;
  FileInfo *fi = file_info_file(f);
  file_info_set_name(fi, filename);
  Parser p;
  parser_init(&p, fi);
  TapeFns fns;
  setup_tapefns(&fns);
  Tape *tape = tape_create_fns(&fns);
  Module *module = module_create_tape(fi, tape);
  global_main_module = vm_add_module(vm, module);
  module_set_tape(module, tape);
  memory_graph_set_field(vm->graph, global_main_module, NAME_KEY,
                         string_create(vm, module_name(module)));

  Element main_function =
      create_function(vm, global_main_module, 0, strings_intern("main"), NULL);
  // Set to true so we don't double-run it.
  memory_graph_set_field(vm->graph, global_main_module, INITIALIZED,
                         element_true(vm));
  Element thread_elt = create_thread_object(vm, main_function, create_none());
  Thread *thread = Thread_extract(thread_elt);

  // Simulate calling the main function. This makes sure the block hierarchy and
  // parent module are properly set.
  t_set_module(thread, global_main_module, 0);
  vm_call_fn(vm, thread, global_main_module, main_function);
  t_shift_ip(thread, 1);

  int num_ins = 0;
  while (true) {
    num_ins += interpret_from_file_statement(&p, vm, thread, global_main_module,
                                             tape, fn);
    if (tape_get(tape, tape_len(tape) - 1)->ins.op == EXIT) {
      break;
    }
    fflush(stdout);
    fflush(stderr);
    printf("<-- ");
    //  vm_call_fn(vm, t, vm_lookup_module(vm, BUILTIN_MODULE_NAME),
    //             obj_get_field(vm_lookup_module(vm, BUILTIN_MODULE_NAME),
    //                           strings_intern("str")));
    //  t_shift_ip(t, 1);
    //  do {
    //#ifdef DEBUG
    //    ins_to_str(t_current_ins(t), stdout);
    //    printf("\n");
    //    fflush(stdout);
    //#endif
    //    if (!execute(vm, t)) {
    //      break;
    //    }
    //  } while (t_get_ip(t) <
    //  tape_len(module_tape(t_get_module(t).obj->module)));
    elt_to_str(t_get_resval(thread), stdout);
    printf("\n");
    fflush(stdout);
  }
  parser_finalize(&p);
  module_delete(module);
  return num_ins;
}

void interpret_statement(VM *vm, Thread *t, Element m, Tape *tape,
                         int num_ins) {
  t_set_module(t, m, tape_len(tape) - num_ins);
#ifdef DEBUG
  tape_write_range(tape, tape_len(tape) - num_ins, tape_len(tape), stdout);
  fflush(stdout);
#endif
  do {
#ifdef DEBUG
    ins_to_str(t_current_ins(t), stdout);
    printf("\n");
    fflush(stdout);
#endif
    if (!execute(vm, t)) {
      break;
    }
  } while (t_get_ip(t) < tape_len(module_tape(t_get_module(t).obj->module)));
}
