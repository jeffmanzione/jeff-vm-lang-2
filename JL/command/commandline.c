/*
 * commandline.c
 *
 *  Created on: May 28, 2018
 *      Author: Jeff
 */

#include "commandline.h"

#include <stdint.h>
#include <string.h>

#include "../arena/strings.h"
#include "../datastructure/map.h"
#include "../error.h"
#include "../graph/memory.h"

#define ARGSTORE_LOOKUP(typet)                                          \
  bool argstore_lookup_##typet(const ArgStore *store, ArgKey key) {     \
    const Arg *arg = argstore_get(store, key);                          \
    if (!arg->used) {                                                   \
      ERROR("Store did not have key: %d", key);                         \
    }                                                                   \
    if (ArgType__##typet != arg->type) {                                \
      ERROR("Expected a "#typet" for key=%d. Was %d.", key, arg->type); \
    }                                                                   \
    return arg->typet##_val;                                            \
  }

struct __Argstore {
  const ArgConfig *config;
  Arg *args;
  Set src_files;
};

struct __ArgConfig {
  Map arg_names;
  Arg *args;
};

ArgConfig *argconfig_create() {
  ArgConfig *config = ALLOC2(ArgConfig);
  config->args = ALLOC_ARRAY(Arg, ArgKey__END);
  map_init_default(&config->arg_names);
  return config;
}

void argconfig_delete(ArgConfig *config) {
  ASSERT(NOT_NULL(config));
  DEALLOC(config->args);
  map_finalize(&config->arg_names);
  DEALLOC(config);
}

ArgStore *argstore_create(const ArgConfig *config) {
  ASSERT(NOT_NULL(config));
  ArgStore *store = ALLOC2(ArgStore);
  store->config = config;
  store->args = ALLOC_ARRAY(Arg, ArgKey__END);
  set_init_default(&store->src_files);
  return store;
}

void argstore_delete(ArgStore *store) {
  ASSERT(NOT_NULL(store));
  set_finalize(&store->src_files);
  DEALLOC(store->args);
  DEALLOC(store);
}

const Arg *argstore_get(const ArgStore *store, ArgKey key) {
  ASSERT(NOT_NULL(store), key > ArgKey__NONE, key < ArgKey__END);
  Arg *arg = &store->args[key];
  if (!arg->used && NULL != store->config) {
    arg = &store->config->args[key];
  }
  return arg;
}
ARGSTORE_LOOKUP(bool);
ARGSTORE_LOOKUP(int);
ARGSTORE_LOOKUP(float);
ARGSTORE_LOOKUP(string);

const Set *argstore_sources(const ArgStore * const store) {
  ASSERT(NOT_NULL(store));
  return &store->src_files;
}

void argconfig_add(ArgConfig *config, ArgKey key, const char name[],
    Arg arg_default) {
  ASSERT(NOT_NULL(config));
  const char *arg_name = strings_intern(name);
  if (!map_insert(&config->arg_names, arg_name, (uint32_t *) key)) {
    ERROR("Argument key string '%s' already added.", arg_name);
  }
  if (config->args[key].used) {
    ERROR("Trying to map arg key to '%s', but is already used.", arg_name);
  }
  config->args[key] = arg_default;
}

void parse_args_inner(int argc, const char * const argv[], Set *sources,
    Map *args) {
  int i;
  for (i = 1; i < argc; ++i) {
    const char *arg = argv[i];
    if ('-' != arg[0]) {
      const char *src = strings_intern(arg);
      set_insert(sources, src);
      continue;
    }
    char *eq = strchr(arg, '=');
    int arg_len = strlen(arg);
    int end_pos = (NULL == eq) ? arg_len : (eq - arg);
    char *key = strings_intern_range(arg, 1, end_pos);
    char *value =
        (NULL == eq) ?
            strings_intern("true") :
            strings_intern_range(arg, end_pos + 1, arg_len);
    map_insert(args, key, value);
  }
}

ArgStore *commandline_parse_args(ArgConfig *config, int argc,
    const char * const argv[]) {
  ASSERT(NOT_NULL(config));
  ArgStore *store = argstore_create(config);
  Map args;
  map_init_default(&args);
  parse_args_inner(argc, argv, &store->src_files, &args);

  void insert_arg(Pair *kv) {
    const char *key = (char *) kv->key;
    const char *value = (char *) kv->value;
    ArgKey arg_key = (ArgKey) map_lookup(&config->arg_names, key);
    if (ArgKey__NONE == arg_key) {
      ERROR("Unknown Arg name: %s", key);
    }
    Arg arg = config->args[arg_key];
    if (ArgType__none == arg.type) {
      ERROR("Unknown Arg type: %s", key);
    }
    store->args[arg_key] = arg_parse(arg.type, value);
    if (store->args[arg_key].type != arg.type) {
      ERROR(
          "ArgType of input does not match preset type. Was: %d, Expected: %d.",
          store->args[arg_key].type, arg.type);
    }
  }
  map_iterate(&args, insert_arg);
  map_finalize(&args);
  return store;
}
