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

#define ARGSTORE_TEMPLATE(type) \
  bool argstore_lookup_##type(const ArgStore *store, ArgKey key)

typedef enum {
  ArgKey__NONE = 0,
  ArgKey__OUT_BINARY,
  ArgKey__OUT_MACHINE,
  ArgKey__OUT_UNOPTIMIZED,
  ArgKey__OPTIMIZE,
  ArgKey__EXECUTE,
  ArgKey__END,
} ArgKey;

typedef struct __ArgConfig ArgConfig;
typedef struct __Argstore ArgStore;

ArgConfig *argconfig_create();
void argconfig_delete(ArgConfig *config);

void argconfig_add(ArgConfig *config, ArgKey key, const char name[],
    Arg arg_default);

ArgStore *commandline_parse_args(ArgConfig *config, int argc,
    const char * const argv[]);

const Set *argstore_sources(const ArgStore * const store);

ARGSTORE_TEMPLATE(bool);
ARGSTORE_TEMPLATE(int);
ARGSTORE_TEMPLATE(float);
ARGSTORE_TEMPLATE(string);

void argstore_delete(ArgStore *store);

#endif /* COMMANDLINE_H_ */
