/*
 * file_load.c
 *
 *  Created on: Jun 17, 2017
 *      Author: Jeff
 */

#include "file_load.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "arena/strings.h"
#include "codegen/codegen.h"
#include "codegen/parse.h"
#include "codegen/syntax.h"
#include "codegen/tokenizer.h"
#include "datastructure/queue.h"
#include "error.h"
#include "graph/memory.h"
#include "optimize/optimize.h"
#include "shared.h"
#include "tape.h"

Module *load_fn_jb(const char fn[], const ArgStore *store) {
  Tape *tape = tape_create();
  char *jb_fn = ALLOC_ARRAY2(char, strlen(fn) + 1);
  strcpy(jb_fn, fn);
  jb_fn[strlen(jb_fn) - 1] = 'b';
  FILE * file = FILE_FN(strings_intern(jb_fn), "rb");
  DEALLOC(jb_fn);
  tape_read_binary(tape, file);
  return module_create_tape(NULL, tape);
}

Module *load_fn_jm(const char fn[], const ArgStore *store) {
  bool out_binary = argstore_lookup_bool(store, ArgKey__OUT_BINARY);
  bool should_optimize = argstore_lookup_bool(store, ArgKey__OPTIMIZE);
  bool out_unoptimized = argstore_lookup_bool(store, ArgKey__OUT_UNOPTIMIZED);

  FileInfo *fi = file_info(fn);
  Tape *tape = tape_create();
  Queue tokens;
  queue_init(&tokens);
  tokenize(fi, &tokens, true);
  tape_read(tape, &tokens);
  queue_shallow_delete(&tokens);

  if (should_optimize) {
    if (out_unoptimized) {
      char *jc_fn = ALLOC_ARRAY2(char, strlen(fn) + 1);
      strcpy(jc_fn, fn);
      jc_fn[strlen(jc_fn) - 1] = 'c';
      FILE *file = FILE_FN(strings_intern(jc_fn), "w+");
      DEALLOC(jc_fn);

      tape_write(tape, file);
      fclose(file);
    }
    tape = optimize(tape);
  }

  if (out_binary) {
    char *jb_fn = ALLOC_ARRAY2(char, strlen(fn) + 1);
    strcpy(jb_fn, fn);
    jb_fn[strlen(jb_fn) - 1] = 'b';
    FILE * file = FILE_FN(strings_intern(jb_fn), "wb+");
    DEALLOC(jb_fn);
    tape_write_binary(tape, file);
    fclose(file);
  }
  Module *module = module_create_tape(fi, tape);
  module_set_filename(module, fn);
  return module;
}

int load_fileinfo_jl_line(Parser *p, VM *vm, Element m,  Tape *t,
    void (*fn)(VM *vm, Element m, Tape *tape, int num_ins)) {
  SyntaxTree st = file_level_statement(p);
  int num_ins = codegen(&st, t);
  fn(vm, m, t, num_ins);
  expression_tree_delete(&st);
  return num_ins;
}

int load_file_jl(FILE *f, VM *vm, void (*fn)(VM *vm, Element m, Tape *tape, int num_ins)) {
  FileInfo *fi = file_info_file(f);
  Parser p;
  parser_init(&p, fi);
  Tape *tape = tape_create();
  Module *module = module_create_tape(fi, tape);
  Element module_element = create_module(vm, module);
  int num_ins = 0;
  while (true) {
    num_ins += load_fileinfo_jl_line(&p, vm, module_element,  tape, fn);
    if (tape_get(tape, tape_len(tape) - 1)->ins.op == EXIT) {
      break;
    }
  }
  parser_finalize(&p);
  module_delete(module);
  return num_ins;
}

Module *load_fn_jl(const char fn[], const ArgStore* store) {
  bool should_optimize = argstore_lookup_bool(store, ArgKey__OPTIMIZE);
  bool out_machine = argstore_lookup_bool(store, ArgKey__OUT_MACHINE);
  bool out_binary = argstore_lookup_bool(store, ArgKey__OUT_BINARY);
  bool out_unoptimized = argstore_lookup_bool(store, ArgKey__OUT_UNOPTIMIZED);

  Module *module;
  FileInfo *fi = file_info(fn);
  SyntaxTree tree = parse_file(fi);
  Tape *tape = tape_create();

  codegen_file(&tree, tape);

  if (should_optimize) {
    if (out_unoptimized) {
      char *jc_fn = ALLOC_ARRAY2(char, strlen(fn) + 1);
      strcpy(jc_fn, fn);
      jc_fn[strlen(jc_fn) - 1] = 'c';
      FILE *file = FILE_FN(strings_intern(jc_fn), "w+");
      DEALLOC(jc_fn);

      tape_write(tape, file);
      fclose(file);
    }
    tape = optimize(tape);
  }

  if (out_machine) {
    char *jm_fn = ALLOC_ARRAY2(char, strlen(fn) + 1);
    strcpy(jm_fn, fn);
    jm_fn[strlen(jm_fn) - 1] = 'm';
    FILE * file = FILE_FN(strings_intern(jm_fn), "w+");
    DEALLOC(jm_fn);

    tape_write(tape, file);
    fclose(file);
  }

  if (out_binary) {
    char *jb_fn = ALLOC_ARRAY2(char, strlen(fn) + 1);
    strcpy(jb_fn, fn);
    jb_fn[strlen(jb_fn) - 1] = 'b';
    FILE * file = FILE_FN(strings_intern(jb_fn), "wb+");
    DEALLOC(jb_fn);

    tape_write_binary(tape, file);
    fclose(file);
  }

  expression_tree_delete(&tree);
  module = module_create_tape(fi, tape);
  return module;
}

Module *load_fn(const char fn[], const ArgStore *store) {
  if (ends_with(fn, ".jb")) {
    return load_fn_jb(fn, store);
  } else if (ends_with(fn, ".jm")) {
    return load_fn_jm(fn, store);
  } else if (ends_with(fn, ".jl")) {
    return load_fn_jl(fn, store);
  } else {
    ERROR("Cannot load file '%s'. File extension not understood.", fn);
  }
  return NULL;
}
