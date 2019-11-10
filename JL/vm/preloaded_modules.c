/*
 * module_merge.c
 *
 *  Created on: Jan 5, 2019
 *      Author: Jeff
 */

#include "../element.h"
#include "../external/external.h"
#include "../external/net/net.h"
#include "../external/time/time.h"
#include "../shared.h"
#include "../threads/sync.h"

#ifdef DEBUG
#define PREDEF_SRC(name, src_prefix) const char name[] = #src_prefix ".jl";
#else
#define PREDEF_SRC(name, src_prefix) const char name[] = #src_prefix ".jb";
#endif

PREDEF_SRC(BUILTIN_SRC, builtin);
PREDEF_SRC(IO_SRC, io);
PREDEF_SRC(STRUCT_SRC, struct);
PREDEF_SRC(ERROR_SRC, error);
PREDEF_SRC(SYNC_SRC, sync);
PREDEF_SRC(NET_SRC, net);
PREDEF_SRC(MATH_SRC, math);
PREDEF_SRC(TIME_SRC, time);

const char *const PRELOADED[] = {
    //  BUILTIN_SRC,
    IO_SRC, STRUCT_SRC, ERROR_SRC, SYNC_SRC, NET_SRC, MATH_SRC, TIME_SRC};

int preloaded_size() { return sizeof(PRELOADED) / sizeof(PRELOADED[0]); }

void maybe_merge_existing_source(VM *vm, Element module_element,
                                 const char fn[]) {
  if (starts_with(fn, BUILTIN_SRC)) {
    add_builtin_external(vm, module_element);
    add_global_builtin_external(vm, module_element);
  } else if (starts_with(fn, IO_SRC)) {
    add_io_external(vm, module_element);
  } else if (starts_with(fn, SYNC_SRC)) {
    add_sync_external(vm, module_element);
  } else if (starts_with(fn, NET_SRC)) {
    add_net_external(vm, module_element);
  } else if (starts_with(fn, TIME_SRC)) {
    add_time_external(vm, module_element);
  }
}
