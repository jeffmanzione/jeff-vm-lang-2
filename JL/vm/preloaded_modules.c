/*
 * module_merge.c
 *
 *  Created on: Jan 5, 2019
 *      Author: Jeff
 */

#include "../element.h"
#include "../external/external.h"
#include "../external/net/net.h"
#include "../shared.h"
#include "../threads/sync.h"

#ifdef DEBUG
#define BUILTIN_SRC "builtin.jl"
#define IO_SRC      "io.jl"
#define STRUCT_SRC  "struct.jl"
#define ERROR_SRC   "error.jl"
#define SYNC_SRC    "sync.jl"
#define NET_SRC     "net.jl"
#else
#define BUILTIN_SRC "builtin.jb"
#define IO_SRC      "io.jb"
#define STRUCT_SRC  "struct.jb"
#define ERROR_SRC   "error.jb"
#define SYNC_SRC    "sync.jb"
#define NET_SRC     "net.jb"
#endif

const char *PRELOADED[] = {
//  BUILTIN_SRC,
    IO_SRC, STRUCT_SRC, ERROR_SRC, SYNC_SRC, NET_SRC };

int preloaded_size() {
  return sizeof(PRELOADED) / sizeof(PRELOADED[0]);
}

void maybe_merge_existing_source(VM *vm, Element module_element,
    const char fn[]) {
  if (starts_with(fn, BUILTIN_SRC)) {
    add_builtin_external(vm, module_element);
    add_global_builtin_external(vm, module_element);

//    void add_ref(Pair *pair) {
//      memory_graph_set_field(vm->graph, vm->root, pair->key,
//          *((Element *) pair->value));
//    }
//    map_iterate(&module_element.obj->fields, add_ref);
  } else if (starts_with(fn, IO_SRC)) {
    add_io_external(vm, module_element);
  } else if (starts_with(fn, SYNC_SRC)) {
    add_sync_external(vm, module_element);
  } else if (starts_with(fn, NET_SRC)) {
    add_net_external(vm, module_element);
  }
}
