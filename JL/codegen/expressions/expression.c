/*
 * expression.c
 *
 *  Created on: Jun 23, 2018
 *      Author: Jeff
 */

#ifdef NEW_PARSER

#include "expression.h"

#include <stddef.h>

#include "../../arena/strings.h"
#include "../../datastructure/map.h"
#include "../../error.h"
#include "../../memory/memory.h"
#include "../../program/instruction.h"
#include "../parse.h"
#include "../syntax.h"
#include "assignment.h"
#include "classes.h"
#include "files.h"
#include "loops.h"
#include "statements.h"

typedef ExpressionTree* (*Populator)(const SyntaxTree *tree);
typedef int (*Producer)(ExpressionTree *tree, Tape *tape);
typedef void (*EDeleter)(ExpressionTree *tree);

Map populators;
Map producers;
Map deleters;

ExpressionTree* __extract_tree(Expando *expando_of_tree, int index) {
  ExpressionTree **tree_ptr2 = (ExpressionTree**) expando_get(expando_of_tree,
      index);
  return *tree_ptr2;
}

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
  tuple_expression->list = expando(ExpressionTree*, DEFAULT_EXPANDO_SIZE);
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
    delete_expression(*((ExpressionTree**) elt));
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
  array_declaration->token = stree->first->token;
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

void postfix_helper(const SyntaxTree *suffix, Expando *suffixes);

void postfix_period(const SyntaxTree *ext, const SyntaxTree *tail,
    Expando *suffixes) {
  Postfix postfix = { .type = Postfix_field, .token = ext->token, .exp = NULL };
  if (IS_SYNTAX(tail, identifier) || IS_TOKEN(tail, NEW)) {
    postfix.id = tail->token;
    expando_append(suffixes, &postfix);
    return;
  } ASSERT(!IS_LEAF(tail));
  postfix.id = tail->first->token;
  expando_append(suffixes, &postfix);
  postfix_helper(tail->second, suffixes);
}

void postfix_surround_helper(const SyntaxTree *suffix, Expando *suffixes,
    PostfixType postfix_type, TokenType opener, TokenType closer) {
  Postfix postfix = { .type = postfix_type, .token = suffix->first->token };
  ASSERT(!IS_LEAF(suffix), IS_TOKEN(suffix->first, opener));
  if (IS_TOKEN(suffix->second, closer)) {
    // No args.
    postfix.exp = NULL;
    expando_append(suffixes, &postfix);
    return;
  }
  // Function call w/o args followed by postfix. E.g., a().b
  if (!IS_LEAF(suffix->second) && IS_TOKEN(suffix->second->first, closer)) {
    // No args.
    postfix.exp = NULL;
    expando_append(suffixes, &postfix);
    postfix_helper(suffix->second->second, suffixes);
    return;
  }

  SyntaxTree *fn_args = suffix->second->first;
  postfix.exp = populate_expression(fn_args);
  expando_append(suffixes, &postfix);

  if (IS_TOKEN(suffix->second->second, closer)) {
    // No additional postfix.
    return;
  }

  // Must be function call with args and followed by postfix. E.g., a(b).c
  ASSERT(IS_TOKEN(suffix->second->second->first, closer));
  postfix_helper(suffix->second->second->second, suffixes);
}

void postfix_helper(const SyntaxTree *suffix, Expando *suffixes) {
  const SyntaxTree *ext = suffix->first;
  switch (ext->token->type) {
  case PERIOD:
    postfix_period(ext, suffix->second, suffixes);
    break;
  case LPAREN:
    postfix_surround_helper(suffix, suffixes, Postfix_fncall, LPAREN, RPAREN);
    break;
  case LBRAC:
    postfix_surround_helper(suffix, suffixes, Postfix_array_index, LBRAC,
        RBRAC);
    break;
  default:
    ERROR("Unknown postfix.");
  }
}

ImplPopulate(postfix_expression, const SyntaxTree *stree) {
  postfix_expression->prefix = populate_expression(stree->first);
  postfix_expression->suffixes = expando(Postfix, DEFAULT_EXPANDO_SIZE);

//  int num_ins = 0;
  SyntaxTree *suffix = stree->second;
  postfix_helper(suffix, postfix_expression->suffixes);
}

ImplDelete(postfix_expression) {
  delete_expression(postfix_expression->prefix);
  void delete_postfix(void *ptr) {
    Postfix *postfix = (Postfix*) ptr;
    if (postfix->type != Postfix_field && NULL != postfix->exp) {
      delete_expression(postfix->exp);
    }
  }
  expando_iterate(postfix_expression->suffixes, delete_postfix);
  expando_delete(postfix_expression->suffixes);
}

int produce_postfix(int *i, int num_postfix, Expando *suffixes, Postfix **next,
    Tape *tape) {
  int num_ins = 0;
  Postfix *cur = *next;
  *next =
      (*i + 1 == num_postfix) ? NULL : (Postfix*) expando_get(suffixes, *i + 1);
  if (cur->type == Postfix_fncall) {
    num_ins += tape->ins_no_arg(tape, PUSH, cur->token);
    if (cur->exp != NULL) {
      num_ins += produce_instructions(cur->exp, tape);
    }
    num_ins += tape->ins_no_arg(tape, CALL, cur->token);
  } else if (cur->type == Postfix_array_index) {
    num_ins += tape->ins_no_arg(tape, PUSH, cur->token)
        + produce_instructions(cur->exp, tape)
        + tape->ins_no_arg(tape, AIDX, cur->token);
  } else if (cur->type == Postfix_field) {
    // Function calls on fields must be handled with CALL X.
    if (NULL != *next && (*next)->type == Postfix_fncall) {
      num_ins +=
          tape->ins_no_arg(tape, PUSH, cur->token)
              + ((*next)->exp != NULL ?
                  produce_instructions((*next)->exp, tape) : 0)
              + tape_ins(tape, CALL, cur->id);
      // Advance past the function call since we have already handled it.
      ++(*i);
      *next =
          (*i + 1 == num_postfix) ?
          NULL :
                                    (Postfix*) expando_get(suffixes, *i + 1);
    } else {
      num_ins += tape->ins(tape, GET, cur->id);
    }
  } else {
    ERROR("Unknown postfix_expression.");
  }
  return num_ins;
}

ImplProduce(postfix_expression, Tape *tape) {
  int i, num_ins = 0, num_postfix = expando_len(postfix_expression->suffixes);
  num_ins += produce_instructions(postfix_expression->prefix, tape);
  Postfix *next = (Postfix*) expando_get(postfix_expression->suffixes, 0);
  for (i = 0; i < num_postfix; ++i) {
    if (NULL == next) {
      break;
    }
    num_ins += produce_postfix(&i, num_postfix, postfix_expression->suffixes,
        &next, tape);
  }
  return num_ins;
}

ImplPopulate(range_expression, const SyntaxTree *stree) {
  range_expression->start = populate_expression(stree->first);
  ASSERT(IS_TOKEN(stree->second->first, COLON));
  range_expression->token = stree->second->first->token;
  SyntaxTree *after_first_colon = stree->second->second;
  if (!IS_LEAF(
      after_first_colon) && !IS_LEAF(
          after_first_colon->second) && IS_TOKEN(after_first_colon->second->first, COLON)) {
    // Has inc.
    range_expression->num_args = 3;
    range_expression->inc = populate_expression(after_first_colon->first);
    range_expression->end = populate_expression(
        after_first_colon->second->second);
    return;
  }
  range_expression->num_args = 2;
  range_expression->inc = NULL;
  range_expression->end = populate_expression(after_first_colon);
}

ImplDelete(range_expression) {
  delete_expression(range_expression->start);
  delete_expression(range_expression->end);
  if (NULL != range_expression->inc) {
    delete_expression(range_expression->inc);
  }
}

ImplProduce(range_expression, Tape *tape) {
  int num_ins = 0;
  num_ins += tape->ins_text(tape, PUSH, strings_intern("range"),
      range_expression->token);
  if (NULL != range_expression->inc) {
    num_ins += produce_instructions(range_expression->inc, tape)
        + tape->ins_no_arg(tape, PUSH, range_expression->token);
  }
  num_ins += produce_instructions(range_expression->end, tape)
      + tape->ins_no_arg(tape, PUSH, range_expression->token)
      + produce_instructions(range_expression->start, tape)
      + tape->ins_no_arg(tape, PUSH, range_expression->token)
      + tape->ins_int(tape, TUPL, range_expression->num_args,
          range_expression->token)
      + tape->ins_no_arg(tape, CALL, range_expression->token);
  return num_ins;
}

UnaryType unary_token_to_type(const Token *token) {
  switch (token->type) {
  case TILDE:
    return Unary_not;
  case EXCLAIM:
    return Unary_notc;
  case MINUS:
    return Unary_negate;
  case CONST_T:
    return Unary_const;
  default:
    ERROR("Unknown unary: %s", token->text);
  }
  return Unary_unknown;
}

ImplPopulate(unary_expression, const SyntaxTree *stree) {
  ASSERT(IS_LEAF(stree->first));
  unary_expression->token = stree->first->token;
  unary_expression->type = unary_token_to_type(unary_expression->token);
  unary_expression->exp = populate_expression(stree->second);
}

ImplDelete(unary_expression) {
  delete_expression(unary_expression->exp);
}

ImplProduce(unary_expression, Tape *tape) {
  int num_ins = 0;
  if (unary_expression->type == Unary_negate
      && constant == unary_expression->exp->type) {
    return tape->ins_neg(tape, RES, unary_expression->exp->constant.token);
  }
  num_ins += produce_instructions(unary_expression->exp, tape);
  switch (unary_expression->type) {
  case Unary_not:
    num_ins += tape->ins_no_arg(tape, NOT, unary_expression->token);
    break;
  case Unary_notc:
    num_ins += tape->ins_no_arg(tape, NOTC, unary_expression->token);
    break;
  case Unary_negate:
    num_ins += tape->ins_no_arg(tape, PUSH, unary_expression->token)
        + tape->ins_int(tape, PUSH, -1, unary_expression->token)
        + tape->ins_no_arg(tape, MULT, unary_expression->token);
    break;
  case Unary_const:
    num_ins += tape->ins_no_arg(tape, CNST, unary_expression->token);
    break;
  default:
    ERROR("Unknown unary: %s", unary_expression->token);
  }
  return num_ins;
}

BiType relational_type_for_token(const Token *token) {
  switch (token->type) {
  case STAR:
    return Mult_mult;
  case FSLASH:
    return Mult_div;
  case PERCENT:
    return Mult_mod;
  case PLUS:
    return Add_add;
  case MINUS:
    return Add_sub;
  case LTHAN:
    return Rel_lt;
  case GTHAN:
    return Rel_gt;
  case LTHANEQ:
    return Rel_lte;
  case GTHANEQ:
    return Rel_gte;
  case EQUIV:
    return Rel_eq;
  case NEQUIV:
    return Rel_neq;
  case AMPER:
    return And_and;
  case CARET:
    return And_xor;
  case PIPE:
    return And_or;
  default:
    ERROR("Unknown type: %s", token->text);
  }
  return BiType_unknown;
}

Op bi_to_ins(BiType type) {
  switch (type) {
  case Mult_mult:
    return MULT;
  case Mult_div:
    return DIV;
  case Mult_mod:
    return MOD;
  case Add_add:
    return ADD;
  case Add_sub:
    return SUB;
  case Rel_lt:
    return LT;
  case Rel_gt:
    return GT;
  case Rel_lte:
    return LTE;
  case Rel_gte:
    return GTE;
  case Rel_eq:
    return EQ;
  case Rel_neq:
    return NEQ;
  case And_and:
    return AND;
  case And_xor:
    return XOR;
  case And_or:
    return OR;
  default:
    ERROR("Unknown type: %s", type);
  }
  return NOP;
}

#define BiExpressionPopulate(expr, stree) { \
  expr->exp = populate_expression(stree->first); \
  Expando *suffixes = expando(BiSuffix, DEFAULT_EXPANDO_SIZE); \
  SyntaxTree *cur_suffix = stree->second; \
  while (true) { \
    EXPECT_TYPE(cur_suffix, expr##1); \
    BiSuffix suffix = { .token = cur_suffix->first->token, .type = \
        relational_type_for_token(cur_suffix->first->token) }; \
    SyntaxTree *second_exp = cur_suffix->second; \
    if (second_exp->expression == stree->expression) { \
      suffix.exp = populate_expression(second_exp->first); \
      expando_append(suffixes, &suffix); \
      cur_suffix = second_exp->second; \
    } else { \
      suffix.exp = populate_expression(second_exp); \
      expando_append(suffixes, &suffix); \
      break; \
    } \
  } \
  expr->suffixes = suffixes; \
}

#define BiExpressionDelete(expr) { \
  delete_expression(expr->exp); \
  void delete_expression_inner(void *ptr) { \
    BiSuffix *suffix = (BiSuffix*) ptr; \
    delete_expression(suffix->exp); \
  } \
  expando_iterate(expr->suffixes, delete_expression_inner); \
  expando_delete(expr->suffixes); \
}

#define BiExpressionProduce(expr, tape) { \
  int num_ins = 0; \
  num_ins += produce_instructions(expr->exp, tape); \
  void iterate_mult(void *ptr) { \
    BiSuffix *suffix = (BiSuffix*) ptr; \
    num_ins += tape->ins_no_arg(tape, PUSH, suffix->token) \
        + produce_instructions(suffix->exp, tape) \
        + tape->ins_no_arg(tape, PUSH, suffix->token) \
        + tape->ins_no_arg(tape, bi_to_ins(suffix->type), \
            suffix->token); \
  } \
  expando_iterate(expr->suffixes, iterate_mult); \
  return num_ins; \
}

#define ImplBiExpression(expr) \
  ImplPopulate(expr, const SyntaxTree *stree) \
    BiExpressionPopulate(expr, stree); \
  ImplDelete(expr) \
    BiExpressionDelete(expr); \
  ImplProduce(expr, Tape *tape) \
    BiExpressionProduce(expr, tape)

ImplBiExpression(multiplicative_expression);
ImplBiExpression(additive_expression);
ImplBiExpression(relational_expression);
ImplBiExpression(equality_expression);
ImplBiExpression(and_expression);
ImplBiExpression(xor_expression);
ImplBiExpression(or_expression);

ImplPopulate(in_expression, const SyntaxTree *stree) {
  in_expression->element = populate_expression(stree->first);
  in_expression->collection = populate_expression(stree->second->second);
  in_expression->token = stree->second->first->token;
  in_expression->is_not = in_expression->token->type == NOTIN ? true : false;
}

ImplDelete(in_expression) {
  delete_expression(in_expression->element);
  delete_expression(in_expression->collection);
}

ImplProduce(in_expression, Tape *tape) {
  return produce_instructions(in_expression->collection, tape)
      + tape->ins_no_arg(tape, PUSH, in_expression->token)
      + produce_instructions(in_expression->element, tape)
      + tape->ins_text(tape, CALL, IN_FN_NAME, in_expression->token)
      + (in_expression->is_not ?
          tape->ins_no_arg(tape, NOT, in_expression->token) : 0);
}

ImplPopulate(is_expression, const SyntaxTree *stree) {
  is_expression->exp = populate_expression(stree->first);
  is_expression->type = populate_expression(stree->second->second);
  is_expression->token = stree->second->first->token;
}

ImplDelete(is_expression) {
  delete_expression(is_expression->exp);
  delete_expression(is_expression->type);
}

ImplProduce(is_expression, Tape *tape) {
  return produce_instructions(is_expression->exp, tape)
      + tape->ins_no_arg(tape, PUSH, is_expression->token)
      + produce_instructions(is_expression->type, tape)
      + tape->ins_no_arg(tape, PUSH, is_expression->token)
      + tape->ins_no_arg(tape, IS, is_expression->token);
}

void populate_if_else(IfElse *if_else, const SyntaxTree *stree) {
  if_else->conditions = expando(Conditional, DEFAULT_EXPANDO_SIZE);
  if_else->else_exp = NULL;
  ASSERT(stree->first->token->type == IF_T);
  SyntaxTree *if_tree = (SyntaxTree*) stree, *else_body = NULL;
  while (true) {
    Conditional cond = { .condition = populate_expression(
        if_tree->second->first), .if_token = if_tree->first->token };
    SyntaxTree *if_body =
    IS_TOKEN(if_tree->second->second->first, THEN) ?
        if_tree->second->second->second : if_tree->second->second;
    // Handles else statements.
    if (!IS_LEAF(
        if_body) && !IS_LEAF(if_body->second) && IS_TOKEN(if_body->second->first, ELSE)) {
      else_body = if_body->second->second;
      if_body = if_body->first;
    } else {
      else_body = NULL;
    }
    cond.body = populate_expression(if_body);
    expando_append(if_else->conditions, &cond);

    // Is this the final else?
    if (NULL != else_body && !IS_SYNTAX(else_body, stree->expression)) {
      if_else->else_exp = populate_expression(else_body);
      break;
    } else if (NULL == else_body) {
      break;
    }
    if_tree = else_body;
  }
}

ImplPopulate(conditional_expression, const SyntaxTree *stree) {
  populate_if_else(&conditional_expression->if_else, stree);
}

void delete_if_else(IfElse *if_else) {
  void delete_conditional(void *ptr) {
    Conditional *cond = (Conditional*) ptr;
    delete_expression(cond->condition);
    delete_expression(cond->body);
  }
  expando_iterate(if_else->conditions, delete_conditional);
  expando_delete(if_else->conditions);
  if (NULL != if_else->else_exp) {
    delete_expression(if_else->else_exp);
  }
}

ImplDelete(conditional_expression) {
  delete_if_else(&conditional_expression->if_else);
}

int produce_if_else(IfElse *if_else, Tape *tape) {
  int i, num_ins = 0, num_conds = expando_len(if_else->conditions),
      num_cond_ins = 0, num_body_ins = 0;

  Expando *conds = expando(Tape*, DEFAULT_EXPANDO_SIZE);
  Expando *bodies = expando(Tape*, DEFAULT_EXPANDO_SIZE);
  for (i = 0; i < num_conds; ++i) {
    Conditional *cond = (Conditional*) expando_get(if_else->conditions, i);
    Tape *condition = tape_create();
    Tape *body = tape_create();
    num_cond_ins += produce_instructions(cond->condition, condition);
    num_body_ins += produce_instructions(cond->body, body);
    expando_append(conds, &condition);
    expando_append(bodies, &body);
  }

  int num_else_ins = 0;
  Tape *else_body = NULL;
  if (NULL != if_else->else_exp) {
    else_body = tape_create();
    num_else_ins += produce_instructions(if_else->else_exp, else_body);
  } else {
    // Compensate for missing last jmp.
    num_else_ins -= 1;
  }

  num_ins = num_cond_ins + num_body_ins + (2 * num_conds) + num_else_ins;

  int num_body_jump = num_body_ins;
  // Iterate and write all conditions forward.
  for (i = 0; i < expando_len(if_else->conditions); ++i) {
    Conditional *cond = (Conditional*) expando_get(if_else->conditions, i);
    Tape *condition = *((Tape**) expando_get(conds, i));
    Tape *body = *((Tape**) expando_get(bodies, i));

    num_cond_ins -= tape_len(condition);
    num_body_jump -= tape_len(body);

    tape_append(tape, condition);
    if (i == num_conds - 1) {
      tape->ins_int(tape, IFN,
          num_body_ins + num_conds + (NULL == if_else->else_exp ? -1 : 0),
          cond->if_token);
    } else {
      tape->ins_int(tape, IF,
          num_cond_ins + num_body_jump + 2 * (num_conds - i - 1),
          cond->if_token);
    }
    tape_delete(condition);
  }
  // Iterate and write all bodies backward.
  for (i = num_conds - 1; i >= 0; --i) {
    Conditional *cond = (Conditional*) expando_get(if_else->conditions, i);
    Tape *body = *((Tape**) expando_get(bodies, i));
    num_body_ins -= tape_len(body);
    tape_append(tape, body);
    if (i > 0 || NULL != if_else->else_exp) {
      tape->ins_int(tape, JMP, num_else_ins + num_body_ins + i, cond->if_token);
    }
    tape_delete(body);
  }
  // Add else if there is one.
  if (NULL != else_body) {
    tape_append(tape, else_body);
    tape_delete(else_body);
  }
  expando_delete(conds);
  expando_delete(bodies);
  return num_ins;
}

ImplProduce(conditional_expression, Tape *tape) {
  return produce_if_else(&conditional_expression->if_else, tape);
}

void set_anon_function_def(const SyntaxTree *fn_identifier, Function *func) {
  func->def_token = fn_identifier->token;
  func->fn_name = NULL;
}

Function populate_anon_function(const SyntaxTree *stree) {
  return populate_function_variant(stree, anon_function_definition,
      anon_signature_const, anon_signature_nonconst, anon_identifier,
      set_anon_function_def);
}

ImplPopulate(anon_function_definition, const SyntaxTree *stree) {
  anon_function_definition->func = populate_anon_function(stree);
}

ImplDelete(anon_function_definition) {
  delete_function(&anon_function_definition->func);
}

int produce_anon_function(Function *func, Tape *tape) {
  int num_ins = 0, func_ins = 0;
  Tape *tmp = tape_create();
  if (func->has_args) {
    func_ins += produce_arguments(&func->args, tmp);
  }
  func_ins += produce_instructions(func->body, tmp);
  if (func->is_const) {
    func_ins += tmp->ins_no_arg(tmp, CNST, func->const_token);
  }
  func_ins += tmp->ins_no_arg(tmp, RET, func->def_token);

  num_ins += tape->ins_int(tape, JMP, func_ins, func->def_token)
      + tape->anon_label(tape, func->def_token);
  tape_append(tape, tmp);
  tape_delete(tmp);
  num_ins += func_ins + tape->ins_text(tape, RES, SELF, func->def_token)
      + tape->ins_anon(tape, GET, func->def_token);

  return num_ins;
}

ImplProduce(anon_function_definition, Tape *tape) {
  return produce_anon_function(&anon_function_definition->func, tape);
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
  Register(range_expression);
  Register(unary_expression);
  Register(multiplicative_expression);
  Register(additive_expression);
  Register(relational_expression);
  Register(equality_expression);
  Register(and_expression);
  Register(xor_expression);
  Register(or_expression);
  Register(in_expression);
  Register(is_expression);
  Register(conditional_expression);
  Register(anon_function_definition);
  Register(assignment_expression);

  Register(foreach_statement);
  Register(for_statement);
  Register(while_statement);

  Register(compound_statement);
  Register(try_statement);
  Register(raise_statement);
  Register(selection_statement);
  Register(jump_statement);
  Register(break_statement);
  Register(exit_statement);

  Register(module_statement);
  Register(import_statement);
  Register(function_definition);

  Register(class_definition);
  Register(file_level_statement_list);
}

void expression_finalize() {
  map_finalize(&populators);
  map_finalize(&producers);
  map_finalize(&deleters);
}

ExpressionTree* populate_expression(const SyntaxTree *tree) {
  Populator populate = (Populator) map_lookup(&populators, tree->expression);
  if (NULL == populate) {
    ERROR("Populator not found: %s",
        map_lookup(&parse_expressions, tree->expression));
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
    ERROR("Deleter not found: %s", map_lookup(&parse_expressions, tree->type));
  }
  delete(tree);
  DEALLOC(tree);
}

#endif
