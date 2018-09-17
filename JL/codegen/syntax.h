/*
 * syntax.h
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
typedef struct SyntaxTree_ SyntaxTree;
typedef SyntaxTree (*ParseExpression)(Parser *);

struct SyntaxTree_ {
  bool matched;
  Token *token;
  //Queue child_exps;
  SyntaxTree *first, *second;
  ParseExpression expression;
};

void expression_tree_delete(SyntaxTree *exp);
void expression_tree_to_str(SyntaxTree *exp, Parser *parser, FILE *file);

#define DefineSyntax(name) SyntaxTree name(Parser *parser)

DefineSyntax(identifier);
DefineSyntax(constant);
DefineSyntax(string_literal);
DefineSyntax(array_declaration);
DefineSyntax(length_expression);
DefineSyntax(primary_expression);
DefineSyntax(primary_expression_no_constants);
//DefineExpression(argument_expression_list);
DefineSyntax(postfix_expression);
DefineSyntax(postfix_expression1);
DefineSyntax(unary_expression);
DefineSyntax(multiplicative_expression);
DefineSyntax(additive_expression);
DefineSyntax(relational_expression);
DefineSyntax(equality_expression);
DefineSyntax(and_expression);
DefineSyntax(xor_expression);
DefineSyntax(or_expression);
DefineSyntax(is_expression);
DefineSyntax(anon_function);
DefineSyntax(conditional_expression);
DefineSyntax(assignment_expression);
DefineSyntax(assignment_tuple);
DefineSyntax(tuple_expression);
DefineSyntax(tuple_expression1);
DefineSyntax(expression_statement);
DefineSyntax(iteration_statement);
DefineSyntax(compound_statement);
DefineSyntax(selection_statement);
DefineSyntax(jump_statement);
DefineSyntax(function_argument_list);
DefineSyntax(function_argument_list1);
DefineSyntax(function_definition);
DefineSyntax(field_statement);
DefineSyntax(class_definition);
DefineSyntax(class_statement);
DefineSyntax(class_statement_list);
DefineSyntax(class_statement_list1);
DefineSyntax(import_statement);
DefineSyntax(module_statement);
DefineSyntax(statement);
DefineSyntax(statement_list);
DefineSyntax(statement_list1);
DefineSyntax(file_level_statement);
DefineSyntax(file_level_statement_list);
DefineSyntax(file_level_statement_list1);
DefineSyntax(class_new_statement);
DefineSyntax(parent_class_list);
DefineSyntax(parent_class_list1);
DefineSyntax(break_statement);
DefineSyntax(try_statement);
DefineSyntax(exit_statement);
DefineSyntax(raise_statement);
DefineSyntax(range_expression);
DefineSyntax(foreach_statement);
DefineSyntax(for_statement);
DefineSyntax(while_statement);


#endif /* EXPRESSION_H_ */
