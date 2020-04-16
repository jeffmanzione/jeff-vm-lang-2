/*
 * loops.h
 *
 *  Created on: Dec 28, 2019
 *      Author: Jeff
 */

#ifndef CODEGEN_EXPRESSIONS_LOOPS_H_
#define CODEGEN_EXPRESSIONS_LOOPS_H_

#include "../tokenizer.h"
#include "assignment.h"
#include "expression_macros.h"

DefineExpression(foreach_statement) {
  Token *for_token, *in_token;
  Assignment assignment_lhs;
  ExpressionTree *iterable;
  ExpressionTree *body;
};

DefineExpression(for_statement) {
  Token *for_token;
  ExpressionTree *init;
  ExpressionTree *condition;
  ExpressionTree *inc;
  ExpressionTree *body;
};

DefineExpression(while_statement) {
  Token *while_token;
  ExpressionTree *condition;
  ExpressionTree *body;
};

#endif /* CODEGEN_EXPRESSIONS_LOOPS_H_ */
