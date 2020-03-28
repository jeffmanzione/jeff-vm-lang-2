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

#include "../arena/strings.h"
#include "../codegen/tokenizer.h"
#include "../datastructure/map.h"
#include "../datastructure/queue.h"
#include "../error.h"
#include "../memory/memory.h"
#include "../shared.h"
#include "instruction.h"
#include "module.h"
#include "tape.h"

#define DEFAULT_PROGRAM_SIZE 256

struct Module_ {
  FILE *file;
  const char *fn;
  FileInfo *fi;
  Tape *tape;
};

Module *module_create(FileInfo *fi) {
  Module *m = ALLOC2(Module);
  m->file = NULL;
  m->fi = fi;
  m->fn = NULL;
  m->tape = NULL;
  return m;
}

Module *module_create_tape(FileInfo *fi, Tape *tape) {
  Module *m = module_create(fi);
  m->tape = tape;
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

FileInfo *module_fileinfo(const Module const *m) { return m->fi; }

DEB_FN(const char *, module_name, const Module const *m) {
  ASSERT(NOT_NULL(m));
  if (NULL == m->tape) {
    return "?";
  }
  return tape_modulename(m->tape);
}

Ins module_ins(const Module *m, uint32_t index) {
  ASSERT(NOT_NULL(m), tape_len(m->tape) > index);
  return module_insc(m, index)->ins;
}

const InsContainer *module_insc(const Module *m, uint32_t index) {
  ASSERT(NOT_NULL(m), tape_len(m->tape) > index);
  return tape_get(m->tape, (int)index);
}

int32_t module_ref(const Module *m, const char ref_name[]) {
  ASSERT(NOT_NULL(m), NOT_NULL(ref_name));
  void *ptr = map_lookup(module_refs(m), ref_name);
  if (NULL == ptr) {
    return -1;
  }
  return (int32_t)(uint32_t)ptr;
}

const Map *module_refs(const Module *m) {
  ASSERT(NOT_NULL(m));
  return tape_refs(m->tape);
}

const Map *module_fn_args(const Module *m) {
  ASSERT(NOT_NULL(m));
  return &m->tape->fn_args;
}

void module_iterate_classes(const Module *m, ClassAction action) {
  ASSERT(NOT_NULL(m));
  Set processed;
  set_init_default(&processed);

  // For each class
  void process_class(Pair * kv) {
    const char *class_name = (char *)kv->key;
    const Map *methods = (Map *)kv->value;
    // Check if already processed.
    if (set_lookup(&processed, methods)) {
      return;
    }
    // Check if the parents have been processed processed.
    void maybe_process_parent(void *ptr) {
      const char *parent_class_name = *((char **)ptr);
      Map *parent_methods = map_lookup(module_classes(m), parent_class_name);
      // If parent not processed, process it.
      if (NULL == parent_methods || set_lookup(&processed, parent_methods)) {
        return;
      }
      Pair kv2;
      kv2.key = parent_class_name;
      kv2.value = parent_methods;
      process_class(&kv2);
    }
    Expando *class_parents = map_lookup(module_class_parents(m), class_name);
    if (NULL != class_parents) {
      expando_iterate(class_parents, maybe_process_parent);
    }
    // Process class
    action((char *)kv->key, (const Map *)kv->value);
    set_insert(&processed, kv->value);
  }
  map_iterate(module_classes(m), process_class);

  set_finalize(&processed);
}

const Map *module_classes(const Module *m) {
  ASSERT(NOT_NULL(m));
  return tape_classes(m->tape);
}

const Map *module_class_parents(const Module *m) {
  ASSERT(NOT_NULL(m));
  return tape_class_parents(m->tape);
}

uint32_t module_size(const Module *m) {
  ASSERT(NOT_NULL(m));
  return tape_len(m->tape);
}

const Tape *module_tape(const Module *m) { return m->tape; }

void module_set_tape(Module *m, Tape *tape) { m->tape = tape; }

void module_delete(Module *m) {
  ASSERT_NOT_NULL(m);
  // Delete file and assert no more tokens
  if (NULL != m->fi) {
    file_info_delete(m->fi);
  }
  if (NULL != m->tape) {
    tape_delete(m->tape);
  }
  DEALLOC(m);
}

#endif
