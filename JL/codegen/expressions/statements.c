/*
 * statements.c
 *
 *  Created on: Dec 29, 2019
 *      Author: Jeff
 */

#ifdef NEW_PARSER

#include "statements.h"

#include <limits.h>
#include <stdbool.h>

#include "../../arena/strings.h"
#include "../../error.h"
#include "../../program/tape.h"
#include "../syntax.h"
#include "../tokenizer.h"
#include "assignment.h"
#include "expression.h"
#include "expression_macros.h"

ImplPopulate(compound_statement, const SyntaxTree *stree) {
  ASSERT(IS_TOKEN(stree->first, LBRCE));
  compound_statement->expressions =
      expando(ExpressionTree *, DEFAULT_EXPANDO_SIZE);
  if (IS_TOKEN(stree->second, RBRCE)) {
    return;
  }
  ASSERT(!IS_LEAF(stree->second), IS_TOKEN(stree->second->second, RBRCE));

  if (!IS_SYNTAX(stree->second->first, statement_list)) {
    ExpressionTree *elt = populate_expression(stree->second->first);
    expando_append(compound_statement->expressions, &elt);
    return;
  }

  ExpressionTree *first = populate_expression(stree->second->first->first);
  expando_append(compound_statement->expressions, &first);
  SyntaxTree *cur = stree->second->first->second;
  while (true) {
    if (!IS_SYNTAX(cur, statement_list1)) {
      ExpressionTree *elt = populate_expression(cur);
      expando_append(compound_statement->expressions, &elt);
      break;
    }
    ExpressionTree *elt = populate_expression(cur->first);
    expando_append(compound_statement->expressions, &elt);
    cur = cur->second;
  }
}

ImplDelete(compound_statement) {
  void delete_statement(void *ptr) {
    ExpressionTree *tree = *((ExpressionTree **)ptr);
    delete_expression(tree);
  }
  expando_iterate(compound_statement->expressions, delete_statement);
  expando_delete(compound_statement->expressions);
}

ImplProduce(compound_statement, Tape *tape) {
  int num_ins = 0;
  if (expando_len(compound_statement->expressions) == 0) {
    return 0;
  }
  void produce_statements(void *ptr) {
    ExpressionTree *tree = *((ExpressionTree **)ptr);
    num_ins += produce_instructions(tree, tape);
  }
  expando_iterate(compound_statement->expressions, produce_statements);
  return num_ins;
}

ImplPopulate(try_statement, const SyntaxTree *stree) {
  ASSERT(!IS_LEAF(stree), !IS_LEAF(stree->first),
         IS_TOKEN(stree->first->first, TRY));
  try_statement->try_body = populate_expression(stree->first->second->first);

  ASSERT(!IS_LEAF(stree->first->second->second),
         IS_TOKEN(stree->first->second->second->first, CATCH));
  try_statement->catch_token = stree->first->second->second->first->token;
  // May be surrounded in parenthesis.
  const SyntaxTree *catch_assign_exp =
      IS_SYNTAX(stree->first->second->second->second, catch_assign)
          ? stree->first->second->second->second->second->first
          : stree->first->second->second->second;
  try_statement->error_assignment_lhs = populate_assignment(catch_assign_exp);
  try_statement->catch_body = populate_expression(stree->second);
}

ImplDelete(try_statement) {
  delete_expression(try_statement->try_body);
  delete_assignment(&try_statement->error_assignment_lhs);
  delete_expression(try_statement->catch_body);
}

ImplProduce(try_statement, Tape *tape) {
  int num_ins = 0;
  Tape *try_body_tape = tape_create();
  int try_ins = produce_instructions(try_statement->try_body, try_body_tape);
  Tape *catch_body_tape = tape_create();
  int catch_ins =
      produce_instructions(try_statement->catch_body, catch_body_tape);

  int goto_pos = try_ins - 1;

  num_ins +=
      tape->ins_int(tape, CTCH, goto_pos, try_statement->catch_token) + try_ins;
  tape_append(tape, try_body_tape);

  Tape *error_assign_tape = tape_create();
  int num_assign =
      produce_assignment(&try_statement->error_assignment_lhs,
                         error_assign_tape, try_statement->catch_token);

  num_ins += tape->ins_int(tape, JMP, catch_ins + num_assign,
                           try_statement->catch_token);
  // Expect error to be in resval
  num_ins += num_assign + catch_ins;
  tape_append(tape, error_assign_tape);
  tape_append(tape, catch_body_tape);
  num_ins += tape->ins_no_arg(tape, RNIL, try_statement->catch_token) +
             tape->ins_text(tape, SET, "$try_goto", try_statement->catch_token);

  tape_delete(try_body_tape);
  tape_delete(error_assign_tape);
  tape_delete(catch_body_tape);
  return num_ins;
}

ImplPopulate(raise_statement, const SyntaxTree *stree) {
  ASSERT(IS_TOKEN(stree->first, RAISE));
  raise_statement->raise_token = stree->first->token;
  raise_statement->exp = populate_expression(stree->second);
}

ImplDelete(raise_statement) { delete_expression(raise_statement->exp); }

ImplProduce(raise_statement, Tape *tape) {
  return produce_instructions(raise_statement->exp, tape) +
         tape->ins_no_arg(tape, RAIS, raise_statement->raise_token);
}

ImplPopulate(selection_statement, const SyntaxTree *stree) {
  populate_if_else(&selection_statement->if_else, stree);
}

ImplDelete(selection_statement) {
  delete_if_else(&selection_statement->if_else);
}

ImplProduce(selection_statement, Tape *tape) {
  return produce_if_else(&selection_statement->if_else, tape);
}

ImplPopulate(jump_statement, const SyntaxTree *stree) {
  if (IS_TOKEN(stree, RETURN)) {
    jump_statement->return_token = stree->token;
    jump_statement->exp = NULL;
    return;
  }
  ASSERT(IS_TOKEN(stree->first, RETURN));
  jump_statement->return_token = stree->first->token;
  if (!IS_LEAF(stree->second) && IS_TOKEN(stree->second->first, LPAREN) &&
      !IS_SYNTAX(stree->second, primary_expression)) {
    jump_statement->exp = populate_expression(stree->second->second->first);
  } else {
    jump_statement->exp = populate_expression(stree->second);
  }
}

ImplDelete(jump_statement) {
  if (NULL != jump_statement->exp) {
    delete_expression(jump_statement->exp);
  }
}

ImplProduce(jump_statement, Tape *tape) {
  int num_ins = 0;
  if (NULL != jump_statement->exp) {
    num_ins += produce_instructions(jump_statement->exp, tape);
  }
  num_ins += tape->ins_no_arg(tape, RET, jump_statement->return_token);
  return num_ins;
}

ImplPopulate(break_statement, const SyntaxTree *stree) {
  ASSERT(IS_LEAF(stree));
  break_statement->token = stree->token;

  if (IS_TOKEN(stree, BREAK)) {
    break_statement->type = Break_break;
  } else if (IS_TOKEN(stree, CONTINUE)) {
    break_statement->type = Break_continue;
  } else {
    ERROR("Unknown break_statement: %s", stree->token->text);
  }
}

ImplDelete(break_statement) {}

ImplProduce(break_statement, Tape *tape) {
  if (break_statement->type == Break_break) {
    // Signals a break.
    return tape->ins_int(tape, JMP, 0, break_statement->token);
  } else if (break_statement->type == Break_continue) {
    // Signals a continue.
    return tape->ins_int(tape, JMP, INT_MAX, break_statement->token);
  } else {
    ERROR("Unknown break_statement.");
  }
  return 0;
}

ImplPopulate(exit_statement, const SyntaxTree *stree) {
  ASSERT(IS_TOKEN(stree, EXIT_T));
  exit_statement->token = stree->token;
}

ImplDelete(exit_statement) {
  // Nop.
}

ImplProduce(exit_statement, Tape *tape) {
  return tape->ins_no_arg(tape, EXIT, exit_statement->token);
}

#endif
