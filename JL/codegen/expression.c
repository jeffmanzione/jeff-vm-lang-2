/*
 * expression.c
 *
 *  Created on: Jun 23, 2018
 *      Author: Jeff
 */

#include "expression.h"

#include "../datastructure/expando.h"
#include "../datastructure/map.h"
#include "../memory/memory.h"
#include "../program/instruction.h"
#include "syntax.h"

typedef ExpressionTree *(*Populator)(SyntaxTree *tree);
typedef int (*Producer)(ExpressionTree *tree, Tape *tape);
typedef void (*EDeleter)(ExpressionTree *tree);

Map populators;
Map producers;
Map deleters;

#define EXPECT_TYPE(stree, type) \
  if (stree->expression != type) { \
    ERROR("Expected type" #type); \
  }

#define IS_SYNTAX(var_name, type) \
   ((var_name->expression) == (type))

#define IS_EXPRESSION(var_name, test_type) \
   ((var_name->type) == (test_type))

#define DECLARE_IF_TYPE(name, type, stree) \
  SyntaxTree *name; \
  { \
    if (stree->expression != type) { \
      ERROR("Expected " #type " for " #name); \
      name = NULL; \
    } else { \
      name = stree;\
    } \
  }

#define ASSIGN_IF_TYPE(name, type, stree) \
  { \
    if (stree->expression != type) { \
      ERROR("Expected " #type " for " #name); \
    } else { \
      name = stree;\
    } \
  }

#define IF_ASSIGN(name, type, stree, statement) \
  {\
    ExpressionTree *name; \
    if ((stree)->expression == (type)) { \
      name = (stree); \
      statement \
    } \
  }

#define IS_LEAF(tree) ((tree)->token != NULL)
#define IS_TOKEN(tree, token_type) \
  IS_LEAF(tree) && ((tree)->token->type == (token_type))

ExpressionTree *__extract_tree(Expando *expando_of_tree, int index) {
  ExpressionTree **tree_ptr2 = (ExpressionTree **) expando_get(expando_of_tree,
      index);
  return *tree_ptr2;
}

#define EXTRACT_TREE(expando_of_tree, i) __extract_tree(expando_of_tree, i)

#define APPEND_TREE(expando_of_tree, stree) \
{\
  ExpressionTree *expr = populate_expression(stree);\
  expando_append(expando_of_tree, (void *) &expr);\
}

#define Register(name) \
{ \
  map_insert(&populators, name, Populate_##name); \
  map_insert(&producers, name, Produce_##name); \
  map_insert(&deleters, name, Delete_##name); \
}

struct ExpressionTree_ {
  ParseExpression type;
  union {
    Expression_identifier identifier;
    Expression_constant constant;
    Expression_string_literal string_literal;
    Expression_tuple_expression tuple_expression;
    Expression_array_declaration array_declaration;
    Expression_primary_expression primary_expression;
    Expression_postfix_expression postfix_expression;
  };
};

#define ImplPopulate(name, stree_input)            \
  ExpressionTree *Populate_##name(stree_input) {     \
    ExpressionTree *etree = ALLOC2(ExpressionTree);  \
    etree->type = name;                              \
    Transform_##name(stree, &etree->name);           \
    return etree;                                    \
  }                                                  \
  void Transform_##name(stree_input, Expression_##name *name)

#define ImplProduce(name, tape) \
  int Produce_##name(ExpressionTree *tree, Tape *t) { \
    return Produce_##name##_inner(&tree->name, t); \
  } \
  int Produce_##name##_inner(Expression_##name *name, tape)

#define ImplDelete(name) \
  void Delete_##name(ExpressionTree *tree) { \
    Delete_##name##_inner(&tree->name); \
  } \
  void Delete_##name##_inner(Expression_##name *name)

ImplPopulate(identifier, const SyntaxTree *stree) {
  identifier->id = stree->token;
}

ImplDelete(identifier) {
}

ImplProduce(identifier, Tape *tape) {
  return tape->ins(tape, RES, identifier->id);
}

ImplPopulate(constant, const SyntaxTree *stree) {
  constant->token = stree->token;
  constant->value = token_to_val(stree->token);
}

ImplDelete(constant) {
}

ImplProduce(constant, Tape *tape) {
  return tape->ins(tape, RES, constant->token);
}

ImplPopulate(string_literal, const SyntaxTree *stree) {
  string_literal->token = stree->token;
  string_literal->str = stree->token->text;
}

ImplDelete(string_literal) {
}

ImplProduce(string_literal, Tape *tape) {
  return tape->ins(tape, RES, string_literal->token);
}

ImplPopulate(tuple_expression, const SyntaxTree *stree) {
  tuple_expression->token = stree->second->first->token;  // First comma.
  tuple_expression->list = expando(ExpressionTree *, DEFAULT_EXPANDO_SIZE);
  APPEND_TREE(tuple_expression->list, stree->first);
  DECLARE_IF_TYPE(tuple1, tuple_expression1, stree->second);
  // Loop through indices > 0.
  while (true) {
    if (IS_LEAF(
        tuple1->second) || !IS_SYNTAX(tuple1->second->second, tuple_expression1)) {
      // Last element.
      APPEND_TREE(tuple_expression->list, tuple1->second);
      break;
    } else {
      APPEND_TREE(tuple_expression->list, tuple1->second->first);
      ASSIGN_IF_TYPE(tuple1, tuple_expression1, tuple1->second->second);
    }
  }
}

ImplDelete(tuple_expression) {
  void delete_expression_inner(void *elt) {
    delete_expression(*((ExpressionTree **) elt));
  }
  expando_iterate(tuple_expression->list, delete_expression_inner);
  expando_delete(tuple_expression->list);
}

int tuple_expression_helper(Expression_tuple_expression *tuple_expression,
    Tape *tape) {
  int i, num_ins = 0, tuple_len = expando_len(tuple_expression->list);
  // Start from end and go backward.
  for (i = tuple_len - 1; i >= 0; --i) {
    ExpressionTree *elt = EXTRACT_TREE(tuple_expression->list, i);
    num_ins += produce_instructions(elt, tape)
        + tape->ins_no_arg(tape, PUSH, tuple_expression->token);
  }
  return num_ins;
}

ImplProduce(tuple_expression, Tape *tape) {
  return tuple_expression_helper(tuple_expression, tape)
      + tape->ins_int(tape, TUPL, expando_len(tuple_expression->list),
          tuple_expression->token);
}

ImplPopulate(array_declaration, const SyntaxTree *stree) {
  if (IS_LEAF(stree->second)) {
    // No args.
    array_declaration->exp = NULL;
    array_declaration->is_empty = true;
    return;
  }
  array_declaration->is_empty = false;
  // Inside braces.
  array_declaration->exp = populate_expression(stree->second->first);
}

ImplDelete(array_declaration) {
  if (array_declaration->exp != NULL) {
    delete_expression(array_declaration->exp);
  }
}

ImplProduce(array_declaration, Tape *tape) {
  if (array_declaration->is_empty) {
    return tape->ins_no_arg(tape, ANEW, array_declaration->token);
  }

  int num_ins = 0, num_members = 1;
  if (IS_EXPRESSION(array_declaration->exp, tuple_expression)) {
    num_members = expando_len(array_declaration->exp->tuple_expression.list);
    num_ins += tuple_expression_helper(
        &array_declaration->exp->tuple_expression, tape);
  } else {
    num_ins += tape->ins_no_arg(tape, PUSH, array_declaration->token);
  }
  num_ins += tape->ins_int(tape, ANEW, num_members, array_declaration->token);
  return num_ins;
}

ImplPopulate(primary_expression, const SyntaxTree *stree) {
  if (IS_TOKEN(stree->first, LPAREN)) {
    primary_expression->token = stree->first->token;
    // Inside parenthesis.
    primary_expression->exp = populate_expression(stree->second->first);
    return;
  }
  ERROR("Unknown primary_expression.");
}

ImplDelete(primary_expression) {
  delete_expression(primary_expression->exp);
}

ImplProduce(primary_expression, Tape *tape) {
  return produce_instructions(primary_expression->exp, tape);
}

void postfix_period(const SyntaxTree *tail, Postfix *postfix) {
  if (IS_LEAF(tail)) {
    postfix->exp = populate_expression(tail);
    return;
  }
  // Nested function call, e.g., a.b()
  if (!IS_LEAF(tail->second) && IS_TOKEN(tail->second->first, LPAREN)) {
    Token
  }
}

void postfix_helper(const SyntaxTree *ext, const SyntaxTree *tail,
    Expando *suffixes) {
  Postfix postfix = { .type = ext->token->type };
  switch (ext->token->type) {
  case PERIOD:

    break;
  case LPAREN:

    break;
  case LBRAC:

    break;
  default:
    ERROR("Unknown postfix.");
  }
  expando_append(suffixes, &postfix);
}

ImplPopulate(postfix_expression, const SyntaxTree *stree) {
  postfix_expression->prefix = populate_expression(stree->first);
  postfix_expression->suffixes = expando(Postfix, DEFAULT_EXPANDO_SIZE);

  int num_ins = 0;
  SyntaxTree *suffix = stree->second;
  SyntaxTree *ext = suffix->first;
  SyntaxTree *tail = suffix->second;
  postfix_helper(ext, tail, postfix_expression->suffixes);
}

ImplDelete(postfix_expression) {
  delete_expression(postfix_expression->prefix);
  void delete_postfix(void *ptr) {
    Postfix *postfix = (Postfix *) ptr;
    delete_expression(postfix->exp);
  }
  expando_delete(postfix_expression->suffixes);
}

ImplProduce(postfix_expression, Tape *tape) {
  return 0;
}

void expression_init() {
  map_init_default(&populators);
  map_init_default(&producers);
  map_init_default(&deleters);

  Register(identifier);
  Register(constant);
  Register(string_literal);
  Register(tuple_expression);
  Register(array_declaration);
  Register(primary_expression);
  Register(postfix_expression);
}

void expression_finalize() {
  map_finalize(&populators);
  map_finalize(&producers);
  map_finalize(&deleters);
}

ExpressionTree *populate_expression(SyntaxTree *tree) {
  Populator populate = (Populator) map_lookup(&populators, tree->expression);
  if (NULL == populate) {
    ERROR("Populator not found.");
  }
  return populate(tree);
}

int produce_instructions(ExpressionTree *tree, Tape *tape) {
  Producer produce = (Producer) map_lookup(&producers, tree->type);
  if (NULL == produce) {
    ERROR("Producer not found.");
  }
  return produce(tree, tape);
}

void delete_expression(ExpressionTree *tree) {
  EDeleter delete = (EDeleter) map_lookup(&deleters, tree->type);
  if (NULL == delete) {
    ERROR("Deleter not found.");
  }
  delete(tree);
  DEALLOC(tree);
}
