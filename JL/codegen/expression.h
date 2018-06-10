/*
 * expression.h
 *
 *  Created on: Jan 20, 2017
 *      Author: Jeff
 */

#ifndef EXPRESSION_H_
#define EXPRESSION_H_

#include <stdbool.h>
#include <stdio.h>

#include "tokenizer.h"

typedef struct Parser_ Parser;
typedef struct ExpressionTree_ ExpressionTree;
typedef ExpressionTree (*ParseExpression)(Parser *);

struct ExpressionTree_ {
  bool matched;
  Token *token;
  //Queue child_exps;
  ExpressionTree *first, *second;
  ParseExpression expression;
};

void expression_tree_delete(ExpressionTree exp);
void expression_tree_to_str(ExpressionTree exp, Parser *parser, FILE *file);

#define DefineExpression(name) ExpressionTree name(Parser *parser)

DefineExpression(identifier);
DefineExpression(constant);
DefineExpression(string_literal);
DefineExpression(array_declaration);
DefineExpression(length_expression);
DefineExpression(primary_expression);
DefineExpression(primary_expression_no_constants);
//DefineExpression(argument_expression_list);
DefineExpression(postfix_expression);
DefineExpression(postfix_expression1);
DefineExpression(unary_expression);
DefineExpression(multiplicative_expression);
DefineExpression(additive_expression);
DefineExpression(relational_expression);
DefineExpression(equality_expression);
DefineExpression(and_expression);
DefineExpression(xor_expression);
DefineExpression(or_expression);
DefineExpression(is_expression);
DefineExpression(anon_function);
DefineExpression(conditional_expression);
DefineExpression(assignment_expression);
DefineExpression(assignment_tuple);
DefineExpression(tuple_expression);
DefineExpression(tuple_expression1);
DefineExpression(expression_statement);
DefineExpression(iteration_statement);
DefineExpression(compound_statement);
DefineExpression(selection_statement);
DefineExpression(jump_statement);
DefineExpression(function_argument_list);
DefineExpression(function_argument_list1);
DefineExpression(function_definition);
DefineExpression(field_statement);
DefineExpression(class_definition);
DefineExpression(class_statement);
DefineExpression(class_statement_list);
DefineExpression(class_statement_list1);
DefineExpression(import_statement);
DefineExpression(module_statement);
DefineExpression(statement);
DefineExpression(statement_list);
DefineExpression(statement_list1);
DefineExpression(file_level_statement);
DefineExpression(file_level_statement_list);
DefineExpression(file_level_statement_list1);
DefineExpression(class_constructor_list);
DefineExpression(class_constructor_list1);
DefineExpression(class_new_statement);
DefineExpression(parent_class_list);
DefineExpression(parent_class_list1);
DefineExpression(class_constructors);

DefineExpression(break_statement);


#endif /* EXPRESSION_H_ */
