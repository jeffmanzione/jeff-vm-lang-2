/*
 * expression.h
 *
 *  Created on: Jun 23, 2018
 *      Author: Jeff
 */

#ifndef CODEGEN_EXPRESSIONS_EXPRESSION_H_
#define CODEGEN_EXPRESSIONS_EXPRESSION_H_

#include <stdbool.h>

#ifdef NEW_PARSER

#include <stdbool.h>
#include <stdint.h>

#include "../../datastructure/expando.h"
#include "../../element.h"
#include "../../program/tape.h"
#include "../syntax.h"
#include "../tokenizer.h"
#include "assignment.h"
#include "classes.h"
#include "expression_macros.h"
#include "files.h"
#include "loops.h"
#include "statements.h"

typedef struct ExpressionTree_ ExpressionTree;

void expression_init();
void expression_finalize();

ExpressionTree *populate_expression(const SyntaxTree *tree);
int produce_instructions(ExpressionTree *tree, Tape *tape);
void delete_expression(ExpressionTree *tree);

DefineExpression(identifier) { Token *id; };

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
  ExpressionTree *exp;  // A tuple expression.
};

DefineExpression(primary_expression) {
  Token *token;
  ExpressionTree *exp;
};

typedef enum {
  Postfix_none,
  Postfix_field,
  Postfix_fncall,
  Postfix_array_index
} PostfixType;

typedef struct {
  PostfixType type;
  union {
    const Token *id;
    ExpressionTree *exp;  // Can be NULL for empty fncall.
  };
  Token *token;
} Postfix;

int produce_postfix(int *i, int num_postfix, Expando *suffixes, Postfix **next,
                    Tape *tape);

DefineExpression(postfix_expression) {
  ExpressionTree *prefix;
  Expando *suffixes;
};

DefineExpression(range_expression) {
  Token *token;
  uint32_t num_args;
  ExpressionTree *start;
  ExpressionTree *end;
  ExpressionTree *inc;
};

typedef enum {
  Unary_unknown,
  Unary_not,
  Unary_notc,
  Unary_negate,
  Unary_const
} UnaryType;

DefineExpression(unary_expression) {
  Token *token;
  UnaryType type;
  ExpressionTree *exp;
};

typedef enum {
  BiType_unknown,

  Mult_mult,
  Mult_div,
  Mult_mod,

  Add_add,
  Add_sub,

  Rel_lt,
  Rel_gt,
  Rel_lte,
  Rel_gte,
  Rel_eq,
  Rel_neq,

  And_and,
  And_xor,
  And_or,

} BiType;

typedef struct {
  Token *token;
  BiType type;
  ExpressionTree *exp;
} BiSuffix;

#define BiDefineExpression(expr) \
  DefineExpression(expr) {       \
    ExpressionTree *exp;         \
    Expando *suffixes;           \
  }

BiDefineExpression(multiplicative_expression);
BiDefineExpression(additive_expression);
BiDefineExpression(relational_expression);
BiDefineExpression(equality_expression);
BiDefineExpression(and_expression);
BiDefineExpression(xor_expression);
BiDefineExpression(or_expression);

DefineExpression(in_expression) {
  Token *token;
  bool is_not;
  ExpressionTree *element;
  ExpressionTree *collection;
};

DefineExpression(is_expression) {
  Token *token;
  ExpressionTree *exp;
  ExpressionTree *type;
};

DefineExpression(conditional_expression) { IfElse if_else; };

DefineExpression(anon_function_definition) { Function func; };

typedef struct {
  Token *colon;
  ExpressionTree *lhs;
  ExpressionTree *rhs;
} MapDecEntry;

DefineExpression(map_declaration) {
  Token *lbrce, *rbrce;
  bool is_empty;
  Expando *entries;
};

struct ExpressionTree_ {
  ParseExpression type;
  union {
    Expression_identifier identifier;
    Expression_constant constant;
    Expression_string_literal string_literal;
    Expression_tuple_expression tuple_expression;
    Expression_array_declaration array_declaration;
    Expression_map_declaration map_declaration;
    Expression_primary_expression primary_expression;
    Expression_postfix_expression postfix_expression;
    Expression_range_expression range_expression;
    Expression_unary_expression unary_expression;
    Expression_multiplicative_expression multiplicative_expression;
    Expression_additive_expression additive_expression;
    Expression_relational_expression relational_expression;
    Expression_equality_expression equality_expression;
    Expression_and_expression and_expression;
    Expression_xor_expression xor_expression;
    Expression_or_expression or_expression;
    Expression_in_expression in_expression;
    Expression_is_expression is_expression;
    Expression_conditional_expression conditional_expression;
    Expression_anon_function_definition anon_function_definition;
    Expression_assignment_expression assignment_expression;
    Expression_foreach_statement foreach_statement;
    Expression_for_statement for_statement;
    Expression_while_statement while_statement;
    Expression_compound_statement compound_statement;
    Expression_try_statement try_statement;
    Expression_raise_statement raise_statement;
    Expression_selection_statement selection_statement;
    Expression_jump_statement jump_statement;
    Expression_break_statement break_statement;
    Expression_exit_statement exit_statement;
    Expression_module_statement module_statement;
    Expression_import_statement import_statement;
    Expression_function_definition function_definition;
    Expression_class_definition class_definition;
    Expression_file_level_statement_list file_level_statement_list;
  };
};

#endif

#endif /* CODEGEN_EXPRESSIONS_EXPRESSION_H_ */
