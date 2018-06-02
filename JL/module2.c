/*
 * module.c
 *
 *  Created on: Jan 1, 2017
 *      Author: Dad
 */


#ifdef NEW_MODULE

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "codegen/tokenizer.h"
#include "datastructure/map.h"
#include "datastructure/queue.h"
#include "error.h"
#include "instruction.h"
#include "graph/memory.h"
#include "module.h"
#include "shared.h"
#include "arena/strings.h"
#include "tape.h"

#define DEFAULT_PROGRAM_SIZE 256

struct Module_ {
  FILE *file;
  const char *fn;
  FileInfo *fi;
  Tape *tape;
};
// Not sure why I need these...
//void module_set_filename(Module *m, const char fn[]);
//void module_load(Module *m);
//
//Module *module_create(const char fn[]) {
//  FILE *file = FILE_FN(fn, "r");
//  Module *m = module_create_file(file);
//  m->fn = NULL;
//  module_set_filename(m, fn);
//  return m;
//}
//
//Module *module_create_file(FILE *file) {
//  Module *m = ALLOC2(Module);
//  m->file = file;
//  m->fn = NULL;
//
//  m->fi = file_info_file(m->file);
//  m->tape = tape_create();
//  module_load(m);
//  return m;
//}

Module *module_create_tape(FileInfo *fi, Tape *tape) {
  Module *m = ALLOC2(Module);
  m->file = NULL;
  m->fi = fi;
  m->tape = tape;
  m->fn = NULL;
  return m;
}

void module_set_filename(Module *m, const char fn[]) {
  ASSERT(NOT_NULL(m), NOT_NULL(fn));
  m->fn = strings_intern(fn);
  if (NULL != m->fi) {
    file_info_set_name(m->fi, fn);
  }
}

const char *module_filename(const Module const *m) {
  ASSERT_NOT_NULL(m);
  if (NULL == m->fn && NULL != m->fi) {
    return file_info_name(m->fi);
  }
  return m->fn;
}

FileInfo *module_fileinfo(const Module const *m) {
  return m->fi;
}

//void module_set_name(Module *m, const char name[]) {
//  ASSERT_NOT_NULL(m);
//  m->name = strings_intern(name);
//}

const char *module_name(const Module const *m) {
  ASSERT_NOT_NULL(m);
  return tape_modulename(m->tape);
}

//void module_load(Module *m) {
//  Queue tokens;
//  queue_init(&tokens);
//  // Read file and tokenize it
//  tokenize(m->fi, &tokens, true);
////  DEBUGF("TOKENS=%d", queue_size(&tokens));
//  tape_read(m->tape, &tokens);
////  tape_write(m->tape, stdout);
////  DEBUGF("module_name=%s", module_name(m));
//  queue_shallow_delete(&tokens);
//}

Ins module_ins(const Module *m, uint32_t index) {
  ASSERT(NOT_NULL(m), tape_len(m->tape) > index);
  return module_insc(m, index)->ins;
}

const InsContainer *module_insc(const Module *m, uint32_t index) {
  ASSERT(NOT_NULL(m), tape_len(m->tape) > index);
  return tape_get(m->tape, (int) index);
}

int32_t module_ref(const Module *m, const char ref_name[]) {
  ASSERT(NOT_NULL(m), NOT_NULL(ref_name));
  void *ptr = map_lookup(module_refs(m), ref_name);
  if (NULL == ptr) {
    return -1;
  }
  return (int32_t) (uint32_t) ptr;
}

const Map *module_refs(const Module *m) {
  ASSERT(NOT_NULL(m));
  return tape_refs(m->tape);
}

const Map *module_classes(const Module *m) {
  ASSERT(NOT_NULL(m));
  return tape_classes(m->tape);
}

uint32_t module_size(const Module *m) {
  ASSERT(NOT_NULL(m));
  return tape_len(m->tape);
}

const Tape *module_tape(const Module *m) {
  return m->tape;
}

void module_delete(Module *m) {
  ASSERT_NOT_NULL(m);
  // Delete file and assert no more tokens
  if (NULL != m->fi) {
    file_info_delete(m->fi);
  }
  tape_delete(m->tape);
  DEALLOC(m);
}

#endif
