/*
 * commandlines.c
 *
 *  Created on: Jun 2, 2018
 *      Author: Jeff
 */

#include "commandlines.h"

#include <stdbool.h>

#include "../error.h"
#include "commandline_arg.h"

void argconfig_compile(ArgConfig * const config) {
  ASSERT(NOT_NULL(config));
  argconfig_add(config, ArgKey__OUT_MACHINE, "m", arg_bool(false));
  argconfig_add(config, ArgKey__OUT_BINARY, "b", arg_bool(false));
  argconfig_add(config, ArgKey__OPTIMIZE, "opt", arg_bool(true));
  argconfig_add(config, ArgKey__OUT_UNOPTIMIZED, "ou", arg_bool(false));
}

void argconfig_run(ArgConfig * const config) {
  ASSERT(NOT_NULL(config));
  argconfig_add(config, ArgKey__EXECUTE, "ex", arg_bool(true));
}
