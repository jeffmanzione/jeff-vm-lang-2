/*
 * expression.c
 *
 *  Created on: Jan 19, 2017
 *      Author: Jeff
 */

#include "expression.h"

#include "error.h"
#include "map.h"
#include "memory.h"
#include "parse.h"
#include "queue.h"

void print_tabs(FILE *file, int num_tabs) {
  int i;
  for(i=0; i < num_tabs; i++) {
    fprintf(file, "\t");
  }
}

void exp_to_str_helper(ExpressionTree exp, Parser *parser, FILE *file,
    int num_tabs) {
  print_tabs(file, num_tabs);
  if (NULL != exp.expression && parser != NULL) {
    fprintf(file, "[%s] ",
        (char *) map_lookup(parser->exp_names, exp.expression));
  }
  if (NULL == exp.token && NULL == exp.first) {
    fprintf(file, "E");
    return;
  } else if (NULL != exp.token) {
    fprintf(file, "'%s'", exp.token->type == ENDLINE ? "\\n" : exp.token->text);
  }
  if (NULL == exp.first) {
    return;
  }
  fprintf(file, "{\n");
  exp_to_str_helper(*exp.first, parser, file, num_tabs+1);
  if (NULL != exp.second) {
    fprintf(file, ",\n");
    exp_to_str_helper(*exp.second, parser, file, num_tabs+1);
  }
  fprintf(file, "\n");
  print_tabs(file, num_tabs);
  fprintf(file, "}");
  fflush(file);
}

void expression_tree_to_str(ExpressionTree exp, Parser *parser, FILE *file) {
  exp_to_str_helper(exp, parser, file, 0);
}

void expression_tree_delete(ExpressionTree exp) {
//  if (exp.token != NULL) {
//    token_delete(exp.token);
//  }
  if (exp.first != NULL) {
    expression_tree_delete(*exp.first);
    DEALLOC(exp.first);
  }
  if (exp.second != NULL) {
    expression_tree_delete(*exp.second);
    DEALLOC(exp.second);
  }
}

ExpressionTree no_match(Parser *parser) {
  ExpressionTree exp_match;
  exp_match.matched = false;
  exp_match.token = NULL;
  exp_match.first = NULL;
  exp_match.second = NULL;
  exp_match.expression = NULL;
  return exp_match;
}

ExpressionTree match_epsilon(Parser *parser) {
  ExpressionTree exp_match;
  exp_match.matched = true;
  exp_match.token = NULL;
  exp_match.first = NULL;
  exp_match.second = NULL;
  exp_match.expression = NULL;
  return exp_match;
}

bool is_epsilon(ExpressionTree exp_match) {
  return exp_match.matched && exp_match.token == NULL && exp_match.token == NULL
      && exp_match.first == NULL && exp_match.second == NULL;
}

bool is_newline(ExpressionTree exp_match) {
  return exp_match.matched && exp_match.token != NULL
      && exp_match.token->type == ENDLINE;
}

ExpressionTree match(Parser *parser) {
  ExpressionTree exp_match = match_epsilon(parser);
  exp_match.token = queue_remove(&parser->queue);
//  DEBUGF("match(%d)", exp_match.token->type);
  return exp_match;
}

ExpressionTree match_rollback(Parser *parser, ExpressionTree exp_match) {
//  DEBUGF("match_rollback");
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
    queue_add_front(&parser->queue, exp_match.token);
  }
  return no_match(parser);
}

ExpressionTree match_merge(Parser *parser, ExpressionTree parent, ExpressionTree child) {
//  DEBUGF("match_merge");
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
    expression_tree_delete(child);
    return parent;
  }
  if (is_newline(parent)) {
    expression_tree_delete(parent);
    return child;
  }

  ExpressionTree m = match_epsilon(parser);
  ExpressionTree *parent_ptr = ALLOC(ExpressionTree);
  *parent_ptr = parent;
  ExpressionTree *child_ptr = ALLOC(ExpressionTree);
  *child_ptr = child;

  m.first = parent_ptr;
  m.second = child_ptr;

  return m;
}

#define ImplExpression(name, exp)                    \
  ExpressionTree name(Parser *parser) {              \
    if (NULL == map_lookup(parser->exp_names, name)) \
      map_insert(parser->exp_names, name, #name);    \
    /*DEBUGF(#name);*/                               \
    ExpressionTree m = exp(parser);                  \
    if (NULL == m.expression)                        \
      m.expression = name;                           \
    return m;                                        \
  }
#define GET_FUNC(_1,_2,_3,_4,_5,_6,_7,_8,_9, NAME,...) NAME
#define Or2(exp1, exp2)                     \
  ({ExpressionTree __fn__(Parser *parser) { \
      ExpressionTree m1 = exp1(parser);     \
      if (m1.matched) return m1;            \
      ExpressionTree m2 = exp2(parser);     \
      if (m2.matched) return m2;            \
      return no_match(parser);              \
    } __fn__;                               \
  })
#define Or3(exp1, exp2, exp3) Or2(exp1, Or2(exp2, exp3))
#define Or4(exp1, exp2, exp3, exp4) Or2(exp1, Or3(exp2, exp3, exp4))
#define Or5(exp1, exp2, exp3, exp4, exp5) Or2(exp1, Or4(exp2, exp3, exp4, exp5))
#define Or6(exp1, exp2, exp3, exp4, exp5, exp6) Or2(exp1, Or5(exp2, exp3, exp4, exp5, exp6))
#define Or7(exp1, exp2, exp3, exp4, exp5, exp6, exp7) Or2(exp1, Or6(exp2, exp3, exp4, exp5, exp6, exp7))
#define Or8(exp1, exp2, exp3, exp4, exp5, exp6, exp7, exp8) Or2(exp1, Or7(exp2, exp3, exp4, exp5, exp6, exp7, exp8))
#define Or9(exp1, exp2, exp3, exp4, exp5, exp6, exp7, exp8, exp9) Or2(exp1, Or8(exp2, exp3, exp4, exp5, exp6, exp7, exp8, exp9))
#define Or(...) GET_FUNC(__VA_ARGS__,Or9,Or8,Or7,Or6,Or5,Or4,Or3,Or2)(__VA_ARGS__)
#define And2(exp1, exp2)                                       \
  ({ExpressionTree __fn__(Parser *parser) {                    \
      ExpressionTree m1 = exp1(parser);                        \
      if (!m1.matched) return no_match(parser);                \
      return match_merge(parser, m1, exp2(parser));            \
    } __fn__;                                                  \
  })
#define And3(exp1, exp2, exp3) And2(exp1, And2(exp2, exp3))
#define And4(exp1, exp2, exp3, exp4) And2(exp1, And3(exp2, exp3, exp4))
#define And5(exp1, exp2, exp3, exp4, exp5) And2(exp1, And4(exp2, exp3, exp4, exp5))
#define And6(exp1, exp2, exp3, exp4, exp5, exp6) And2(exp1, And5(exp2, exp3, exp4, exp5, exp6))
#define And7(exp1, exp2, exp3, exp4, exp5, exp6, exp7) And2(exp1, And6(exp2, exp3, exp4, exp5, exp6, exp7))
#define And8(exp1, exp2, exp3, exp4, exp5, exp6, exp7, exp8) And2(exp1, And7(exp2, exp3, exp4, exp5, exp6, exp7, exp8))
#define And9(exp1, exp2, exp3, exp4, exp5, exp6, exp7, exp8, exp9) And2(exp1, And8(exp2, exp3, exp4, exp5, exp6, exp7, exp8, exp9))
#define And(...) GET_FUNC(__VA_ARGS__,And9,And8,And7,And6,And5,And4,And3,And2)(__VA_ARGS__)
#define Type(tok_type)                            \
  ({ExpressionTree __fn__(Parser *parser) {       \
      Token *tok = parser_next(parser);           \
      if (NULL == tok || tok_type != tok->type) { \
        return no_match(parser);                  \
      }                                           \
      return match(parser);                       \
    } __fn__;                                     \
  })
#define Opt(exp)                                  \
  ({ExpressionTree __fn__(Parser *parser) {       \
      ExpressionTree m = exp(parser);             \
      if (m.matched) return m;                    \
      return match_epsilon(parser);               \
    } __fn__;                                     \
  })
#define Epsilon                                   \
  ({ExpressionTree __fn__(Parser *parser) {       \
      return match_epsilon(parser);               \
    } __fn__;                                     \
  })
#define Ln(exp)                                   \
  And(exp, Opt(Type(ENDLINE)))
#define TypeLn(tok_type)                          \
  Ln(Type(tok_type))

DefineExpression(tuple_expression);
ImplExpression(identifier, Or(Type(WORD), Type(CLASS), Type(MODULE_T)));
ImplExpression(constant, Or(Type(INTEGER), Type(FLOATING)));
ImplExpression(string_literal, Type(STR));

// array_declaration
//     [ ]
//     [ tuple_expression ]
ImplExpression(array_declaration,
    Or(And(Type(LBRAC), Type(RBRAC)),
       And(Type(LBRAC), tuple_expression, Type(RBRAC))));

// length_expression
//     | expression |
//
ImplExpression(length_expression, And(Type(PIPE), tuple_expression, Type(PIPE)))

// primary_expression
//     identifier
//     constant
//     string_literal
//     array_declaration
//     ( expression )
ImplExpression(primary_expression,
    Or(identifier,
       constant,
       string_literal,
       array_declaration,
       length_expression,
       And(Type(LPAREN), tuple_expression, Type(RPAREN))));

ImplExpression(primary_expression_no_constants,
    Or(identifier,
       string_literal,
       array_declaration,
       And(Type(LPAREN), tuple_expression, Type(RPAREN))));

DefineExpression(assignment_expression);

//// argument_expression_list
////    assignment_expression
////    argument_expression-list , assignment_expression
//ImplExpression(argument_expression_list1,
//    Or(And(Type(COMMA), assignment_expression, argument_expression_list1),
//       Epsilon));
//ImplExpression(argument_expression_list,
//    And(assignment_expression, argument_expression_list1));

// postfix_expression
//     primary_expression
//     postfix_expression [ experssion ]
//     postfix_expression ( )
//     postfix_expression ( tuple_expression )
//     postfix_expression . identifier
//     postfix_expression ++
//     postfix_expression --
//     anon_function
ImplExpression(postfix_expression1,
    Or(And(Type(LBRAC), tuple_expression, Type(RBRAC), postfix_expression1),
       And(Type(LPAREN), Type(RPAREN), postfix_expression1),
       And(Type(LPAREN), tuple_expression, Type(RPAREN), postfix_expression1),
       And(Type(PERIOD), identifier, postfix_expression1),
//       And(Type(INC), postfix_expression1),
//       And(Type(DEC), postfix_expression1),
       primary_expression,
       Epsilon));
ImplExpression(postfix_expression,
    Or(And(primary_expression_no_constants, postfix_expression1),
       primary_expression));

// unary_expression
//    postfix_expression
//    ~ unary_expression
//    ! unary_expression
//    ++ unary_expression
//    -- unary_expression
ImplExpression(unary_expression,
    Or(And(Type(TILDE), unary_expression),
       And(Type(EXCLAIM), unary_expression),
//       And(Type(INC), unary_expression),
//       And(Type(DEC), unary_expression),
       postfix_expression));

// multiplicative_expression
//    unary_expression
//    multiplicative_expression * unary_expression
//    multiplicative_expression / unary_expression
//    multiplicative_expression % unary_expression
ImplExpression(multiplicative_expression1,
    Or(And(Type(STAR), multiplicative_expression),
       And(Type(FSLASH), multiplicative_expression),
       And(Type(PERCENT), multiplicative_expression),
       Epsilon));
ImplExpression(multiplicative_expression,
    And(unary_expression, multiplicative_expression1));

// additive_expression
//    multiplicative_expression
//    additive_expression + multiplicative_expression
//    additive_expression - multiplicative_expression
ImplExpression(additive_expression1,
    Or(And(Type(PLUS), additive_expression),
       And(Type(MINUS), additive_expression),
       Epsilon));
ImplExpression(additive_expression,
    And(multiplicative_expression, additive_expression1));

// relational_expression
//    shift_expression
//    relational_expression < additive_expression
//    relational_expression > additive_expression
//    relational_expression <= additive_expression
//    relational_expression >= additive_expression
ImplExpression(relational_expression1,
    Or(And(Type(LTHAN), additive_expression, relational_expression1),
       And(Type(GTHAN), additive_expression, relational_expression1),
       And(Type(LTHANEQ), additive_expression, relational_expression1),
       And(Type(GTHANEQ), additive_expression, relational_expression1),
       Epsilon));
ImplExpression(relational_expression,
    And(additive_expression, relational_expression1));

// equality_expression
//    relational_expression
//    equality_expression == relational_expression
//    equality_expression != relational_expression
ImplExpression(equality_expression1,
    Or(And(Type(EQUIV), relational_expression, equality_expression1),
       And(Type(NEQUIV), relational_expression, equality_expression1),
       Epsilon));
ImplExpression(equality_expression,
    And(relational_expression, equality_expression1));

// and_expression
//    equality_expression
//    and_expression & equality_expression
ImplExpression(and_expression1,
    Or(And(Type(AMPER), equality_expression, and_expression1),
       Epsilon));
ImplExpression(and_expression, And(equality_expression, and_expression1));

// xor_expression
//    and_expression
//    xor_expression ^ and_expression
ImplExpression(xor_expression1,
    Or(And(Type(CARET), and_expression, xor_expression1),
       Epsilon));
ImplExpression(xor_expression, And(and_expression, xor_expression1));

// or_expression
//    xor_expression
//    or_expression | xor_expression
ImplExpression(or_expression1,
    Or(And(Type(PIPE), xor_expression, or_expression1),
       Epsilon));
ImplExpression(or_expression, And(xor_expression, or_expression1));

// is_expression
//    or_expression
//    or_expression is identifier
ImplExpression(is_expression,
    Or(And(or_expression, TypeLn(IS_T), identifier),
       or_expression));

// conditional_expression
//    or_expression
//    if is_expression conditional_expression
//    if is_expression then conditional_expression
//    if is_expression conditional_expression else conditional_expression
//    if is_expression then conditional_expression else conditional_expression
ImplExpression(conditional_expression,
    Or(And(TypeLn(IF_T), Ln(is_expression), Opt(TypeLn(THEN)), Ln(conditional_expression),
           TypeLn(ELSE), conditional_expression),
       And(TypeLn(IF_T), Ln(is_expression), Opt(TypeLn(THEN)), conditional_expression),
       is_expression));

ImplExpression(anon_function,
    Or(And(Type(AT), Type(LPAREN), Opt(function_argument_list), TypeLn(RPAREN), statement),
           conditional_expression));

// assignment_expression
//    conditional_expression
//    assignment_tuple = assignment_expression
//    unary_expression = assignment_expression
ImplExpression(assignment_expression,
    Or(And(assignment_tuple, Type(EQUALS), assignment_expression),
       And(unary_expression, Type(EQUALS), assignment_expression),
       anon_function));

// assignment_tuple
//    ( function_argument_List )
ImplExpression(assignment_tuple, And(TypeLn(LPAREN), function_argument_list, TypeLn(RPAREN)));

//expression
//    assignment_expression
//    tuple_expression , assignment_expression
ImplExpression(tuple_expression1,
    Or(And(Type(COMMA), assignment_expression, tuple_expression1),
       Epsilon));
ImplExpression(tuple_expression, And(assignment_expression, tuple_expression1));


// expression_statement
//    \n
//    expression \n
ImplExpression(expression_statement,
    Ln(tuple_expression));

DefineExpression(statement);
DefineExpression(statement_list);

// iteration_statement
//    while expression  statement
//    for ( expression , expression , expression ) statement
ImplExpression(iteration_statement,
    Or(And(TypeLn(WHILE), Ln(tuple_expression), statement),
       And(TypeLn(FOR), Or(And(assignment_expression, TypeLn(COMMA), assignment_expression, TypeLn(COMMA), Ln(assignment_expression)),
                           And(TypeLn(LPAREN), And(assignment_expression, TypeLn(COMMA), assignment_expression, TypeLn(COMMA), Ln(assignment_expression)), TypeLn(RPAREN))), statement)));

// compound_statement
//    { }
//    { statement_list }
ImplExpression(compound_statement,
    Or(And(TypeLn(LBRCE), TypeLn(RBRCE)),
       And(TypeLn(LBRCE), Ln(statement_list), TypeLn(RBRCE))));

// selection_statement
//    if expression statement
//    if expression then statement
//    if expression statement else statement
//    if expression then statement else statement
ImplExpression(selection_statement,
    And(Type(IF_T), Ln(tuple_expression), Ln(statement), Opt(And(TypeLn(ELSE), statement))));

// jump_statement
//    return \n
//    return expression \n
ImplExpression(jump_statement,
    Or(And(Type(RETURN), Type(ENDLINE)),
       And(Type(RETURN), tuple_expression, Type(ENDLINE))));

// function_argument_list
//    identifier | function_argument_list , identifier
ImplExpression(function_argument_list1,
    Or(And(Type(COMMA), identifier, function_argument_list1),
       Epsilon));
ImplExpression(function_argument_list,
    And(identifier, function_argument_list1));

// function_definition
//    def identifier ( argument_list ) statement
ImplExpression(function_definition,
      And(Type(DEF), identifier, Type(LPAREN), Opt(function_argument_list), TypeLn(RPAREN), statement));

// field_statement
//    field function_argument_lsit
ImplExpression(field_statement,
    And(TypeLn(FIELD), function_argument_list));

// class_constructor_list
//    identifier ( argument_list )
//    identifier ( argument_list ) , class_constructor_list
ImplExpression(class_constructor_list1,
    Or(And(TypeLn(COMMA), Ln(identifier), TypeLn(LPAREN), Opt(function_argument_list), TypeLn(RPAREN), class_constructor_list1),
       Epsilon));
ImplExpression(class_constructor_list,
    And(Ln(identifier), TypeLn(LPAREN), Opt(function_argument_list), TypeLn(RPAREN),
    		class_constructor_list1));

// class_constructors
//    : class_constructor_list
ImplExpression(class_constructors,
		And(TypeLn(COLON), class_constructor_list));

// class_new_statement
//    def new ( argument_list ) class_constructors statement
//    def new ( argument_list ) statement
ImplExpression(class_new_statement,
		And(TypeLn(DEF), TypeLn(NEW), TypeLn(LPAREN), Opt(function_argument_list), TypeLn(RPAREN),
				Opt(class_constructors), statement));

// class_statement
//    class_new_statement
//    function_definition
//    field_statement
ImplExpression(class_statement,
    Ln(Or(class_new_statement, field_statement, function_definition)));

// class_statement_list
//    class_statement
//    class_statement class_statement_list
ImplExpression(class_statement_list1,
    Or(And(class_statement, class_statement_list1),
       Epsilon));
ImplExpression(class_statement_list,
    And(class_statement, class_statement_list1));

// parent_class_list
//    identifier
//    identifier , paret_class_list
ImplExpression(parent_class_list1,
		Or(And(TypeLn(COMMA), Ln(identifier), parent_class_list1),
	       Epsilon));
ImplExpression(parent_class_list,
		And(Ln(identifier), parent_class_list1));

// parent_classes
//    : parent_class_list
ImplExpression(parent_classes,
		And(TypeLn(COLON), parent_class_list));

// class_definition
//    class identifier statement
//    class identifier parent_classes statement
ImplExpression(class_definition,
      And(Type(CLASS), Ln(identifier), Opt(parent_classes),
          Or(And(TypeLn(LBRCE), Ln(class_statement_list), TypeLn(RBRCE)),
             Ln(class_statement))));

// import_statement
//    import identifier
ImplExpression(import_statement,
    Or(And(Type(IMPORT), identifier, Type(AS), identifier, Type(ENDLINE)),
       And(Type(IMPORT), identifier, Type(ENDLINE))));

// module_statement
//    module identifier
ImplExpression(module_statement, And(Type(MODULE_T), identifier, Type(ENDLINE)));

// statement
//    compound_statement
//    expression_statement
//    selection_statement
//    iteration_statement
ImplExpression(statement,
    Or(selection_statement,
       expression_statement,
       compound_statement,
       iteration_statement,
       jump_statement));

// file_level_statement
//    module_statement
//    import_statement
//    function_definition
//    statement
ImplExpression(file_level_statement,
    Or(module_statement,
       import_statement,
       function_definition,
       class_definition,
       statement));

// file_level_statement_list
//    file_level_statement
//    file_level_statement_list statement
ImplExpression(file_level_statement_list1,
    Or(And(file_level_statement, file_level_statement_list1),
       Epsilon));
ImplExpression(file_level_statement_list,
    And(file_level_statement, file_level_statement_list1));


// statement_list
//    statement
//    statement_list statement
ImplExpression(statement_list1,
    Or(And(statement, statement_list1),
       Epsilon));
ImplExpression(statement_list,
    And(statement, statement_list1));

