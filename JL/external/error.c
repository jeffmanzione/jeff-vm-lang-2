/*
 * error.c
 *
 *  Created on: Jun 9, 2018
 *      Author: Jeff
 */

#include "../error.h"
#include "error.h"

#include "../codegen/tokenizer.h"
#include "../datastructure/tuple.h"
#include "../element.h"
#include "../graph/memory_graph.h"
#include "../module.h"
#include "../tape.h"
#include "../vm.h"
#include "external.h"

Element token__(VM *vm, ExternalData *ed, Element argument) {
  ASSERT(is_object_type(&argument, TUPLE));
  Tuple *args = argument.obj->tuple;
  ASSERT(tuple_size(args) == 2);
  Element module = tuple_get(args, 0);
  Element ip = tuple_get(args, 1);
  ASSERT(is_object_type(&module, MODULE), is_value_type(&ip, INT));
  const InsContainer *c = module_insc(module.obj->module, ip.val.int_val);
  const FileInfo *fi = module_fileinfo(module.obj->module);
  const LineInfo *li =
      (fi == NULL) ? NULL : file_info_lookup(fi, c->token->line);
  Element retval = create_tuple(vm->graph);
  memory_graph_tuple_add(vm->graph, retval, string_create(vm, c->token->text));
  memory_graph_tuple_add(vm->graph, retval, create_int(c->token->line));
  memory_graph_tuple_add(vm->graph, retval, create_int(c->token->col));
  memory_graph_tuple_add(vm->graph, retval,
      (fi == NULL) ?
          create_none() :
          string_create(vm, module_filename(module.obj->module)));
  memory_graph_tuple_add(vm->graph, retval,
      (li == NULL) ? create_none() : string_create(vm, li->line_text));
  return retval;
}

//Element file_line_text__(VM *vm, ExternalData *ed, Element argument) {
//  ASSERT(is_object_type(&argument, TUPLE));
//  Tuple *args = argument.obj->tuple;
//  ASSERT(tuple_size(args) == 2);
//  Element module = tuple_get(args, 0);
//  Element ip = tuple_get(args, 1);
//  ASSERT(is_object_type(&module, MODULE), is_value_type(&ip, INT));
//  const InsContainer *c = module_insc(module.obj->module, ip.val.int_val);
//  const FileInfo *fi = module_fileinfo(module.obj->module);
//  const LineInfo *li = file_info_lookup(fi, c->token->line);
//  return string_create(vm, li->line_text);
//
//}
