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
#include "../class.h"
#include "../codegen/codegen.h"
#include "../codegen/parse.h"
#include "../codegen/syntax.h"
#include "../codegen/tokenizer.h"
#include "../datastructure/map.h"
#include "../datastructure/queue2.h"
#include "../external/strings.h"
#include "../memory/memory_graph.h"
#include "../program/instruction.h"
#include "../program/module.h"
#include "../threads/thread.h"
#include "../vm/vm.h"

static VM *global_vm;
static Element global_main_module;

int interpret_from_file_statement(Parser *p, VM *vm, Thread *th, Element m,
                                  Tape *t, InterpretFn fn) {
  SyntaxTree st = file_level_statement(p);

  int num_ins = codegen(&st, t);

  // # actually executed. May include catch body.
  num_ins = fn(vm, th, m, t, num_ins);
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
    if (tape_get(tape, t_get_ip(thread) - 1)->ins.op == EXIT) {
      break;
    }
    fflush(stdout);
    fflush(stderr);
    printf("<-- ");
    vm_call_fn(vm, thread, vm_lookup_module(vm, BUILTIN_MODULE_NAME),
               obj_get_field(vm_lookup_module(vm, BUILTIN_MODULE_NAME),
                             strings_intern("str")));
    t_shift_ip(thread, 1);
    do {
#ifdef DEBUG
      ins_to_str(t_current_ins(thread), stdout);
      printf("\n");
      fflush(stdout);
#endif
      if (!execute(vm, thread)) {
        break;
      }
    } while (t_get_ip(thread) <
             tape_len(module_tape(t_get_module(thread).obj->module)));
    Element str_rep = t_get_resval(thread);
    if (!ISTYPE(str_rep, class_string)) {
      ERROR("Expected str() to return a String.");
    }
    String *to_s = String_extract(str_rep);
    printf("%*s\n", String_size(to_s), String_cstr(to_s));
    fflush(stdout);
  }
  parser_finalize(&p);
  module_delete(module);
  return num_ins;
}

int interpret_statement(VM *vm, Thread *t, Element m, Tape *tape, int num_ins) {
  t_set_module(t, m, tape_len(tape) - num_ins);
  // Prevents error from bailing out of main funciton. +1 for to jump over the
  // jump.
  vm_set_catch_goto(vm, t, t_get_ip(t) + num_ins + 1);

  // Adds catch body.
  // What token to use?
  Token *token = (Token *)tape_get(tape, tape_len(tape) - 1)->token;
  num_ins += tape->ins_int(tape, JMP, 5, token) +
             tape->ins_no_arg(tape, PUSH, token) +
             tape->ins_text(tape, LMDL, strings_intern("error"), token) +
             tape->ins_no_arg(tape, RES, token) +
             tape->ins_text(tape, PUSH, strings_intern("error"), token) +
             tape->ins_text(tape, CALL, strings_intern("display_error"), token);
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
  return num_ins;
}
