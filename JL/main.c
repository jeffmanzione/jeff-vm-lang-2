/*
 * main.c
 *
 *  Created on: Sep 28, 2016
 *      Author: Jeff
 */

#include <stdio.h>
#include <stdlib.h>

#include "arena/arena.h"
#include "arena/strings.h"
#include "command/commandline.h"
#include "command/commandlines.h"
#include "datastructure/map.h"
#include "datastructure/set.h"
#include "element.h"
#include "error.h"
#include "file_load.h"
#include "interpreter/interpreter.h"
#include "memory/memory.h"
#include "memory/memory_graph.h"
#include "optimize/optimize.h"
#include "vm/vm.h"

int main(int argc, const char *argv[]) {
#ifdef DEBUG
  alloc_init();
#endif
  arenas_init();
  strings_init();
  CKey_init();

  ArgConfig *config = argconfig_create();
  argconfig_compile(config);
  argconfig_run(config);
  ArgStore *store = commandline_parse_args(config, argc, argv);

  optimize_init();

  Set modules;
  set_init_default(&modules);

  void load_src(void *ptr) {
    ASSERT(NOT_NULL(ptr));
    Module *module = load_fn((char *)ptr, store);
    set_insert(&modules, module);
  }
  set_iterate(argstore_sources(store), load_src);
  VM *vm = vm_create(store);

  if (argstore_lookup_bool(store, ArgKey__EXECUTE) ||
      argstore_lookup_bool(store, ArgKey__INTERPRETER)) {
    Element main_element = create_none();
    void load_module(void *ptr) {
      ASSERT(NOT_NULL(ptr));
      Element module = vm_add_module(vm, (Module *)ptr);
      if (NONE == main_element.type) {
        main_element = module;
      }
    }
    set_iterate(&modules, load_module);
    if (argstore_lookup_bool(store, ArgKey__EXECUTE)) {
      vm_start_execution(vm, main_element);
    }
    if (argstore_lookup_bool(store, ArgKey__INTERPRETER)) {
      printf(
          "Starting Interpreter.\nWrite code below. Press enter to "
          "evaluate.\n");
      fflush(stdout);
      interpret_from_file(stdin, "stdin", vm, interpret_statement);
      printf("Goodbye!\n");
      fflush(stdout);
    }
  }

  set_finalize(&modules);
  argstore_delete(store);
  argconfig_delete(config);
  vm_delete(vm);
  optimize_finalize();
  CKey_finalize();
  strings_finalize();
  arenas_finalize();

#ifdef DEBUG
  alloc_finalize();
  printf(
      "Maps: %d\nMap inserts: %d\nMap insert compares: %d\n"
      "Map lookups: %d\nMap lookup compares: %d\n",
      MAP__count, MAP__insert_count, MAP__insert_compares_count,
      MAP__lookup_count, MAP__lookup_compares_count);
  fflush(stdout);
#endif
  return EXIT_SUCCESS;
}
