/*
 * parse.h
 *
 *  Created on: Jan 16, 2017
 *      Author: Jeff
 */

#ifndef PARSE_H_
#define PARSE_H_

#include <stdio.h>

#include "expression.h"
#include "map.h"
#include "queue.h"
#include "tokenizer.h"

#define GET_PARSER(_1,_2,NAME,...) NAME
#define WITH_TARGET(parser, target) parser_next__(parser, target)
#define NO_TARGET(parser) WITH_TARGET(parser, NULL)
#define SKIP_WITH_TARGET(parser, target) parser_next_skip_ln__(parser, target)
#define SKIP_NO_TARGET(parser) SKIP_WITH_TARGET(parser, NULL)
#define parser_next(...) GET_PARSER(__VA_ARGS__, WITH_TARGET, NO_TARGET)(__VA_ARGS__)
#define parser_next_skip_ln(...) GET_PARSER(__VA_ARGS__, SKIP_WITH_TARGET, SKIP_NO_TARGET)(__VA_ARGS__)

typedef struct Parser_ {
  Queue queue;
  FileInfo *fi;
  int line;
  Map *exp_names;
} Parser;

ExpressionTree parse_file(FileInfo *src);
Token *parser_next__(Parser *parser, Token **target);
Token *parser_next_skip_ln__(Parser *parser, Token **target);

#endif /* PARSE_H_ */
