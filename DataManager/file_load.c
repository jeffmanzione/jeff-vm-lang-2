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

#include "codegen.h"
#include "error.h"
#include "expression.h"
#include "memory.h"
#include "optimize.h"
#include "parse.h"
#include "shared.h"
#include "strings.h"
#include "tape.h"
#include "tokenizer.h"

Module *load_fn_jm(const char fn[]) {
  Module *module = module_create(fn);
  return module;
}

Module *load_fn_jl(const char fn[], bool should_optimize, bool write_jm) {
  Module *module;
  FileInfo *fi = file_info(fn);
  ExpressionTree tree = parse_file(fi);
  Tape *tape = tape_create();

  codegen_file(&tree, tape);

  char *jc_fn = ALLOC_ARRAY2(char, strlen(fn) + 1);
  strcpy(jc_fn, fn);
  jc_fn[strlen(jc_fn) - 1] = 'c';
//  printf("Writing for file %s\n", jc_fn); fflush(stdout);
  FILE *file2 = FILE_FN(strings_intern(jc_fn), "w+");
  DEALLOC(jc_fn);

  tape_write(tape, file2);
  fclose(file2);

  tape = optimize(tape);
  char *jm_fn = ALLOC_ARRAY2(char, strlen(fn) + 1);
  strcpy(jm_fn, fn);
  jm_fn[strlen(jm_fn) - 1] = 'm';
//  printf("Writing for file %s\n", jm_fn); fflush(stdout);
  FILE * file = FILE_FN(strings_intern(jm_fn), "w+");
  DEALLOC(jm_fn);

  tape_write(tape, file);
  fclose(file);

  expression_tree_delete(tree);
  module = module_create_tape(fi, tape);
  return module;
}

Module *load_fn(const char fn[]) {
  if (ends_with(fn, ".jm")) {
    return load_fn_jm(fn);
  } else if (ends_with(fn, ".jl")) {
    Module *m = load_fn_jl(fn, true, true);
    return m;
  } else {
    ERROR("Cannot load file '%s'. File extension not understood.", fn);
  }
  return NULL;
}
