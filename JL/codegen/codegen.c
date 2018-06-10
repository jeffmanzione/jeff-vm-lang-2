/*
 * codegen.c
 *
 *  Created on: Jan 22, 2017
 *      Author: Jeff
 */

#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "../arena/strings.h"
#include "../error.h"
#include "../instruction.h"
#include "../tape.h"
#include "expression.h"
#include "tokenizer.h"

int codegen(ExpressionTree *tree, Tape *tape);

bool is_leaf(ExpressionTree *exp_tree) {
  ASSERT_NOT_NULL(exp_tree);
  return exp_tree->token != NULL && exp_tree->first == NULL
      && exp_tree->second == NULL;
}

Op token_to_op(Token *tok) {
  ASSERT_NOT_NULL(tok);
  switch (tok->type) {
  case PLUS:
    return ADD;
  case MINUS:
    return SUB;
  case STAR:
    return MULT;
  case FSLASH:
    return DIV;
  case PERCENT:
    return MOD;
  case LTHAN:
    return LT;
  case GTHAN:
    return GT;
  case LTHANEQ:
    return LTE;
  case GTHANEQ:
    return GTE;
  case EQUIV:
    return EQ;
  case NEQUIV:
    return NEQ;
  case AMPER:
    return AND;
  case CARET:
    return XOR;
  case PIPE:
    return OR;
  case IS_T:
    return IS;
  default:
    ERROR("Unknown op.");
  }
  return NOP;
}

void append(FILE *head, FILE *tail) {
  char buf[BUFSIZ];
  size_t n;

  rewind(tail);
  while ((n = fread(buf, 1, sizeof buf, tail)) > 0) {
    if (fwrite(buf, 1, n, head) != n) {
      ERROR("FAILED TO TRANSFER CONTENTS");
    }
  }

  if (ferror(tail)) {
    ERROR("FERRROR ON TAIL");
  }
}

int codegen_if(ExpressionTree *tree, Tape *tape) {
  //DEBUGF("codegen_if");
  int num_lines = 0;
  ASSERT(!is_leaf(tree->second), !is_leaf(tree->second->second));
  bool has_then = is_leaf(tree->second->second->first)
      && tree->second->second->first->token->type == THEN;
  ExpressionTree *child_expressions =
      has_then ? tree->second->second->second : tree->second->second;

  bool has_else = !is_leaf(child_expressions)
      && !is_leaf(child_expressions->second)
      && is_leaf(child_expressions->second->first)
      && child_expressions->second->first->token->type == ELSE;
  ExpressionTree *if_case =
      has_else ? child_expressions->first : child_expressions;
  int lines_for_condition = codegen(tree->second->first, tape);
  num_lines += lines_for_condition;
  Tape *tmp_tape = tape_create();

  if (has_else) {
    ExpressionTree *else_case = child_expressions->second->second;
    int lines_for_else = codegen(else_case, tmp_tape);
    num_lines += lines_for_else
        + tape_ins_int(tape, IF, lines_for_else + 1 /* for the jmp to end */,
            child_expressions->second->first->token);
    tape_append(tape, tmp_tape);
    tape_delete(tmp_tape);

    tmp_tape = tape_create();
    int lines_for_if = codegen(if_case, tmp_tape);
    num_lines += lines_for_if
        + tape_ins_int(tape, JMP, lines_for_if,
            child_expressions->second->first->token);

    tape_append(tape, tmp_tape);
    tape_delete(tmp_tape);
  } else {
    int lines_for_if = codegen(if_case, tmp_tape);
    num_lines += lines_for_if
        + tape_ins_int(tape, IFN, lines_for_if, tree->first->token);
    tape_append(tape, tmp_tape);
    tape_delete(tmp_tape);
  }
  return num_lines;
}

int codegen_while(ExpressionTree *tree, Token *while_token, Tape *tape) {
  //DEBUGF("codegen_while");
  int num_lines = 0;
  ExpressionTree *condition_expression = tree->first;
  ExpressionTree *body = tree->second;
  num_lines += codegen(condition_expression, tape);

  Tape *tmp_tape = tape_create();
  int lines_for_body = codegen(body, tmp_tape);
  int i;
  for (i = 0; i < tape_len(tmp_tape); ++i) {
    InsContainer *c = tape_get_mutable(tmp_tape, i);
    if (c->ins.op != JMP) {
      continue;
    }
    if (c->ins.val.int_val == 0) {
      c->ins.val.int_val = lines_for_body - i + 1;
    } else if (c->ins.val.int_val == INT_MAX) {
      c->ins.val.int_val = -(i + 1);
    }
  }

  num_lines += lines_for_body
      + tape_ins_int(tape, IFN, lines_for_body + 1, while_token);
  tape_append(tape, tmp_tape);
  tape_delete(tmp_tape);

  num_lines += tape_ins_int(tape, JMP, -(num_lines + 1), while_token);

  return num_lines;
}

int codegen_for(ExpressionTree *tree, Token *for_token, Tape *tape) {
  //DEBUGF("codegen_for");
  int num_lines = 0;
  ExpressionTree *condition_expression = tree->first;
  ExpressionTree *body = tree->second;
  if (is_leaf(condition_expression->first)
      && LPAREN == condition_expression->first->token->type) {
    condition_expression = condition_expression->second->first;
  }
  ExpressionTree *init = condition_expression->first;
  ExpressionTree *condition = condition_expression->second->second->first;
  ExpressionTree *incremental =
      condition_expression->second->second->second->second;
  num_lines += codegen(init, tape);
  int lines_for_condition = codegen(condition, tape);
  num_lines += lines_for_condition;

  Tape *tmp_tape = tape_create();
  int lines_for_body = codegen(body, tmp_tape);
  int lines_for_incremental = codegen(incremental, tmp_tape);

  int i;
  for (i = 0; i < tape_len(tmp_tape); ++i) {
    InsContainer *c = tape_get_mutable(tmp_tape, i);
    if (c->ins.op != JMP) {
      continue;
    }
    if (c->ins.val.int_val == 0) {
      c->ins.val.int_val = lines_for_body + lines_for_incremental - i + 1;
    } else if (c->ins.val.int_val == INT_MAX) {
      c->ins.val.int_val = -(num_lines + i + 2);
    }
  }

  num_lines += lines_for_body + lines_for_incremental
      + tape_ins_int(tape, IFN, lines_for_body + lines_for_incremental + 1,
          for_token);
  tape_append(tape, tmp_tape);
  tape_delete(tmp_tape);

  num_lines += tape_ins_int(tape, JMP,
      -(lines_for_body + lines_for_condition + lines_for_incremental + 1),
      for_token);

  return num_lines;
}

int codegen_tuple1(ExpressionTree *tree, int *argument_count, Tape *tape) {
  //DEBUGF("codegen_tuple");
  int num_lines = 0;
  ParseExpression prod = tree->expression;
  if (prod != tuple_expression1) {
    ERROR("Not a tuple_expression1");
  }
  if (tree->second->second == NULL
      || tree->second->second->expression != tuple_expression1) {
    (*argument_count)++;
    num_lines += codegen(tree->second, tape)
        + tape_ins_no_arg(tape, PUSH, tree->first->token);
  } else {
    num_lines += codegen_tuple1(tree->second->second, argument_count, tape);
    (*argument_count)++;
    num_lines += codegen(tree->second->first, tape)
        + tape_ins_no_arg(tape, PUSH, tree->first->token);
  }
  return num_lines;
}

int codegen_tuple(ExpressionTree *tree, int *num_args, Tape *tape) {
  int num_lines = 0;
  if (tree->expression != tuple_expression) {
    ERROR("Not a tuple_expression");
  }
  num_lines += codegen_tuple1(tree->second, num_args, tape)
      + codegen(tree->first, tape)
      + tape_ins_no_arg(tape, PUSH, tree->second->first->token);
  return num_lines;
}

int codegen_function_args(ExpressionTree *tree, int argument_index, Tape *tape) {
  //DEBUGF("codegen_function_args");
  int num_lines = 0;
  ParseExpression prod = tree->expression;
  if (prod != function_argument_list1) {
    ERROR("Not a function_argument_list1");
  }
  if (is_leaf(tree->second)
      || tree->second->second->expression != function_argument_list1) {
    Token *tok =
        is_leaf(tree->second) ?
            tree->second->token : tree->second->first->token;
    num_lines += tape_ins_no_arg(tape, RES, tok)
        + tape_ins_int(tape, TGET, argument_index, tok)
        + tape_ins(tape, SET, tok);
  } else {
    num_lines += tape_ins_no_arg(tape, RES, tree->second->first->token)
        + tape_ins_no_arg(tape, PUSH, tree->second->first->token)
        + tape_ins_int(tape, TGET, argument_index, tree->second->first->token)
        + tape_ins(tape, SET, tree->second->first->token)
        + codegen_function_args(tree->second->second, argument_index + 1, tape);
  }
  return num_lines;
}

int codegen_function_arguments_list(ExpressionTree*tree, Tape* tape) {
  return tape_ins_no_arg(tape, PUSH, tree->first->token)
      + tape_ins_int(tape, TGET, 0, tree->first->token)
      + tape_ins(tape, SET, tree->first->token)
      + codegen_function_args(tree->second, 1, tape);
}

int codegen_class(ExpressionTree *tree, Tape *tape) {
  //DEBUGF("codegen_class");
  int num_lines = 0;
  ExpressionTree *class_inner = tree->second;
  ExpressionTree *class_body = class_inner->second;
  Token *class_name = class_inner->first->token;
  num_lines += tape_class(tape, class_name);
  // Iters through all parent classes
  if (!is_leaf(class_body->first)) {
    ExpressionTree *parent_class_iter = class_body->first->second;
    // When there are parent classes, the class body will actually be here.
    class_body = class_body->second;
    while (!is_leaf(parent_class_iter)) {
      ExpressionTree *parent_class = parent_class_iter->first;
      // TODO: do something with parent_class
      if (is_leaf(parent_class)) {
        break;
      }
      parent_class_iter = parent_class_iter->second->second;
    }
    // TODO: do something with parent_class_iter
  }

  if (is_leaf(class_body->first) && class_body->first->token->type == LBRCE) {
    num_lines += codegen(class_body->second->first, tape);
  } else {
    num_lines += codegen(class_body, tape);
  }
  num_lines += tape_endclass(tape, class_name);
  return num_lines;
}

int codegen_import(ExpressionTree *tree, Tape *tape) {
  //DEBUGF("codegen_import");
  if (!is_leaf(tree->first) || tree->first->token->type != IMPORT) {
    ERROR("Unknown import expression");
  }
  Token *module_name, *var_name;
  if (is_leaf(tree->second)) {
    module_name = var_name = tree->second->token;
  } else {
    module_name = tree->second->first->token;
    var_name = tree->second->second->second->token;
  }
  return tape_ins(tape, RMDL, module_name) + tape_ins(tape, MDST, var_name);
}

int codegen_function_params(ExpressionTree *arg_begin,
    ExpressionTree **function_body, Tape *tape) {
  int lines_for_func = 0;
  bool no_args = is_leaf(arg_begin->first)
      && arg_begin->first->token->type == RPAREN;
  if (no_args) {
    *function_body = arg_begin->second;
  } else if (arg_begin->first->expression != function_argument_list) {
    lines_for_func += tape_ins(tape, SET, arg_begin->first->token);
    if (is_leaf(arg_begin->second)) {
      *function_body = NULL;
    }
    *function_body = arg_begin->second->second;
  } else {
    lines_for_func += codegen(arg_begin->first, tape);
    if (is_leaf(arg_begin->second)) {
      *function_body = NULL;
    }
    *function_body = arg_begin->second->second;
  }
  if (!is_leaf(*function_body) && !is_leaf((*function_body)->first)
      && (*function_body)->first->expression == class_constructors) {
    lines_for_func += codegen((*function_body)->first, tape);
    *function_body = (*function_body)->second;
  }
  return lines_for_func;
}

int codegen_function_helper(ExpressionTree *arg_begin, const Token *token,
    Tape* tape) {
  ExpressionTree *function_body = NULL;
  int lines_for_func = codegen_function_params(arg_begin, &function_body, tape);

  lines_for_func += codegen(function_body, tape);
  return lines_for_func;
}

int codegen_class_constructors(ExpressionTree *tree, Tape *tape) {
  int lines_for_func = 0;
  ExpressionTree *constructor_list = tree->second;
  while (true) {
    ExpressionTree *class_name = constructor_list->first;
//		DEBUGF("class_name=%s", class_name->token->text);
    ExpressionTree *args = constructor_list->second->second;
    if (is_leaf(args)) {
      ASSERT(RPAREN == args->token->type);
      break;
    } else if (is_leaf(args->first)) {
      // special case where Class() is in list not at the end.
      if (RPAREN == args->first->token->type) {
        constructor_list = args->second->second;
        continue;
      }
      ASSERT(args->first->expression == identifier);
      ExpressionTree *identifier = args->first;
//			DEBUGF("identifier=%s", identifier->token->text);
    } else {
      ASSERT(args->first->expression == function_argument_list);
      Tape *tmp = tape_create();
      /* lines_for_func += */codegen_function_arguments_list(args->first, tmp);
      tape_delete(tmp);
    }
    if (is_leaf(args->second)) {
      break;
    }
//		ASSERT(args->second->expression == class_constructor_list1);
    constructor_list = args->second->second->second;
  }
  return lines_for_func;
}

int codegen_anon_function(ExpressionTree *tree, Tape *tape) {
//DEBUGF("codegen_anon_function");
  Tape *tmp_tape = tape_create();
  Token *token = tree->first->token;
  int num_lines = tape_anon_label(tmp_tape, token);
  int lines_for_func = 0;
  ExpressionTree *arg_begin = tree->second->second;

  lines_for_func += codegen_function_helper(arg_begin, token, tmp_tape);
  lines_for_func += tape_ins_no_arg(tmp_tape, RET, token);

  num_lines += tape_ins_int(tape, JMP, lines_for_func, token);
  tape_append(tape, tmp_tape);
  tape_delete(tmp_tape);
  num_lines += lines_for_func + tape_ins_anon(tape, RES, token);

  return num_lines;
}

int codegen_function(ExpressionTree *tree, Tape *tape) {
//DEBUGF("codegen_function");
  Tape *tmp_tape = tape_create();

  ExpressionTree *arg_begin = tree->second->second->second;
  Token *token = tree->second->first->token;
  int num_lines = tape_label(tmp_tape, token);
  int lines_for_func = 0;

  lines_for_func += codegen_function_helper(arg_begin, token, tmp_tape);
  lines_for_func += tape_ins_no_arg(tmp_tape, RET, token);

  num_lines += tape_ins_int(tape, JMP, lines_for_func, token);
  tape_append(tape, tmp_tape);
  tape_delete(tmp_tape);
  num_lines += lines_for_func;

  return num_lines;
}

int codegen_new(ExpressionTree *tree, Tape *tape) {
//DEBUGF("codegen_function");
  Tape *tmp_tape = tape_create();

  ExpressionTree *arg_begin = tree->second->second->second;
  Token *token = tree->second->first->token;
  int num_lines = tape_label(tmp_tape, token);
  int lines_for_func = 0;

  lines_for_func += codegen_function_helper(arg_begin, token, tmp_tape);
  lines_for_func += tape_ins_text(tmp_tape, RES, THIS, token);
  lines_for_func += tape_ins_no_arg(tmp_tape, RET, token);

  num_lines += tape_ins_int(tape, JMP, lines_for_func, token);
  tape_append(tape, tmp_tape);
  tape_delete(tmp_tape);
  num_lines += lines_for_func;

  return num_lines;
}

int codegen_postfix1(ExpressionTree *tree, Tape *tape) {
//DEBUGF("codegen_postfix1");
  int num_lines = 0;
  ParseExpression prod = tree->expression;
  if (prod != postfix_expression1) {
    ERROR("Was not a postfix_expression1");
  }
  if (is_leaf(tree)) {
    ERROR("Improperly handled postfix. Did not expect leaf");
  }
  ExpressionTree *ext = tree->first;
  ExpressionTree *tail = tree->second;
  switch (ext->token->type) {
  case (PERIOD):
    if (is_leaf(tail)) {
      num_lines += tape_ins(tape, GET, tail->token);
    } else {
      if (!is_leaf(tail->second) && is_leaf(tail->second->first)
          && tail->second->first->token->type == LPAREN) {
        num_lines += tape_ins_no_arg(tape, PUSH, tail->second->first->token);
        if (!is_leaf(tail->second->second)
            && is_leaf(tail->second->second->first)
            && tail->second->second->first->token->type == RPAREN) {
          num_lines += tape_ins(tape, CALL, tail->first->token);
          num_lines += codegen(tail->second->second->second, tape);
        } else {
          if (!is_leaf(tail->second->second)) {
            num_lines += codegen(tail->second->second->first, tape);
          }
          num_lines += tape_ins(tape, CALL, tail->first->token);
          if (!is_leaf(tail->second->second)
              && !is_leaf(tail->second->second->second)) {
            num_lines += codegen(tail->second->second->second->second, tape);
          }
        }
      } else {
        num_lines += tape_ins(tape, GET, tail->first->token);
        num_lines += codegen(tail->second, tape);
      }
    }
    break;
  case (LPAREN):
    num_lines += tape_ins_no_arg(tape, PUSH, ext->token);
    if (!is_leaf(tail) && is_leaf(tail->first)
        && tail->first->token->type == RPAREN) {
      num_lines += tape_ins_no_arg(tape, CALL, ext->token);
      num_lines += codegen(tail->second, tape);
    } else {
      if (!is_leaf(tail)) {
        num_lines += codegen(tail->first, tape);
      }
      num_lines += tape_ins_no_arg(tape, CALL, ext->token);
      if (!is_leaf(tail) && !is_leaf(tail->second)) {
        num_lines += codegen(tail->second->second, tape);
      }
    }
    break;
  case (LBRAC):
    num_lines += tape_ins_no_arg(tape, PUSH, ext->token);
    if (tail->token == NULL || tail->token->type != RBRAC) {
      num_lines += codegen(tail->first, tape);
    }
    num_lines += tape_ins_no_arg(tape, AIDX, ext->token);

    if (NULL != tail->second && !is_leaf(tail->second)) {
      num_lines += codegen(tail->second->second, tape);
    }
    break;
  default:
    ERROR("Unknown postfix.");
  }
  return num_lines;
}

int codegen_jump(ExpressionTree *tree, Tape *tape) {
//DEBUGF("codegen_jump");
  ParseExpression prod = tree->expression;
  if (prod != jump_statement) {
    ERROR("Was not a jump_statement");
  }
  if (is_leaf(tree)) {
    if (tree->token->type != RETURN) {
      ERROR("jump_statement not implemented");
    }
    return tape_ins_no_arg(tape, RET, tree->token);
  }

  ExpressionTree *jump_type = tree->first;
  ExpressionTree *jump_value = tree->second;

  if (jump_type->token->type != RETURN) {
    ERROR("jump_statement not implemented");
  }
  return codegen(jump_value, tape)
      + tape_ins_no_arg(tape, RET, jump_type->token);
}

int codegen_break(ExpressionTree *tree, Tape *tape) {
  ParseExpression prod = tree->expression;
  if (prod != break_statement) {
    ERROR("Was not a break_statement");
  }
  if (BREAK == tree->token->type) {
    return tape_ins_int(tape, JMP, 0, tree->token);
  } else if (CONTINUE == tree->token->type) {
    return tape_ins_int(tape, JMP, INT_MAX, tree->token);
  } else {
    ERROR("Unknown break statement.");
    return 0;
  }
}

int codegen_assignment_ext(ExpressionTree *tree, Tape *tape) {
//DEBUGF("codegen_assignment_ext");
  ParseExpression prod = tree->expression;
  if (prod != postfix_expression1) {
    ERROR("Was not a postfix_expression1");
  }
  ExpressionTree *ext = tree->first;
  ExpressionTree *tail = tree->second;

  int num_lines = 0;

  switch (ext->token->type) {
  case PERIOD:
    if (is_leaf(tail)) {
      num_lines += tape_ins(tape, FLD, tail->token);
    } else {
      if (!is_leaf(tail->second)
          && tail->second->first->token->type == LPAREN) {
        num_lines += tape_ins_no_arg(tape, PUSH, tail->second->first->token);
        if (!is_leaf(tail->second->second)) {
          num_lines += codegen(tail->second->second->first, tape);
        }
        num_lines += tape_ins(tape, CALL, tail->first->token);
      } else {
        num_lines += tape_ins(tape, GET, tail->first->token)
            + codegen_assignment_ext(tail->second, tape);
      }
    }
    break;
  case LPAREN:
    num_lines += tape_ins_no_arg(tape, PUSH, ext->token);
    if (!is_leaf(tail)) {
      num_lines += codegen(tail->first, tape);
    }
    num_lines += tape_ins_no_arg(tape, CALL, ext->token);

    if (is_leaf(tail)) {
      break;
    }

    if (is_leaf(tail->second)) {
      ERROR("NOT ALLOWED");
    } else {
      num_lines += codegen_assignment_ext(tail->second->second, tape);
    }
    break;
  case LBRAC:
    num_lines += tape_ins_no_arg(tape, PUSH, ext->token);
    if (is_leaf(tail)) {
      ERROR("NOT ALLOWED");
    }
    num_lines += codegen(tail->first, tape);

    if (is_leaf(tail->second)) {
      num_lines += tape_ins_no_arg(tape, ASET, tail->second->token);
    } else {
      num_lines += (tape_ins_no_arg(tape, AIDX, ext->token)
          + codegen_assignment_ext(tail->second->second, tape));
    }
    break;
  default:
    ERROR("Unknown postfix.");
  }
  return num_lines;
}

int codegen_assignment(ExpressionTree *lhs, Token *eq_sign, ExpressionTree* rhs,
    Tape *tape) {
  ParseExpression prod = lhs->expression;
  int num_lines = codegen(rhs, tape);
  if (is_leaf(lhs)) {
    num_lines += tape_ins(tape, SET, lhs->token);
  } else if (prod == postfix_expression) {
    num_lines +=
        (tape_ins_no_arg(tape, PUSH, eq_sign) + codegen(lhs->first, tape)
            + codegen_assignment_ext(lhs->second, tape));
  } else if (prod == assignment_tuple) {
    num_lines += codegen(lhs->second->first, tape);
  } else {
    ERROR("Unknown lhs assignment.");
  }
  return num_lines;
}

int codegen(ExpressionTree *tree, Tape *tape) {
//DEBUGF("codegen_");
  int num_lines = 0;
  ParseExpression prod = tree->expression;
  if (prod == identifier || prod == string_literal || prod == constant) {
    num_lines += tape_ins(tape, RES, tree->token);
  } else if (prod == array_declaration) {
    if (!is_leaf(tree->second)) {
      if (tree->second->first->expression == tuple_expression) {
        int num_args = 1;
        num_lines += codegen_tuple(tree->second->first, &num_args, tape)
            + tape_ins_int(tape, ANEW, num_args, tree->first->token);
      } else {
        num_lines += codegen(tree->second->first, tape)
            + tape_ins_no_arg(tape, PUSH, tree->first->token)
            + tape_ins_int(tape, ANEW, 1, tree->first->token);
      }
    } else {
      num_lines += tape_ins_no_arg(tape, ANEW, tree->second->token);
    }
  } else if (prod == length_expression) {
    if (!is_leaf(tree->first) || is_leaf(tree->second)
        || !is_leaf(tree->second->second)) {
      ERROR("Invalid length_expression");
    }
    num_lines += codegen(tree->second->first, tape)
        + tape_ins_text(tape, GET, LENGTH_KEY, tree->first->token);
  } else if (prod == primary_expression
      || prod == primary_expression_no_constants) {
    if (is_leaf(tree->first) && tree->first->token->type == LPAREN) {
      num_lines += codegen(tree->second->first, tape);
    } else {
      expression_tree_to_str(*tree, NULL, stdout);
      ERROR("Unknown primary_expression");
    }
  } else if (prod == additive_expression || prod == multiplicative_expression
      || prod == relational_expression || prod == equality_expression
      || prod == and_expression || prod == xor_expression
      || prod == or_expression || prod == is_expression) {
    Token *token;
    if (!is_leaf(tree->second) && !is_leaf(tree->second->second)
        && !is_leaf(tree->second->second->second)
        && is_leaf(tree->second->second->second->first)) {
      ExpressionTree *second = tree->second->second;
      ExpressionTree *op = second->second->first;
      token = op->token;
      switch (op->token->type) {
      case PIPE:
        second->expression = or_expression;
        break;
      case AMPER:
        second->expression = and_expression;
        break;
      case CARET:
        second->expression = xor_expression;
        break;
      case PLUS:
      case MINUS:
        second->expression = additive_expression;
        break;
      case STAR:
      case FSLASH:
        second->expression = multiplicative_expression;
        break;
      case EQUIV:
      case NEQUIV:
        second->expression = equality_expression;
        break;
      default:
        break;
      }
    } else {
      token = tree->second->first->token;
    }
    num_lines += codegen(tree->first, tape) + tape_ins_no_arg(tape, PUSH, token)
        + codegen(tree->second->second, tape)
        + tape_ins_no_arg(tape, PUSH, token)
        + tape_ins_no_arg(tape, token_to_op(tree->second->first->token), token);
  } else if (prod == unary_expression) {
    if (!is_leaf(tree->first)) {
      ERROR("Unary is not a leaf");
    }
    num_lines += codegen(tree->second, tape);
    switch (tree->first->token->type) {
    case TILDE:
      num_lines += tape_ins_no_arg(tape, NOT, tree->first->token);
      break;
    case EXCLAIM:
      num_lines += tape_ins_no_arg(tape, NOTC, tree->first->token);
      break;
    default:
      ERROR("Unknown unary");
    }
  } else if (prod == assignment_expression) {
    num_lines += codegen_assignment(tree->first, tree->second->first->token,
        tree->second->second, tape);
  } else if (prod == conditional_expression || prod == selection_statement) {
    num_lines += codegen_if(tree, tape);
  } else if (prod == iteration_statement) {
    switch (tree->first->token->type) {
    case WHILE:
      num_lines += codegen_while(tree->second, tree->first->token, tape);
      break;
    case FOR:
      num_lines += codegen_for(tree->second, tree->first->token, tape);
      break;
    default:
      expression_tree_to_str(*tree, NULL, stdout);
      ERROR("Unknown iteration_statement");
    }
  } else if (prod == function_definition) {
    num_lines += codegen_function(tree, tape);
  } else if (prod == class_new_statement) {
    num_lines += codegen_new(tree, tape);
  } else if (prod == class_constructors) {
    num_lines += codegen_class_constructors(tree, tape);
  } else if (prod == anon_function) {
    num_lines += codegen_anon_function(tree, tape);
  } else if (prod == postfix_expression) {
    num_lines += codegen(tree->first, tape) + codegen(tree->second, tape);
  } else if (prod == postfix_expression1) {
    num_lines += codegen_postfix1(tree, tape);
  } else if (prod == tuple_expression) {
    int num_args = 1;
    num_lines += codegen_tuple(tree, &num_args, tape)
        + tape_ins_int(tape, TUPL, num_args, tree->second->first->token);
  } else if (prod == function_argument_list) {
    num_lines += codegen_function_arguments_list(tree, tape);
  } else if (prod == statement_list || prod == statement_list1
      || prod == file_level_statement_list || prod == file_level_statement_list1
      || prod == class_statement_list || prod == class_statement_list1) {
    num_lines += codegen(tree->first, tape) + codegen(tree->second, tape);
  } else if (prod == compound_statement) {
    if (!is_leaf(tree->second)) {
      num_lines += codegen(tree->second->first, tape);
    }
  } else if (prod == module_statement) {
    num_lines += tape_module(tape, tree->second->token);
  } else if (prod == import_statement) {
    num_lines += codegen_import(tree, tape);
  } else if (prod == class_definition) {
    num_lines += codegen_class(tree, tape);
  } else if (prod == jump_statement) {
    num_lines += codegen_jump(tree, tape);
  } else if (prod == break_statement) {
    num_lines += codegen_break(tree, tape);
  } else {
    expression_tree_to_str(*tree, NULL, stdout);
    ERROR("Unknown");
  }
  return num_lines;
}

int codegen_file(ExpressionTree *tree, Tape *tape) {
//DEBUGF("codegen_file");
  return codegen(tree, tape) + tape_ins_int(tape, EXIT, 0, NULL);
}
