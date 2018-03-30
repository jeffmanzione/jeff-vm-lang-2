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

Module *load_fn_jb(const char fn[]) {
  Tape *tape = tape_create();
  char *jb_fn = ALLOC_ARRAY2(char, strlen(fn) + 1);
  strcpy(jb_fn, fn);
  jb_fn[strlen(jb_fn) - 1] = 'b';
  FILE * file = FILE_FN(strings_intern(jb_fn), "rb");
  DEALLOC(jb_fn);
  tape_read_binary(tape, file);
  return module_create_tape(NULL, tape);
}

Module *load_fn_jm(const char fn[], bool write_jb) {
  Module *module = module_create(fn);
  if (write_jb) {
    char *jb_fn = ALLOC_ARRAY2(char, strlen(fn) + 1);
    strcpy(jb_fn, fn);
    jb_fn[strlen(jb_fn) - 1] = 'b';
    FILE * file = FILE_FN(strings_intern(jb_fn), "wb+");
    DEALLOC(jb_fn);
    tape_write_binary(module_tape(module), file);
    fclose(file);
  }
  return module;
}

Module *load_fn_jl(const char fn[], bool should_optimize,
bool output_unoptimized, bool write_jm,
bool write_jb) {
  Module *module;
  FileInfo *fi = file_info(fn);
  ExpressionTree tree = parse_file(fi);
  Tape *tape = tape_create();

  codegen_file(&tree, tape);

  if (should_optimize) {
    if (output_unoptimized) {
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

  if (write_jm) {
    char *jm_fn = ALLOC_ARRAY2(char, strlen(fn) + 1);
    strcpy(jm_fn, fn);
    jm_fn[strlen(jm_fn) - 1] = 'm';
    FILE * file = FILE_FN(strings_intern(jm_fn), "w+");
    DEALLOC(jm_fn);

    tape_write(tape, file);
    fclose(file);
  }

  if (write_jb) {
    char *jb_fn = ALLOC_ARRAY2(char, strlen(fn) + 1);
    strcpy(jb_fn, fn);
    jb_fn[strlen(jb_fn) - 1] = 'b';
    FILE * file = FILE_FN(strings_intern(jb_fn), "wb+");
    DEALLOC(jb_fn);

    tape_write_binary(tape, file);
    fclose(file);
  }

//  {
//    tape_delete(tape);
//    tape = tape_create();
//    char *jb_fn = ALLOC_ARRAY2(char, strlen(fn) + 1);
//    strcpy(jb_fn, fn);
//    jb_fn[strlen(jb_fn) - 1] = 'b';
//    FILE * file = FILE_FN(strings_intern(jb_fn), "rb");
//    DEALLOC(jb_fn);
//    tape_read_binary(tape, file);
//  }

  expression_tree_delete(tree);
  module = module_create_tape(fi, tape);
  return module;
}

Module *load_fn(const char fn[]) {
  if (ends_with(fn, ".jb")) {
    return load_fn_jb(fn);
  } else if (ends_with(fn, ".jm")) {
    return load_fn_jm(fn, true);
  } else if (ends_with(fn, ".jl")) {
    Module *m = load_fn_jl(fn, true, true, true, true);
    return m;
  } else {
    ERROR("Cannot load file '%s'. File extension not understood.", fn);
  }
  return NULL;
}
