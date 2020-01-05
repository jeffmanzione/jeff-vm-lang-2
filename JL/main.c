/*
 * main.c
 *
 *  Created on: Sep 28, 2016
 *      Author: Jeff
 */

#include <stdio.h>
#include <stdlib.h>

#include "arena/arena.h"
#include "arena/strings.h"
#include "codegen/expressions/expression.h"
#include "codegen/parse.h"
#include "codegen/syntax.h"
#include "codegen/tokenizer.h"
#include "command/commandline.h"
#include "command/commandlines.h"
#include "datastructure/map.h"
#include "datastructure/set.h"
#include "element.h"
#include "error.h"
#include "file_load.h"
#include "interpreter/interpreter.h"
#include "ltable/ltable.h"
#include "memory/memory.h"
#include "optimize/optimize.h"
#include "program/tape.h"
#include "vm/vm.h"

#ifdef NEW_PARSER
void test_expression(const char expression[]) {
  FILE *file = tmpfile();
  printf("EXPR: %s\n", expression);
  fflush(stdout);
  fprintf(file, "%s", expression);
  rewind(file);

  expression_init();
  Parser parser;
  FileInfo *fi = file_info_file(file);
  parser_init(&parser, fi);
  SyntaxTree stree = file_level_statement_list(&parser);
  syntax_tree_to_str(&stree, &parser, stdout);
  printf("\n");
  fflush(stdout);
  ExpressionTree *etree = populate_expression(&stree);
  Tape *tape = tape_create();
  produce_instructions(etree, tape);
  tape_write(tape, stdout);
  fflush(stdout);

  tape_delete(tape);
  delete_expression(etree);
  syntax_tree_delete(&stree);
  parser_finalize(&parser);
  file_info_delete(fi);
  expression_finalize();
}
#endif

int main(int argc, const char *argv[]) {
#ifdef MEMORY_WRAPPER
  alloc_init();
#endif

  arenas_init();
  strings_init();
  CKey_init();
  parsers_init();
  expression_init();

  ArgConfig *config = argconfig_create();
  argconfig_compile(config);
  argconfig_run(config);
  ArgStore *store = commandline_parse_args(config, argc, argv);

  optimize_init();

  Set modules;
  set_init_default(&modules);

//#ifdef NEW_PARSER
//  test_expression("a = b");
//  test_expression("const a = b");
//  test_expression("a[b] = c");
//  test_expression("a(b)[c] = d");
//  test_expression("(a, b) = c");
//  test_expression("(a.b, c) = d");
//  test_expression("(a().b, c) = d");
//  test_expression("(a(b).c, d) = e");
//  test_expression("(a[b].c, d) = e");
//  test_expression("(a[b], c) = d");
//  test_expression("(a(b)[c], d) = e");
//  test_expression("(a(b)[c], const d) = e");
//  test_expression("(a.b.c, const d) = e");
//  test_expression("a.b()[c] = d");
//  test_expression("a.b()[c].d = e");
//  test_expression("a().b.c[d] = e");
//  test_expression("a().b.c[d].e = f");
//  test_expression("(a, b, c, d) = e");
//  test_expression("((a, b), (c, d)) = e");
//  test_expression("[a, b] = c");
//  test_expression("[(a, b), c] = d");
//  test_expression("([a, b], c) = d");
//  test_expression("[a] = b");
//  test_expression("(a) = b");
//  test_expression("([a], b) = b");
//  test_expression("for a in b x");
//  test_expression("for (a in b) x");
//  test_expression("for (a=0, a < 10, a=a+1) x");
//  test_expression("for a=0, a < 10, a=a+1 x");
//  test_expression("while x y");
//  test_expression("while (x) y");
//  test_expression("{a\nb}");
//  test_expression("{a\nb\nc}");
//  test_expression("{a\nb\nc\nd}");
//  test_expression("try a catch b b.c");
//  test_expression("try { a } catch (const b) { b.c }");
//  test_expression("raise e()");
//  test_expression("if a { b } else if c { d } else { e }");
//  test_expression("if a { b }");
//  test_expression("if a { b } else if c { d }");
//  test_expression("if a { b } else if c { d } else if e { f } else if g { h } else { i }");
//  test_expression("return");
//  test_expression("return a");
//  test_expression("return (a, b, c)");
//  test_expression("break");
//  test_expression("continue");
//  test_expression("module a");
//  test_expression("import a");
//  test_expression("def a() b");
//  test_expression("def a(b) c");
//  test_expression("def a(b) { c }");
//  test_expression("def a(b, c) { d }");
//  test_expression("def a(b, c, d) { e }");
//  test_expression("def a(b, const c, d = e, const f = g ) { h }");
//  test_expression("def a(b) const { c }");
//  test_expression("def a(b, c) const { d }");
//  test_expression("class A {\n  field b\n}");
//  test_expression("class A : B {\n  def b(c) {\n    d\n  }\n}");
//  test_expression("class A : B, C {\n  field d\n  def b(c) {\n    d\n  }\n}");
//  test_expression("class A : B, C, D {\n  field a\n  field b\n  def c(d) {\n    e\n  }\n  def f(g) {\n    h\n  }\n}");
//  test_expression("class A : B, C, D {\n  field a\n  field b\n  def c(d) {\n    e\n  }\n  def f(g) {\n    h\n  }\n  def new() {\n    x\n  }\n}");
//#else

  void load_src(void *ptr) {
    ASSERT(NOT_NULL(ptr));
    Module *module = load_fn((char*) ptr, store);
    set_insert(&modules, module);
  }
  set_iterate(argstore_sources(store), load_src);

  VM *vm = vm_create(store);

  if (argstore_lookup_bool(store, ArgKey__EXECUTE)
      || argstore_lookup_bool(store, ArgKey__INTERPRETER)) {
    Element main_element = create_none();
    void load_module(void *ptr) {
      ASSERT(NOT_NULL(ptr));
      Element module = vm_add_module(vm, (Module*) ptr);
      if (NONE == main_element.type) {
        main_element = module;
      }
    }
    set_iterate(&modules, load_module);
    if (argstore_lookup_bool(store, ArgKey__EXECUTE)) {
      if (NONE == main_element.type) {
        ERROR("Main not provided.");
      }
      vm_start_execution(vm, main_element);
    }
    if (argstore_lookup_bool(store, ArgKey__INTERPRETER)) {
      printf("Starting Interpreter.\nWrite code below. Press enter to "
          "evaluate.\n");
      fflush(stdout);
      interpret_from_file(stdin, "stdin", vm, interpret_statement);
      printf("Goodbye!\n");
      fflush(stdout);
    }
  }
//#endif

  set_finalize(&modules);
  argstore_delete(store);
  argconfig_delete(config);

//#ifndef NEW_PARSER
  vm_delete(vm);
//#endif

  optimize_finalize();
  expression_finalize();
  parsers_finalize();
  CKey_finalize();
  strings_finalize();
  arenas_finalize();

#ifdef MEMORY_WRAPPER
  alloc_finalize();
#endif
#ifdef DEBUG
  printf("Maps: %d\nMap inserts: %d\nMap insert compares: %d\n"
      "Map lookups: %d\nMap lookup compares: %d\n", MAP__count,
      MAP__insert_count, MAP__insert_compares_count, MAP__lookup_count,
      MAP__lookup_compares_count);
  fflush(stdout);
#endif
  return EXIT_SUCCESS;
}
