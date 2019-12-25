/*
 * expression.h
 *
 *  Created on: Jun 23, 2018
 *      Author: Jeff
 */

#ifndef CODEGEN_EXPRESSION_H_
#define CODEGEN_EXPRESSION_H_

#include <stdbool.h>

#include "../datastructure/expando.h"
#include "../element.h"
#include "../program/tape.h"
#include "syntax.h"
#include "tokenizer.h"

typedef struct ExpressionTree_ ExpressionTree;

void expression_init();
void expression_finalize();

ExpressionTree *populate_expression(SyntaxTree *tree);
int produce_instructions(ExpressionTree *tree, Tape *tape);
void delete_expression(ExpressionTree *tree);

#define DefineExpression(name) typedef struct Expression_##name##_ Expression_##name;                  \
                               void Transform_##name(const SyntaxTree *tree, Expression_##name *name); \
                               void Delete_##name(ExpressionTree *tree);                         \
                               void Delete_##name##_inner(Expression_##name *name);                         \
                               int Produce_##name(ExpressionTree *tree, Tape *tape);                \
                               int Produce_##name##_inner(Expression_##name *name, Tape *tape);               \
                               struct Expression_##name##_

DefineExpression(identifier) {
  Token *id;
};

DefineExpression(constant) {
  Token *token;
  Value value;
};

DefineExpression(string_literal) {
  Token *token;
  const char *str;
};

DefineExpression(tuple_expression) {
  Token *token;
  Expando *list;
};

DefineExpression(array_declaration) {
  Token *token;
  bool is_empty;
  ExpressionTree *exp;
};

DefineExpression(primary_expression) {
  Token *token;
  ExpressionTree *exp;
};

typedef struct {
  TokenType type;
  ExpressionTree *exp;
} Postfix;

DefineExpression(postfix_expression) {
  ExpressionTree *prefix;
  Expando *suffixes;
};

#endif /* CODEGEN_EXPRESSION_H_ */
