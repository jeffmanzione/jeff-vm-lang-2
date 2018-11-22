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
#include "external/strings.h"
#include "element.h"
#include "error.h"
#include "file_load.h"
#include "graph/memory.h"
#include "module.h"
#include "optimize/optimize.h"
#include "vm.h"

void execute_code(VM *vm, Element m, Tape *tape, int num_ins) {
  vm_set_module(vm, m, tape_len(tape) - num_ins);
#ifdef DEBUG
  tape_write_range(tape, tape_len(tape) - num_ins, tape_len(tape), stdout);
  fflush(stdout);
#endif
  do {
//    ins_to_str(vm_current_ins(vm), stdout);
//    printf("\n");
//    fflush(stdout);
    if (!execute(vm)) {
      break;
    }
  } while (vm_get_ip(vm) < tape_len(module_tape(vm_get_module(vm).obj->module)));

  printf("<-- ");
  elt_to_str(vm_get_resval(vm), stdout);
  printf("\n");
  fflush(stdout);
}

int main(int argc, const char *argv[]) {
  alloc_init();
  //  alloc_set_verbose(true);
  arenas_init();
  strings_init();

  ArgConfig *config = argconfig_create();
  argconfig_compile(config);
  argconfig_run(config);
  ArgStore *store = commandline_parse_args(config, argc, argv);

  Set modules;
  set_init_default(&modules);

  optimize_init();

  void load_src(void *ptr) {
    ASSERT(NOT_NULL(ptr));
    Module *module = load_fn((char *) ptr, store);
    set_insert(&modules, module);
  }
  set_iterate(argstore_sources(store), load_src);
  VM *vm = vm_create(store);

  Element main_element = create_none();
  void load_module(void *ptr) {
    ASSERT(NOT_NULL(ptr));
    Element module = vm_add_module(vm, (Module*) ptr);
    if (NONE == main_element.type) {
      main_element = module;
    }
  }
  set_iterate(&modules, load_module);

  if (argstore_lookup_bool(store, ArgKey__EXECUTE)) {
    vm_set_module(vm, main_element, 0);
    vm_maybe_initialize_and_execute(vm, main_element.obj->module);
  }
  if (argstore_lookup_bool(store, ArgKey__INTERPRETER)) {
    printf(
        "Starting Interpreter.\nWrite code below. Press enter to evaluate.\n");
    fflush(stdout);
    load_file_jl(stdin, vm, execute_code);
  }

//  memory_graph_print(vm_get_graph(vm), stdout);
//  memory_graph_free_space((MemoryGraph*) vm_get_graph(vm));
//  memory_graph_print(vm_get_graph(vm), stdout);
  set_finalize(&modules);
  argstore_delete(store);
  argconfig_delete(config);
  vm_delete(vm);
  optimize_finalize();
  strings_finalize();
  arenas_finalize();

  alloc_finalize();
#ifdef DEBUG
  printf("Maps: %d\nMap inserts: %d\nMap insert compares: %d\n"
      "Map lookups: %d\nMap lookup compares: %d\n", MAP__count,
      MAP__insert_count, MAP__insert_compares_count, MAP__lookup_count,
      MAP__lookup_compares_count);
#endif
  return EXIT_SUCCESS;
}
