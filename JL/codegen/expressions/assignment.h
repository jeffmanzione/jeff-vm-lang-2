/*
 * assignment.h
 *
 *  Created on: Dec 27, 2019
 *      Author: Jeff
 */

#ifndef CODEGEN_EXPRESSIONS_ASSIGNMENT_H_
#define CODEGEN_EXPRESSIONS_ASSIGNMENT_H_

#ifdef NEW_PARSER

#include "../../datastructure/expando.h"
#include "../tokenizer.h"
#include "expression_macros.h"

typedef struct {
  enum {
    SingleAssignment_var, SingleAssignment_complex
  } type;
  union {
    struct {
      Token *var;
      bool is_const;
      Token *const_token;
    };
    struct {
      ExpressionTree *prefix_exp;
      Expando *suffixes;  // Should be Posfixes.
    };
  };
} SingleAssignment;

typedef struct {
  Expando *subargs;
} MultiAssignment;

typedef struct {
  enum {
    Assignment_single, Assignment_array, Assignment_tuple
  } type;
  union {
    SingleAssignment single;
    MultiAssignment multi;
  };
} Assignment;

DefineExpression(assignment_expression) {
  Token *eq_token;
  Assignment assignment;
  ExpressionTree *rhs;
};

Assignment populate_assignment(const SyntaxTree *stree);
void delete_assignment(Assignment *assignment);
int produce_assignment(Assignment *assign, Tape *tape, const Token *eq_token);

#endif

#endif /* CODEGEN_EXPRESSIONS_ASSIGNMENT_H_ */
