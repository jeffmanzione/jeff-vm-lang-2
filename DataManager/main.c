/*
 * main.c
 *
 *  Created on: Sep 28, 2016
 *      Author: Jeff
 */

#include <stdio.h>
#include <stdlib.h>

#include "arena.h"
#include "element.h"
#include "file_load.h"
#include "map.h"
#include "memory.h"
#include "module.h"
#include "optimize.h"
#include "strings.h"
#include "vm.h"

int main() {
  alloc_init();
  arenas_init();
  strings_init();
  optimize_init();

//  alloc_set_verbose(true);
  Module *mainm = load_fn("main.jl");
  Module *io = load_fn("io.jm");
  Module *math = load_fn("math.jm");
  VM *vm = vm_create();
  vm_add_module(vm, io);
  vm_add_module(vm, math);
  Element main_element = vm_add_module(vm, mainm);
  vm_set_module(vm, main_element, 0);
  vm_maybe_initialize_and_execute(vm, mainm);

//  memory_graph_print(vm_get_graph(vm), stdout);
//  memory_graph_free_space((MemoryGraph*) vm_get_graph(vm));
//  memory_graph_print(vm_get_graph(vm), stdout);

//  FILE_OP_FN("test.out", "wb", (FILE *file) {
//    Buffer *buf = buffer_create(file, 128);
//    serialize_module(buf, math);
//    buffer_delete(buf);
//  });

  module_delete(mainm);
  module_delete(io);
  module_delete(math);

  vm_delete(vm);

  optimize_finalize();
  strings_finalize();
  arenas_finalize();

//  printf("HERE?\n");fflush(stdout);
  alloc_finalize();
#ifdef DEBUG
  printf("Maps: %d\nMap inserts: %d\nMap insert compares: %d\n"
      "Map lookups: %d\nMap lookup compares: %d\n", MAP__count,
      MAP__insert_count, MAP__insert_compares_count, MAP__lookup_count,
      MAP__lookup_compares_count);
#endif
  return EXIT_SUCCESS;
}
