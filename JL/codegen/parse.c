/*
 * parse.c
 *
 *  Created on: Jan 16, 2017
 *      Author: Jeff
 */

#include "parse.h"

#include <stdbool.h>
#include <stdio.h>

#include "../error.h"
#include "../shared.h"

void parser_init(Parser *parser, FileInfo *src) {
  ASSERT_NOT_NULL(parser);
  queue_init(&parser->queue);
  parser->fi = src;
  parser->line = 1;
  parser->exp_names = map_create_default();
}

void parser_finalize(Parser *parser) {
  ASSERT_NOT_NULL(parser);
  void say_tokens(void *ptr) {
    Token *tok = (Token *) ptr;
    DEBUGF("Tok remaining: '%s',%d", tok->text, tok->type);
    token_delete(tok);
  }
  queue_deep_delete(&parser->queue, (Deleter) say_tokens);
  map_delete(parser->exp_names);
}

Token *parser_next__(Parser *parser, Token **target) {
  ASSERT_NOT_NULL(parser);
  Token *to_return;
  while (parser->queue.size <= 0
      && tokenize_line(&parser->line, parser->fi, &parser->queue, false))
    ;
  if (parser->queue.size == 0) {
    to_return = NULL;
  } else {
    to_return = (Token *) queue_peek(&parser->queue);
  }

  if (NULL != target) {
    *target = to_return;
  }
  return to_return;
}

Token *parser_consume(Parser *parser) {
  ASSERT(NOT_NULL(parser), parser->queue.size > 0);
  return queue_remove(&parser->queue);
}

Token *parser_next_skip_ln__(Parser *parser, Token **target) {
  ASSERT_NOT_NULL(parser);
  Token *tok;
  while (NULL != (tok = parser_next(parser, target)) && ENDLINE == tok->type) {
    parser_consume(parser);
    token_delete(tok);
  }
  return tok;
}

ExpressionTree parse_file(FileInfo *src) {
  Parser parser;
  parser_init(&parser, src);
  ExpressionTree m = file_level_statement_list(&parser);
  if(DBG) {
    DEBUGF("%d", m.matched);
    if (m.matched) {
      expression_tree_to_str(m, &parser, stdout);
      fprintf(stdout, "\n");
      fflush(stdout);
    }
  }
  parser_finalize(&parser);
  return m;
}
