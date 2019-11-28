/*
 * expression.h
 *
 *  Created on: Jun 23, 2018
 *      Author: Jeff
 */

#ifndef CODEGEN_EXPRESSION_H_
#define CODEGEN_EXPRESSION_H_

#include "../datastructure/expando.h"
#include "../element.h"
#include "../program/tape.h"
#include "syntax.h"
#include "tokenizer.h"

typedef struct ExpressionTree_ ExpressionTree;

#define DefineExpression(name) typedef struct Expression_##name##_ Expression_##name;                  \
                               void Transform_##name(const SyntaxTree *tree, Expression_##name *name); \
                               int Produce_##name(Expression_##name *name, Tape *tape);                \
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
  Expando *list;
};


#endif /* CODEGEN_EXPRESSION_H_ */
