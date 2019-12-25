/*
 * file_load.c
 *
 *  Created on: Jun 17, 2017
 *      Author: Jeff
 */

#include "file_load.h"

#include <io.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "arena/strings.h"
#include "codegen/codegen.h"
#include "codegen/parse.h"
#include "codegen/syntax.h"
#include "codegen/tokenizer.h"
#include "datastructure/queue.h"
#include "error.h"
#include "memory/memory.h"
#include "optimize/optimize.h"
#include "program/tape.h"
#include "shared.h"

char *guess_file_extension(const char dir[], const char file_prefix[]) {
  size_t fn_len = strlen(dir) + strlen(file_prefix) + 3;
  if (!ends_with(dir, "/")) {
    fn_len++;
  }
  char *fn = ALLOC_ARRAY2(char, fn_len);
  char *pos = fn;
  strcpy(pos, dir);
  pos += strlen(dir);
  if (!ends_with(dir, "/")) {
    strcpy(pos++, "/");
  }
  strcpy(pos, file_prefix);
  pos += strlen(file_prefix);
  strcpy(pos, ".jb");
  if (access(fn, F_OK) != -1) {
    char *to_return = strings_intern(fn);
    DEALLOC(fn);
    return to_return;
  }
  strcpy(pos, ".jm");
  if (access(fn, F_OK) != -1) {
    char *to_return = strings_intern(fn);
    DEALLOC(fn);
    return to_return;
  }
  strcpy(pos, ".jl");
  if (access(fn, F_OK) != -1) {
    char *to_return = strings_intern(fn);
    DEALLOC(fn);
    return to_return;
  }
  DEALLOC(fn);
  return NULL;
}

void make_dir_if_does_not_exist(const char path[]) {
  ASSERT(NOT_NULL(path));
  struct stat st = {0};
  if (stat(path, &st) == -1) {
#ifdef _WIN32
    mkdir(path);
#else
    mkdir(path, 0700);
#endif
  }
}

Module *load_fn_jb(const char fn[], const ArgStore *store) {
  char *path, *file_name, *ext;
  split_path_file(fn, &path, &file_name, &ext);

  Tape *tape = tape_create();
  FILE *file = FILE_FN(combine_path_file(path, file_name, ".jb"), "rb");
  tape_read_binary(tape, file);
  return module_create_tape(NULL, tape);
}

Module *load_fn_jm(const char fn[], const ArgStore *store) {
  bool out_binary = argstore_lookup_bool(store, ArgKey__OUT_BINARY);
  bool should_optimize = argstore_lookup_bool(store, ArgKey__OPTIMIZE);
  bool out_unoptimized = argstore_lookup_bool(store, ArgKey__OUT_UNOPTIMIZED);
  const char *uoout_dir =
      argstore_lookup_string(store, ArgKey__UNOPTIMIZED_OUT_DIR);
  const char *bout_dir = argstore_lookup_string(store, ArgKey__BIN_OUT_DIR);

  char *path, *file_name, *ext;
  split_path_file(fn, &path, &file_name, &ext);

  FileInfo *fi = file_info(fn);
  Tape *tape = tape_create();
  Queue tokens;
  queue_init(&tokens);
  tokenize(fi, &tokens, true);
  tape_read(tape, &tokens);
  queue_shallow_delete(&tokens);
  file_info_close_file(fi);

  if (out_unoptimized) {
    make_dir_if_does_not_exist(uoout_dir);
  }
  if (out_binary) {
    make_dir_if_does_not_exist(bout_dir);
  }
  if (should_optimize) {
    if (out_unoptimized) {
      FILE *file =
          FILE_FN(combine_path_file(uoout_dir, file_name, ".jc"), "w+");
      tape_write(tape, file);
      fclose(file);
    }
    tape = optimize(tape);
  }

  if (out_binary) {
    FILE *file = FILE_FN(combine_path_file(bout_dir, file_name, ".jb"), "wb+");
    tape_write_binary(tape, file);
    fclose(file);
  }

  Module *module = module_create_tape(fi, tape);
  module_set_filename(module, fn);
  return module;
}

Module *load_fn_jl(const char fn[], const ArgStore *store) {
  bool should_optimize = argstore_lookup_bool(store, ArgKey__OPTIMIZE);
  bool out_machine = argstore_lookup_bool(store, ArgKey__OUT_MACHINE);
  bool out_binary = argstore_lookup_bool(store, ArgKey__OUT_BINARY);
  bool out_unoptimized = argstore_lookup_bool(store, ArgKey__OUT_UNOPTIMIZED);
  const char *mout_dir = argstore_lookup_string(store, ArgKey__MACHINE_OUT_DIR);
  const char *uoout_dir =
      argstore_lookup_string(store, ArgKey__UNOPTIMIZED_OUT_DIR);
  const char *bout_dir = argstore_lookup_string(store, ArgKey__BIN_OUT_DIR);

  char *path, *file_name, *ext;
  split_path_file(fn, &path, &file_name, &ext);

  Module *module;
  FileInfo *fi = file_info(fn);
  SyntaxTree tree = parse_file(fi);
  Tape *tape = tape_create();

  codegen_file(&tree, tape);
  file_info_close_file(fi);

  if (out_machine) {
    make_dir_if_does_not_exist(mout_dir);
  }
  if (out_unoptimized) {
    make_dir_if_does_not_exist(uoout_dir);
  }
  if (out_binary) {
    make_dir_if_does_not_exist(bout_dir);
  }

  if (should_optimize) {
    if (out_unoptimized) {
      FILE *file =
          FILE_FN(combine_path_file(uoout_dir, file_name, ".jc"), "w+");
      tape_write(tape, file);
      fclose(file);
    }
    tape = optimize(tape);
  }

  if (out_machine) {
    FILE *file = FILE_FN(combine_path_file(mout_dir, file_name, ".jm"), "wb+");
    tape_write(tape, file);
    fclose(file);
  }

  if (out_binary) {
    FILE *file = FILE_FN(combine_path_file(bout_dir, file_name, ".jb"), "wb+");
    tape_write_binary(tape, file);
    fclose(file);
  }

  syntax_tree_delete(&tree);
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
