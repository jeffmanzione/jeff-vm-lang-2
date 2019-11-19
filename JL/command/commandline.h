/*
 * commandline.h
 *
 *  Created on: May 28, 2018
 *      Author: Jeff
 */

#ifndef COMMANDLINE_H_
#define COMMANDLINE_H_

#include "../datastructure/set.h"
#include "commandline_arg.h"

#define ARGSTORE_TEMPLATE_RETVAL(type, retval) \
  retval argstore_lookup_##type(const ArgStore *store, ArgKey key)

#define ARGSTORE_TEMPLATE(type) ARGSTORE_TEMPLATE_RETVAL(type, type)

typedef enum {
  ArgKey__NONE = 0,
  ArgKey__OUT_BINARY,
  ArgKey__OUT_MACHINE,
  ArgKey__OUT_UNOPTIMIZED,
  ArgKey__OPTIMIZE,
  ArgKey__EXECUTE,
  ArgKey__INTERPRETER,
  ArgKey__BUILTIN_DIR,
  ArgKey__BUILTIN_FILES,
  ArgKey__BIN_OUT_DIR,
  ArgKey__MACHINE_OUT_DIR,
  ArgKey__UNOPTIMIZED_OUT_DIR,
  ArgKey__END,
} ArgKey;

typedef struct __ArgConfig ArgConfig;
typedef struct __Argstore ArgStore;

ArgConfig *argconfig_create();
void argconfig_delete(ArgConfig *config);

void argconfig_add(ArgConfig *config, ArgKey key, const char name[],
                   Arg arg_default);

ArgStore *commandline_parse_args(ArgConfig *config, int argc,
                                 const char *const argv[]);

const Set *argstore_sources(const ArgStore *const store);

ARGSTORE_TEMPLATE(int);
ARGSTORE_TEMPLATE(float);
ARGSTORE_TEMPLATE_RETVAL(bool, _Bool);
ARGSTORE_TEMPLATE_RETVAL(string, const char *);
ARGSTORE_TEMPLATE_RETVAL(stringlist, const char **);

const Arg *argstore_get(const ArgStore *store, ArgKey key);
void argstore_delete(ArgStore *store);

#endif /* COMMANDLINE_H_ */
