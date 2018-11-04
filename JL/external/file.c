/*
 * file.c
 *
 *  Created on: Jun 4, 2018
 *      Author: Jeff
 */

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

#include "../arena/strings.h"
#include "../class.h"
#include "../datastructure/map.h"
#include "../datastructure/tuple.h"
#include "../element.h"
#include "../error.h"
#include "../graph/memory.h"
#include "../graph/memory_graph.h"
#include "../shared.h"
#include "../vm.h"
#include "external.h"
#include "strings.h"

Element file_constructor(VM *vm, ExternalData *data, Element arg) {
  ASSERT(arg.type == OBJECT);
  char *fn, *mode;
  if (ISTYPE(arg, class_string)) {
    fn = string_to_cstr(arg);
    mode = strings_intern("r");
  } else if (is_object_type(&arg, TUPLE)) {
    Tuple *args = arg.obj->tuple;
    fn = string_to_cstr(tuple_get(args, 0));
    mode = string_to_cstr(tuple_get(args, 1));
  } else {
    ERROR("Unknown input.");
    return create_none();
  }

  FILE *file;
  if (0 == strcmp(fn, "__STDOUT__")) {
    file = stdout;
  } else if (0 == strcmp(fn, "__STDIN__")) {
    file = stdin;
  } else if (0 == strcmp(fn, "__STDERR__")) {
    file = stderr;
  } else {
    file = fopen(fn, mode);
  }

  ASSERT(NOT_NULL(data));
  memory_graph_set_field(vm->graph, data->object, strings_intern("success"),
      (NULL == file) ? create_none() : create_int(1));

  if (NULL != file) {
    map_insert(&data->state, strings_intern("file"), file);
  }
  return data->object;
}

Element file_deconstructor(VM *vm, ExternalData *data, Element arg) {
  FILE *file = map_lookup(&data->state, strings_intern("file"));
  if (NULL != file && stdin != file && stdout != file && stderr != file) {
    fclose(file);
  }
  return create_none();
}

Element file_gets(VM *vm, ExternalData *data, Element arg) {
  FILE *file = map_lookup(&data->state, strings_intern("file"));
  ASSERT(NOT_NULL(file));
  ASSERT(is_value_type(&arg, INT));
  char *buf = ALLOC_ARRAY2(char, arg.val.int_val + 1);
  fgets(buf, arg.val.int_val + 1, file);
  Element string = string_create_len(vm, buf, arg.val.int_val);
  DEALLOC(buf);
  return string;
}

Element file_puts(VM *vm, ExternalData *data, Element arg) {
  FILE *file = map_lookup(&data->state, strings_intern("file"));
  ASSERT(NOT_NULL(file));
  if (arg.type == NONE || !ISTYPE(arg, class_string)) {
    return create_none();
  }
//  char *cstr = string_to_cstr(arg);
  String *string = String_extract(arg);
  fprintf(file, "%*s", String_size(string), String_cstr(string));
//  fputs(cstr, file);
  fflush(file);
  return create_none();
}

Element file_getline(VM *vm, ExternalData *data, Element arg) {
  FILE *file = map_lookup(&data->state, strings_intern("file"));
  ASSERT(NOT_NULL(file));
  char *line = NULL;
  size_t len = 0;
  int nread = getline(&line, &len, file);
  Element string;
  if (-1 == nread) {
    string = create_none();
  } else {
    string = string_create_len(vm, line, nread);
  }
  if (line != NULL) {
    DEALLOC(line);
  }
  return string;
}

Element file_rewind(VM *vm, ExternalData *data, Element arg) {
  FILE *file = map_lookup(&data->state, strings_intern("file"));
  ASSERT(NOT_NULL(file));
  rewind(file);
  return create_none();
}

Element create_file_class(VM *vm, Element module) {
  Element file_class = create_external_class(vm, module,
      strings_intern("File__"), file_constructor, file_deconstructor);
  add_external_function(vm, file_class, strings_intern("gets__"), file_gets);
  add_external_function(vm, file_class, strings_intern("puts__"), file_puts);
  add_external_function(vm, file_class, strings_intern("getline__"),
      file_getline);
  add_external_function(vm, file_class, strings_intern("rewind__"),
      file_rewind);
  add_external_function(vm, file_class, strings_intern("close__"),
      file_deconstructor);
  return file_class;
}

