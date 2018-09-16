/*
 * expression.c
 *
 *  Created on: Jun 23, 2018
 *      Author: Jeff
 */

#include "expression.h"

#include "../datastructure/expando.h"
#include "syntax.h"


struct ExpressionTree_ {
  enum {
    EType__identifier,
    EType__tuple_expression,
  } type;
  union {
    Expression_identifier identifier;
    Expression_tuple_expression tuple_expression;
  };
};

#define Claim(name)                                                \
ExpressionTree *fill_##name(const SyntaxTree *stree) {             \
  ExpressionTree *etree = ALLOC2(ExpressionTree);                  \
  etree->type = EType__##name;                                     \
  claim_##name(stree, &etree->name);                               \
  return etree;                                                    \
}                                                                  \
void claim_##name(const SyntaxTree *stree, Expression_##name *name)

Claim(identifier) {
  identifier->id = stree->token;
}


Claim(tuple_expression) {

}

