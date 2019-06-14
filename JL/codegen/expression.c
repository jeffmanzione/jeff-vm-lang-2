/*
 * expression.c
 *
 *  Created on: Jun 23, 2018
 *      Author: Jeff
 */

#include "expression.h"

#include "../datastructure/expando.h"
#include "../graph/memory.h"
#include "../instruction.h"
#include "syntax.h"

#define Populate(name, stree) \
  if (tree->expression == name) return Populate_##name(stree)
#define Codegen(name, etree, tape) \
  if (tree->type == name) return Produce_##name(&etree->name, tape)

struct ExpressionTree_ {
  ParseExpression type;
  union {
    Expression_identifier identifier;
    Expression_constant constant;
    Expression_string_literal string_literal;
    Expression_tuple_expression tuple_expression;
  };
};

#define ImplExpression(name, stree_input)           \
  ExpressionTree *Populate_##name(stree_input) {    \
    ExpressionTree *etree = ALLOC2(ExpressionTree); \
    etree->type = name;                             \
    Transform_##name(stree, &etree->name);          \
    return etree;                                   \
  }                                                 \
  void Transform_##name(stree_input, Expression_##name *name)

#define ImplProduce(name, tape) \
  int Produce_##name(Expression_##name *name, tape)

ImplExpression(identifier, const SyntaxTree *stree) {
  identifier->id = stree->token;
}

ImplProduce(identifier, Tape *tape) {
  return tape->ins(tape, RES, identifier->id);
}

ImplExpression(constant, const SyntaxTree *stree) {
  constant->token = stree->token;
  constant->value = token_to_val(stree->token);
}

ImplProduce(constant, Tape *tape) {
  return tape->ins(tape, RES, constant->token);
}

ImplExpression(string_literal, const SyntaxTree *stree) {
  string_literal->token = stree->token;
  string_literal->str = stree->token->text;
}

ImplProduce(string_literal, Tape *tape) {
  return tape->ins(tape, RES, string_literal->token);
}

ImplExpression(tuple_expression, const SyntaxTree *stree) {}

ImplProduce(tuple_expression, Tape *tape) { return 0; }

ExpressionTree *produce_expression(SyntaxTree *tree) {
  Populate(identifier, tree);
  Populate(constant, tree);
  Populate(string_literal, tree);
  Populate(tuple_expression, tree);
  return NULL;
}

int codegen_from_expression(ExpressionTree *tree, Tape *tape) {
  Codegen(identifier, tree, tape);
  Codegen(constant, tree, tape);
  Codegen(string_literal, tree, tape);
  Codegen(tuple_expression, tree, tape);
  return 0;
}
