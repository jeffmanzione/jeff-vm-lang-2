/*
 * syntax.c
 *
 *  Created on: Jan 19, 2017
 *      Author: Jeff
 */

#include "syntax.h"

#include "../datastructure/map.h"
#include "../datastructure/queue.h"
#include "../error.h"
#include "../memory/memory.h"
#include "parse.h"

void print_tabs(FILE *file, int num_tabs) {
  int i;
  for (i = 0; i < num_tabs; i++) {
    fprintf(file, "  ");
  }
}

void syntax_to_str_helper(SyntaxTree *exp, Parser *parser, FILE *file,
                          int num_tabs) {
  print_tabs(file, num_tabs);
  if (NULL != exp->expression) {
    fprintf(file, "[%s] ",
            (char *)map_lookup(&parse_expressions, exp->expression));
  }
  if (NULL == exp->token && NULL == exp->first) {
    fprintf(file, "Epsilon");
    return;
  } else if (NULL != exp->token) {
    fprintf(file, "'%s'",
            exp->token->type == ENDLINE ? "\\n" : exp->token->text);
  }
  if (NULL == exp->first) {
    return;
  }
  fprintf(file, "{\n");
  syntax_to_str_helper(exp->first, parser, file, num_tabs + 1);
  if (NULL != exp->second) {
    fprintf(file, ",\n");
    syntax_to_str_helper(exp->second, parser, file, num_tabs + 1);
  }
  fprintf(file, "\n");
  print_tabs(file, num_tabs);
  fprintf(file, "}");
  fflush(file);
}

void syntax_tree_to_str(SyntaxTree *exp, Parser *parser, FILE *file) {
  syntax_to_str_helper(exp, parser, file, 0);
}

void syntax_tree_delete(SyntaxTree *exp) {
  if (exp->first != NULL) {
    syntax_tree_delete(exp->first);
    DEALLOC(exp->first);
  }
  if (exp->second != NULL) {
    syntax_tree_delete(exp->second);
    DEALLOC(exp->second);
  }
}

SyntaxTree no_match(Parser *parser) {
  SyntaxTree exp_match;
  exp_match.matched = false;
  exp_match.token = NULL;
  exp_match.first = NULL;
  exp_match.second = NULL;
  exp_match.expression = NULL;
  return exp_match;
}

SyntaxTree match_epsilon(Parser *parser) {
  SyntaxTree exp_match;
  exp_match.matched = true;
  exp_match.token = NULL;
  exp_match.first = NULL;
  exp_match.second = NULL;
  exp_match.expression = NULL;
  return exp_match;
}

bool is_epsilon(SyntaxTree exp_match) {
  return exp_match.matched && exp_match.token == NULL &&
         exp_match.token == NULL && exp_match.first == NULL &&
         exp_match.second == NULL;
}

bool is_newline(SyntaxTree exp_match) {
  return exp_match.matched && exp_match.token != NULL &&
         exp_match.token->type == ENDLINE;
}

SyntaxTree match(Parser *parser) {
  SyntaxTree exp_match = match_epsilon(parser);
  exp_match.token = queue_remove(&parser->queue);
  //  DEBUGF("+++TOKEN+++=%s", exp_match.token->text);
  return exp_match;
}

SyntaxTree match_rollback(Parser *parser, SyntaxTree exp_match) {
  ASSERT(exp_match.matched);

  if (NULL != exp_match.second) {
    match_rollback(parser, *exp_match.second);
    DEALLOC(exp_match.second);
  }
  if (NULL != exp_match.first) {
    match_rollback(parser, *exp_match.first);
    DEALLOC(exp_match.first);
  }
  if (NULL != exp_match.token) {
    //    DEBUGF("ROLLBACK %s", exp_match.token->text);
    queue_add_front(&parser->queue, exp_match.token);
  } else {
    //    DEBUGF("ROLLBACK %s");
  }
  return no_match(parser);
}

SyntaxTree match_merge(Parser *parser, SyntaxTree parent, SyntaxTree child) {
  if (!parent.matched) {
    return match_rollback(parser, child);
  }
  if (!child.matched) {
    return match_rollback(parser, parent);
  }

  if (is_epsilon(child)) {
    return parent;
  }

  if (is_epsilon(parent)) {
    return child;
  }

  if (is_newline(child)) {
    syntax_tree_delete(&child);
    return parent;
  }
  if (is_newline(parent)) {
    syntax_tree_delete(&parent);
    return child;
  }

  SyntaxTree m = match_epsilon(parser);
  SyntaxTree *parent_ptr = ALLOC(SyntaxTree);
  *parent_ptr = parent;
  SyntaxTree *child_ptr = ALLOC(SyntaxTree);
  *child_ptr = child;

  m.first = parent_ptr;
  m.second = child_ptr;

  return m;
}

#define ImplSyntax(name, exp)                        \
  SyntaxTree name(Parser *parser) {                  \
    if (NULL == map_lookup(parser->exp_names, name)) \
      map_insert(parser->exp_names, name, #name);    \
    /*DEBUGF(#name);*/                               \
    SyntaxTree m = exp(parser);                      \
    if (NULL == m.expression) m.expression = name;   \
    return m;                                        \
  }
#define GET_FUNC(_1, _2, _3, _4, _5, _6, _7, _8, _9, NAME, ...) NAME
#define Or2(exp1, exp2)                 \
  ({                                    \
    SyntaxTree __fn__(Parser *parser) { \
      SyntaxTree m1 = exp1(parser);     \
      if (m1.matched) return m1;        \
      SyntaxTree m2 = exp2(parser);     \
      if (m2.matched) return m2;        \
      return no_match(parser);          \
    }                                   \
    __fn__;                             \
  })
#define Or3(exp1, exp2, exp3) Or2(exp1, Or2(exp2, exp3))
#define Or4(exp1, exp2, exp3, exp4) Or2(exp1, Or3(exp2, exp3, exp4))
#define Or5(exp1, exp2, exp3, exp4, exp5) Or2(exp1, Or4(exp2, exp3, exp4, exp5))
#define Or6(exp1, exp2, exp3, exp4, exp5, exp6) \
  Or2(exp1, Or5(exp2, exp3, exp4, exp5, exp6))
#define Or7(exp1, exp2, exp3, exp4, exp5, exp6, exp7) \
  Or2(exp1, Or6(exp2, exp3, exp4, exp5, exp6, exp7))
#define Or8(exp1, exp2, exp3, exp4, exp5, exp6, exp7, exp8) \
  Or2(exp1, Or7(exp2, exp3, exp4, exp5, exp6, exp7, exp8))
#define Or9(exp1, exp2, exp3, exp4, exp5, exp6, exp7, exp8, exp9) \
  Or2(exp1, Or8(exp2, exp3, exp4, exp5, exp6, exp7, exp8, exp9))
#define Or(...) \
  GET_FUNC(__VA_ARGS__, Or9, Or8, Or7, Or6, Or5, Or4, Or3, Or2)(__VA_ARGS__)
#define And2(exp1, exp2)                            \
  ({                                                \
    SyntaxTree __fn__(Parser *parser) {             \
      SyntaxTree m1 = exp1(parser);                 \
      if (!m1.matched) return no_match(parser);     \
      return match_merge(parser, m1, exp2(parser)); \
    }                                               \
    __fn__;                                         \
  })
#define And3(exp1, exp2, exp3) And2(exp1, And2(exp2, exp3))
#define And4(exp1, exp2, exp3, exp4) And2(exp1, And3(exp2, exp3, exp4))
#define And5(exp1, exp2, exp3, exp4, exp5) \
  And2(exp1, And4(exp2, exp3, exp4, exp5))
#define And6(exp1, exp2, exp3, exp4, exp5, exp6) \
  And2(exp1, And5(exp2, exp3, exp4, exp5, exp6))
#define And7(exp1, exp2, exp3, exp4, exp5, exp6, exp7) \
  And2(exp1, And6(exp2, exp3, exp4, exp5, exp6, exp7))
#define And8(exp1, exp2, exp3, exp4, exp5, exp6, exp7, exp8) \
  And2(exp1, And7(exp2, exp3, exp4, exp5, exp6, exp7, exp8))
#define And9(exp1, exp2, exp3, exp4, exp5, exp6, exp7, exp8, exp9) \
  And2(exp1, And8(exp2, exp3, exp4, exp5, exp6, exp7, exp8, exp9))
#define And(...)                                                        \
  GET_FUNC(__VA_ARGS__, And9, And8, And7, And6, And5, And4, And3, And2) \
  (__VA_ARGS__)
#define Type(tok_type)                            \
  ({                                              \
    SyntaxTree __fn__(Parser *parser) {           \
      Token *tok = parser_next(parser);           \
      if (NULL == tok || tok_type != tok->type) { \
        return no_match(parser);                  \
      }                                           \
      return match(parser);                       \
    }                                             \
    __fn__;                                       \
  })
#define Opt(exp)                        \
  ({                                    \
    SyntaxTree __fn__(Parser *parser) { \
      SyntaxTree m = exp(parser);       \
      if (m.matched) return m;          \
      return match_epsilon(parser);     \
    }                                   \
    __fn__;                             \
  })
#define Epsilon                                                         \
  ({                                                                    \
    SyntaxTree __fn__(Parser *parser) { return match_epsilon(parser); } \
    __fn__;                                                             \
  })
#define Ln(exp) And(exp, Opt(Type(ENDLINE)))
#define TypeLn(tok_type) Ln(Type(tok_type))

// clang-format off

DefineSyntax(tuple_expression);
ImplSyntax(identifier, Or(Type(WORD), Type(CLASS), Type(MODULE_T)));
ImplSyntax(new_expression, Type(NEW));
ImplSyntax(constant, Or(Type(INTEGER), Type(FLOATING)));
ImplSyntax(string_literal, Type(STR));

// array_declaration
//     [ ]
//     [ tuple_expression ]
ImplSyntax(array_declaration,
           Or(And(Type(LBRAC), Type(RBRAC)),
              And(Type(LBRAC), tuple_expression, Type(RBRAC))));

ImplSyntax(map_declaration_entry,
    And(postfix_expression, TypeLn(COLON), Ln(postfix_expression)));

ImplSyntax(map_declaration_entry1,
    Or(And(And(TypeLn(COMMA), Ln(map_declaration_entry)), map_declaration_entry1),
       Epsilon));

ImplSyntax(map_declaration_list,
    And(map_declaration_entry, map_declaration_entry1));

ImplSyntax(map_declaration,
           Or(And(TypeLn(LBRCE), TypeLn(RBRCE)),
              And(TypeLn(LBRCE), Ln(map_declaration_list), TypeLn(RBRCE))));

// length_expression
//     | expression |
ImplSyntax(length_expression, And(Type(PIPE), tuple_expression, Type(PIPE)));

// primary_expression
//     identifier
//     constant
//     string_literal
//     array_declaration
//     map_declaration
//     anon_function_definition
//     ( expression )
ImplSyntax(primary_expression,
           Or(anon_function_definition,
              identifier,
              new_expression,
              constant,
              string_literal,
              array_declaration,
              map_declaration,
              length_expression,
              And(Type(LPAREN), tuple_expression, Type(RPAREN))));


ImplSyntax(primary_expression_no_constants,
           Or(anon_function_definition,
              identifier,
              string_literal,
              array_declaration,
              map_declaration));

DefineSyntax(assignment_expression);

// postfix_expression
//     primary_expression
//     postfix_expression [ experssion ]
//     postfix_expression ( )
//     postfix_expression ( tuple_expression )
//     postfix_expression . identifier
//     postfix_expression ++
//     postfix_expression --
//     anon_function_definition
ImplSyntax(
    postfix_expression1,
    Or(And(Type(LBRAC), tuple_expression, Type(RBRAC), postfix_expression1),
       And(TypeLn(LPAREN), TypeLn(RPAREN), postfix_expression1),
       And(TypeLn(LPAREN), tuple_expression, TypeLn(RPAREN), postfix_expression1),
       And(Opt(Type(ENDLINE)), Type(PERIOD), Or(identifier, new_expression), postfix_expression1),
//       And(Type(INCREMENT), postfix_expression1),
//       And(Type(DECREMENT), postfix_expression1),
       Epsilon));
ImplSyntax(postfix_expression,
           Or(And(primary_expression_no_constants, postfix_expression1),
              primary_expression));

ImplSyntax(range_expression,
           And(postfix_expression,
               Opt(And(Type(COLON), postfix_expression,
                       Opt(And(Type(COLON), postfix_expression))))));

// unary_expression
//    postfix_expression
//    ~ unary_expression
//    ! unary_expression
//    - unary_expression
//    ++ unary_expression
//    -- unary_expression
//    const unary_expression
ImplSyntax(unary_expression,
           Or(And(Type(TILDE), unary_expression),
              And(Type(EXCLAIM), unary_expression),
              And(Type(MINUS), unary_expression),
              //       And(Type(INC), unary_expression),
              //       And(Type(DEC), unary_expression),
              And(Type(CONST_T), unary_expression),
              range_expression));

// multiplicative_expression
//    unary_expression
//    multiplicative_expression * unary_expression
//    multiplicative_expression / unary_expression
//    multiplicative_expression % unary_expression
ImplSyntax(multiplicative_expression1,
           Or(And(Type(STAR), multiplicative_expression),
              And(Type(FSLASH), multiplicative_expression),
              And(Type(PERCENT), multiplicative_expression),
              Epsilon));
ImplSyntax(multiplicative_expression,
           And(unary_expression, multiplicative_expression1));

// additive_expression
//    multiplicative_expression
//    additive_expression + multiplicative_expression
//    additive_expression - multiplicative_expression
ImplSyntax(additive_expression1,
           Or(And(Type(PLUS), additive_expression),
              And(Type(MINUS), additive_expression), Epsilon));
ImplSyntax(additive_expression,
           And(multiplicative_expression, additive_expression1));

// in_expression
//    additive_expression
//    in_expression in additive_expression
ImplSyntax(in_expression1,
           Or(And(
               Or(Type(IN), Type(NOTIN)),
               additive_expression, in_expression1),
              Epsilon));
ImplSyntax(in_expression,
           And(additive_expression, in_expression1));

// relational_expression
//    in_expression
//    relational_expression < in_expression
//    relational_expression > in_expression
//    relational_expression <= in_expression
//    relational_expression >= in_expression
ImplSyntax(relational_expression1,
           Or(And(Type(LTHAN), relational_expression),
              And(Type(GTHAN), relational_expression),
              And(Type(LTHANEQ), relational_expression),
              And(Type(GTHANEQ), relational_expression),
              Epsilon));
ImplSyntax(relational_expression,
           And(in_expression, relational_expression1));

// equality_expression
//    relational_expression
//    equality_expression == relational_expression
//    equality_expression != relational_expression
ImplSyntax(equality_expression1,
           Or(And(Type(EQUIV), equality_expression),
              And(Type(NEQUIV), equality_expression),
              Epsilon));
ImplSyntax(equality_expression,
           And(relational_expression, equality_expression1));

// and_expression
//    equality_expression
//    and_expression & equality_expression
ImplSyntax(and_expression1,
           Or(And(Type(AND_T), and_expression),
              Epsilon));
ImplSyntax(and_expression,
           And(equality_expression, and_expression1));

// xor_expression
//    and_expression
//    xor_expression ^ and_expression
ImplSyntax(xor_expression1,
           Or(And(Type(CARET), xor_expression),
              Epsilon));
ImplSyntax(xor_expression,
           And(and_expression, xor_expression1));

// or_expression
//    xor_expression
//    or_expression | xor_expression
ImplSyntax(or_expression1,
           Or(And(Type(OR_T), or_expression),
              Epsilon));
ImplSyntax(or_expression,
           And(xor_expression, or_expression1));

// is_expression
//    or_expression
//    or_expression is identifier
ImplSyntax(is_expression,
           Or(And(or_expression, TypeLn(IS_T), or_expression),
              or_expression));

// conditional_expression
//    or_expression
//    if is_expression conditional_expression
//    if is_expression then conditional_expression
//    if is_expression conditional_expression else conditional_expression
//    if is_expression then conditional_expression else conditional_expression
ImplSyntax(conditional_expression,
           Or(And(TypeLn(IF_T), Ln(is_expression), Opt(TypeLn(THEN)),
                  Ln(conditional_expression), TypeLn(ELSE),
                  conditional_expression),
              And(TypeLn(IF_T), Ln(is_expression), Opt(TypeLn(THEN)),
                  conditional_expression),
              is_expression));

ImplSyntax(const_assignment_expression,
    And(TypeLn(CONST_T), identifier));

ImplSyntax(array_index_assignment,
    And(TypeLn(LBRAC), tuple_expression, TypeLn(RBRAC)));

ImplSyntax(function_call_args,
    Or(And(TypeLn(LPAREN), TypeLn(RPAREN)),
       And(TypeLn(LPAREN), tuple_expression, TypeLn(RPAREN))));

ImplSyntax(field_extension,
    Or(array_index_assignment,
       function_call_args,
       Epsilon));

ImplSyntax(field_set_value,
    And(Type(PERIOD), identifier));

ImplSyntax(field_suffix,
    Or(field_set_value,
       array_index_assignment));

DefineSyntax(field_expression1);

ImplSyntax(field_next,
    And(field_suffix, field_expression1));

ImplSyntax(field_expression1,
    Or(And(field_extension, field_next),
       field_next,
       Epsilon));

ImplSyntax(field_expression,
    And(identifier, field_expression1));

// assignment_lhs_single
//    const x
//    x.y.z
ImplSyntax(assignment_lhs_single,
    Or(const_assignment_expression,
       field_expression));

ImplSyntax(assignment_tuple_list1,
   Or(And(And(TypeLn(COMMA), assignment_lhs), assignment_tuple_list1),
      Epsilon));

ImplSyntax(assignment_tuple_list,
    And(assignment_lhs, assignment_tuple_list1));

// assignment_tuple
//    ( assignment_tuple_list )
ImplSyntax(assignment_tuple,
           And(TypeLn(LPAREN), assignment_tuple_list, TypeLn(RPAREN)));

ImplSyntax(assignment_array,
    And(TypeLn(LBRAC), assignment_tuple_list, TypeLn(RBRAC)));

ImplSyntax(assignment_lhs,
    Or(assignment_tuple,
       assignment_array,
       assignment_lhs_single
        ));

// assignment_expression
//    conditional_expression
//    assignment_tuple = assignment_expression
//    unary_expression = assignment_expression
ImplSyntax(assignment_expression,
           Or(And(assignment_lhs, TypeLn(EQUALS), conditional_expression),
              conditional_expression));

// expression
//    assignment_expression
//    tuple_expression , assignment_expression
ImplSyntax(tuple_expression1,
           Or(And(TypeLn(COMMA), assignment_expression, tuple_expression1),
              Epsilon));
ImplSyntax(tuple_expression, And(assignment_expression, tuple_expression1));

// expression_statement
//    \n
//    expression \n
ImplSyntax(expression_statement, Ln(tuple_expression));

DefineSyntax(statement);
DefineSyntax(statement_list);

ImplSyntax(foreach_statement,
           And(And(TypeLn(FOR),
                 Or(And(TypeLn(LPAREN), And(assignment_lhs, TypeLn(IN), conditional_expression), TypeLn(RPAREN)),
                     And(assignment_lhs, TypeLn(IN), conditional_expression))),
               statement));

ImplSyntax(
    for_statement,
    And(And(TypeLn(FOR),
        Or(And(TypeLn(LPAREN),
               And(assignment_expression, TypeLn(COMMA), conditional_expression,
               TypeLn(COMMA), Ln(assignment_expression)),
            TypeLn(RPAREN)),
            And(assignment_expression, TypeLn(COMMA), conditional_expression,
               TypeLn(COMMA), Ln(assignment_expression))
           )),
        statement));

ImplSyntax(while_statement,
           And(And(TypeLn(WHILE), Ln(conditional_expression)),
               statement));

// iteration_statement
//    while expression  statement
//    for (identifier in expression) statement
//    for ( expression , expression , expression ) statement
ImplSyntax(iteration_statement,
           Or(while_statement,
              foreach_statement,
              for_statement));

// compound_statement
//    { }
//    { statement_list }
ImplSyntax(compound_statement,
           Or(And(TypeLn(LBRCE), TypeLn(RBRCE)),
              And(TypeLn(LBRCE), Ln(statement_list), TypeLn(RBRCE))));

// selection_statement
//    if expression statement
//    if expression then statement
//    if expression statement else statement
//    if expression then statement else statement
ImplSyntax(selection_statement,
           And(Type(IF_T), Ln(tuple_expression), Ln(statement),
               Opt(And(TypeLn(ELSE), statement))));

// exit_statement
//    exit
ImplSyntax(exit_statement, TypeLn(EXIT_T));

// raise_statement
//    raise expression
ImplSyntax(raise_statement, And(TypeLn(RAISE), Ln(assignment_expression)));

ImplSyntax(catch_assign,
    Or(And(TypeLn(LPAREN),
        assignment_lhs, TypeLn(RPAREN)),
       assignment_lhs));

// try_statement
//    try statement catch identifier statement
//    try statement catch ( identifier ) statement
ImplSyntax(try_statement,
           Or(And(And(TypeLn(TRY), Ln(statement), And(TypeLn(CATCH), catch_assign)), Ln(statement)),
              And(And(TypeLn(TRY), Ln(statement), And(TypeLn(CATCH), catch_assign)),
                  Ln(statement))));

// jump_statement
//    return \n
//    return expression \n
ImplSyntax(jump_statement,
           Or(And(Type(RETURN), TypeLn(LPAREN), tuple_expression,
                  TypeLn(RPAREN)),
              And(Type(RETURN), tuple_expression),
              Type(RETURN)));

// break_statement
//    break \n
//    continue \n
ImplSyntax(break_statement,
           And(Or(Type(BREAK), Type(CONTINUE)), Type(ENDLINE)));

ImplSyntax(default_value_expression, And(Type(EQUALS), conditional_expression));

ImplSyntax(const_expression,
           And(Opt(TypeLn(CONST_T)),
               Or(And(TypeLn(LPAREN), function_argument_list, TypeLn(RPAREN)),
                  identifier),
               Opt(default_value_expression)));

ImplSyntax(function_arg_default_value,
           And(TypeLn(EQUALS), conditional_expression));

ImplSyntax(function_arg_elt_with_default,
           And(identifier, function_arg_default_value));

ImplSyntax(function_arg_elt,
           Or(function_arg_elt_with_default,
              identifier));

ImplSyntax(const_function_argument,
    And(TypeLn(CONST_T), function_arg_elt));

ImplSyntax(function_argument,
           Or(const_function_argument,
              function_arg_elt));

ImplSyntax(function_argument_list1,
           Or(And(And(TypeLn(COMMA), function_argument), function_argument_list1),
              Epsilon));

ImplSyntax(function_argument_list,
           And(function_argument, function_argument_list1));

ImplSyntax(function_arguments_present,
           And(TypeLn(LPAREN), function_argument_list, TypeLn(RPAREN)));

ImplSyntax(function_arguments_no_args,
           And(TypeLn(LPAREN), TypeLn(RPAREN)));

ImplSyntax(function_arguments,
           Or(function_arguments_present,
               function_arguments_no_args));

ImplSyntax(def_identifier,
           And(Type(DEF), identifier));

ImplSyntax(function_signature_nonconst,
           And(def_identifier, function_arguments));

ImplSyntax(function_signature_const,
           And(function_signature_nonconst, TypeLn(CONST_T)));

ImplSyntax(function_signature,
           Or(function_signature_const,
              function_signature_nonconst));

ImplSyntax(function_definition,
           And(function_signature,
               statement));

ImplSyntax(method_identifier,
           And(Type(METHOD), identifier));

ImplSyntax(method_signature_nonconst,
           And(method_identifier, function_arguments));

ImplSyntax(method_signature_const,
           And(method_signature_nonconst, TypeLn(CONST_T)));

ImplSyntax(method_signature,
           Or(method_signature_const,
               method_signature_nonconst));

ImplSyntax(method_definition,
           And(method_signature,
               statement));

ImplSyntax(new_arg_var,
           Or(new_field_arg,
              identifier));

ImplSyntax(new_field_arg,
           And(TypeLn(FIELD), identifier));

ImplSyntax(new_arg_default_value,
           And(TypeLn(EQUALS), conditional_expression));

ImplSyntax(new_arg_elt_with_default,
           And(new_arg_var, new_arg_default_value));

ImplSyntax(new_arg_elt,
           Or(new_arg_elt_with_default,
               new_arg_var));

ImplSyntax(const_new_argument,
    And(TypeLn(CONST_T), new_arg_elt));

ImplSyntax(new_argument,
           Or(const_new_argument,
               new_arg_elt));

ImplSyntax(new_argument_list1,
           Or(And(And(TypeLn(COMMA), new_argument), new_argument_list1),
              Epsilon));

ImplSyntax(new_argument_list,
           And(new_argument, new_argument_list1));

ImplSyntax(new_arguments_present,
           And(TypeLn(LPAREN), new_argument_list, TypeLn(RPAREN)));

ImplSyntax(new_arguments_no_args,
           And(TypeLn(LPAREN), TypeLn(RPAREN)));

ImplSyntax(new_arguments,
           Or(new_arguments_present,
               new_arguments_no_args));

ImplSyntax(new_identifier, new_expression);

ImplSyntax(new_signature_nonconst,
           And(new_identifier, new_arguments));

ImplSyntax(new_signature_const,
           And(new_signature_nonconst, TypeLn(CONST_T)));

ImplSyntax(new_signature,
           Or(new_signature_const,
              new_signature_nonconst));

ImplSyntax(new_definition,
           And(new_signature,
               statement));

ImplSyntax(anon_signature_nonconst,
           function_arguments);

ImplSyntax(anon_signature_const,
           And(anon_signature_nonconst, TypeLn(CONST_T)));

ImplSyntax(anon_signature,
           Or(anon_signature_const,
              anon_signature_nonconst));


ImplSyntax(anon_function_lambda_lhs,
           Or(anon_signature,
              identifier));

ImplSyntax(anon_function_lambda_rhs,
           And(TypeLn(RARROW), assignment_expression));

ImplSyntax(anon_function_definition,
           Or(And(anon_function_lambda_lhs,
                  anon_function_lambda_rhs),
              And(anon_signature,
                  compound_statement)));

ImplSyntax(identifier_list1,
    Or(And(And(TypeLn(COMMA), identifier), identifier_list1),
       Epsilon));

ImplSyntax(identifier_list,
    And(identifier, identifier_list1));

// field_statement
//    field function_argument_lsit
ImplSyntax(field_statement, And(TypeLn(FIELD), identifier_list));

// class_statement
//    new_definition
//    function_definition
//    field_statement
ImplSyntax(class_statement,
           Ln(Or(new_definition,
                 field_statement,
                 method_definition)));

// class_statement_list
//    class_statement
//    class_statement class_statement_list
ImplSyntax(class_statement_list1,
           Or(And(Ln(class_statement), class_statement_list1),
              Epsilon));
ImplSyntax(class_statement_list,
           And(Ln(class_statement), class_statement_list1));

// parent_class_list
//    identifier
//    identifier , paret_class_list
ImplSyntax(parent_class_list1,
           Or(And(And(TypeLn(COMMA), Ln(identifier)), parent_class_list1),
              Epsilon));
ImplSyntax(parent_class_list, And(Ln(identifier), parent_class_list1));

// parent_classes
//    : parent_class_list
ImplSyntax(parent_classes,
           And(TypeLn(COLON), parent_class_list));

ImplSyntax(class_compound_statement,
    Or(And(TypeLn(LBRCE), TypeLn(RBRCE)),
       And(TypeLn(LBRCE), Ln(class_statement_list), TypeLn(RBRCE))));

ImplSyntax(class_name_and_inheritance,
    And(Ln(identifier), Opt(parent_classes)));

// class_definition
//    class identifier statement
//    class identifier parent_classes statement
ImplSyntax(class_definition,
           And(And(Type(CLASS), class_name_and_inheritance),
               Or(class_compound_statement,
                  Ln(class_statement))));

// import_statement
//    import identifier
ImplSyntax(import_statement, Or(And(Type(IMPORT), identifier, Type(AS),
                                    identifier, Type(ENDLINE)),
                                And(Type(IMPORT), identifier, Type(ENDLINE))));

// module_statement
//    module identifier
ImplSyntax(module_statement, And(Type(MODULE_T), identifier, Type(ENDLINE)));

// statement
//    exit_statement
//    raise_statement
//    try_statement
//    selection_statement
//    expression_statement
//    compound_statement
//    iteration_statement
//    jump_statement
//    break_statement
ImplSyntax(statement,
           Or(exit_statement,
              raise_statement,
              try_statement,
              selection_statement,
              compound_statement,
              iteration_statement,
              jump_statement,
              break_statement,
              expression_statement));

// file_level_statement
//    module_statement
//    import_statement
//    class_definition
//    function_definition
//    statement
ImplSyntax(file_level_statement,
           Or(module_statement,
              import_statement,
              class_definition,
              function_definition,
              statement));

// file_level_statement_list
//    file_level_statement
//    file_level_statement_list statement
ImplSyntax(file_level_statement_list1,
           Or(And(file_level_statement, file_level_statement_list1),
              Epsilon));
ImplSyntax(file_level_statement_list,
           And(file_level_statement, file_level_statement_list1));

// statement_list
//    statement
//    statement_list statement
ImplSyntax(statement_list1,
    Or(And(statement, statement_list1),
       Epsilon));
ImplSyntax(statement_list,
    And(statement, statement_list1));
