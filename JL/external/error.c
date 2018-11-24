/*
 * error.c
 *
 *  Created on: Jun 9, 2018
 *      Author: Jeff
 */

#include "../error.h"
#include "error.h"

#include "../arena/strings.h"
#include "../codegen/tokenizer.h"
#include "../datastructure/tuple.h"
#include "../element.h"
#include "../graph/memory_graph.h"
#include "../module.h"
#include "../shared.h"
#include "../tape.h"
#include "../vm.h"
#include "external.h"

Element token__(VM *vm, Thread *t, ExternalData *ed, Element argument) {
  if (!is_object_type(&argument, TUPLE)) {
    return throw_error(vm, t, "Argument to token__ must be a tuple.");
  }
  Tuple *args = argument.obj->tuple;
  if (tuple_size(args) != 2) {
    return throw_error(vm, t, "Argument to token__ must be a size 2 tuple.");
  }
  Element module = tuple_get(args, 0);
  Element ip = tuple_get(args, 1);
  if (!is_object_type(&module, MODULE)) {
    return throw_error(vm, t, "First argument to token__ must be a Module.");
  }
  if (!is_value_type(&ip, INT)) {
    return throw_error(vm, t, "Second argument to token__ must be an int.");
  }

  const InsContainer *c = module_insc(module.obj->module, ip.val.int_val);
  const FileInfo *fi = module_fileinfo(module.obj->module);

  const LineInfo *li =
      (fi == NULL || c == NULL || c->token == NULL) ?
          NULL : file_info_lookup(fi, c->token->line);
  Element retval = create_tuple(vm->graph);
  memory_graph_tuple_add(vm->graph, retval,
      string_create(vm, GET_OR(c->token, text, strings_intern(""))));
  memory_graph_tuple_add(vm->graph, retval,
      create_int(GET_OR(c->token, line, -1)));
  memory_graph_tuple_add(vm->graph, retval,
      create_int(GET_OR(c->token, col, -1)));
  memory_graph_tuple_add(vm->graph, retval,
      (fi == NULL) ?
          create_none() :
          string_create(vm, module_filename(module.obj->module)));
  memory_graph_tuple_add(vm->graph, retval,
      (li == NULL) ? create_none() : string_create(vm, li->line_text));
  return retval;
}
