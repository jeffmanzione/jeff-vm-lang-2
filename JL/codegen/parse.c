/*
 * parse.c
 *
 *  Created on: Jan 16, 2017
 *      Author: Jeff
 */

#include "parse.h"

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "../error.h"
#include "../shared.h"

void parser_init(Parser *parser, FileInfo *src) {
  ASSERT_NOT_NULL(parser);
  queue_init(&parser->queue);
  parser->fi = src;
  parser->line = 1;
  parser->exp_names = map_create_default();
}

bool parser_finalize(Parser *parser) {
  bool has_error = false;
  ASSERT_NOT_NULL(parser);
  if (queue_size(&parser->queue) > 0) {
    Token *tok = queue_peek(&parser->queue);
    const FileInfo *fi = parser->fi;
    const LineInfo *li = file_info_lookup(fi, tok->line);
    fprintf(stderr, "Error in '%s' at line %d: %s.\n", file_info_name(fi),
            li->line_num, "Cannot parse");
    fflush(stderr);
    if (NULL != fi && NULL != li && NULL != tok) {
      int num_line_digits = (int)(log10(file_info_len(fi)) + 1);
      fprintf(stderr, "%*d: %s", num_line_digits, tok->line,
              (NULL == li) ? "No LineInfo" : li->line_text);
      if ('\n' != li->line_text[strlen(li->line_text) - 1]) {
        fprintf(stderr, "\n");
      }
      int i;
      for (i = 0; i < tok->col + num_line_digits + 1; i++) {
        fprintf(stderr, " ");
      }
      fprintf(stderr, "^");
      for (i = 1; i < tok->len; i++) {
        fprintf(stderr, "^");
      }
      fprintf(stderr, "\n");
      fflush(stderr);
    }
    has_error = true;
  }
  void say_tokens(void *ptr) {
    Token *tok = (Token *)ptr;
    fprintf(stderr, "Token(type=%d,line=%d,col=%d,text=%s)\n", tok->type,
            tok->line, tok->col, tok->text);
    token_delete((Token *)ptr);
  }
  queue_deep_delete(&parser->queue, (Deleter)say_tokens);
  map_delete(parser->exp_names);
  return has_error;
}

Token *parser_next__(Parser *parser, Token **target) {
  ASSERT_NOT_NULL(parser);
  Token *to_return;
  while (parser->queue.size <= 0 &&
         tokenize_line(&parser->line, parser->fi, &parser->queue, true))
    ;
  if (parser->queue.size == 0) {
    to_return = NULL;
  } else {
    to_return = (Token *)queue_peek(&parser->queue);
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

SyntaxTree parse_file(FileInfo *src) {
  Parser parser;
  parser_init(&parser, src);
  SyntaxTree m = file_level_statement_list(&parser);
  if (DBG) {
    DEBUGF("%d", m.matched);
    if (m.matched) {
      expression_tree_to_str(&m, &parser, stdout);
      fprintf(stdout, "\n");
      fflush(stdout);
    }
  }
  bool has_error = parser_finalize(&parser);
  if (has_error) {
    ERROR("Cannot finish parsing.");
  }
  return m;
}
