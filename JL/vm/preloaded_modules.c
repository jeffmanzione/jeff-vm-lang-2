/*
 * module_merge.c
 *
 *  Created on: Jan 5, 2019
 *      Author: Jeff
 */

#include "../command/commandline.h"
#include "../element.h"
#include "../external/external.h"
#include "../external/net/net.h"
#include "../external/time/time.h"
#include "../shared.h"
#include "../threads/sync.h"

#define ENDS_WITH_ANY(fn, src_prefix)                                      \
  (ends_with(fn, #src_prefix ".jl") || ends_with(fn, #src_prefix ".jm") || \
   ends_with(fn, #src_prefix ".jb"))

void maybe_merge_existing_source(VM *vm, Element module_element,
                                 const char fn[]) {
  const char *builtin_prefix =
      argstore_lookup_string(vm->store, ArgKey__BUILTIN_DIR);
  if (!starts_with(fn, builtin_prefix)) {
    return;
  }
  if (ENDS_WITH_ANY(fn, builtin)) {
    add_builtin_external(vm, module_element);
    add_global_builtin_external(vm, module_element);
  } else if (ENDS_WITH_ANY(fn, io)) {
    add_io_external(vm, module_element);
  } else if (ENDS_WITH_ANY(fn, sync)) {
    add_sync_external(vm, module_element);
  } else if (ENDS_WITH_ANY(fn, net)) {
    add_net_external(vm, module_element);
  } else if (ENDS_WITH_ANY(fn, time)) {
    add_time_external(vm, module_element);
  }
}
