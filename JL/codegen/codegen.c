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
#include "../datastructure/queue.h"
#include "../datastructure/queue2.h"
#include "../error.h"
#include "../instruction.h"
#include "../tape.h"
#include "syntax.h"
#include "tokenizer.h"

int codegen(SyntaxTree *tree, Tape *tape);

bool is_leaf(SyntaxTree *exp_tree) {
  ASSERT_NOT_NULL(exp_tree);
  return exp_tree->token != NULL && exp_tree->first == NULL &&
         exp_tree->second == NULL;
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

int codegen_if(SyntaxTree *tree, Tape *tape) {
  // DEBUGF("codegen_if");
  int num_lines = 0;
  ASSERT(!is_leaf(tree->second), !is_leaf(tree->second->second));
  bool has_then = is_leaf(tree->second->second->first) &&
                  tree->second->second->first->token->type == THEN;
  SyntaxTree *child_expressions =
      has_then ? tree->second->second->second : tree->second->second;

  bool has_else = !is_leaf(child_expressions) &&
                  !is_leaf(child_expressions->second) &&
                  is_leaf(child_expressions->second->first) &&
                  child_expressions->second->first->token->type == ELSE;
  SyntaxTree *if_case = has_else ? child_expressions->first : child_expressions;
  int lines_for_condition = codegen(tree->second->first, tape);
  num_lines += lines_for_condition;
  Tape *tmp_tape = tape_create();

  if (has_else) {
    SyntaxTree *else_case = child_expressions->second->second;
    int lines_for_else = codegen(else_case, tmp_tape);
    num_lines +=
        lines_for_else +
        tape->ins_int(tape, IF, lines_for_else + 1 /* for the jmp to end */,
                      child_expressions->second->first->token);
    tape_append(tape, tmp_tape);
    tape_delete(tmp_tape);

    tmp_tape = tape_create();
    int lines_for_if = codegen(if_case, tmp_tape);
    num_lines +=
        lines_for_if + tape->ins_int(tape, JMP, lines_for_if,
                                     child_expressions->second->first->token);

    tape_append(tape, tmp_tape);
    tape_delete(tmp_tape);
  } else {
    int lines_for_if = codegen(if_case, tmp_tape);
    num_lines += lines_for_if +
                 tape->ins_int(tape, IFN, lines_for_if, tree->first->token);
    tape_append(tape, tmp_tape);
    tape_delete(tmp_tape);
  }
  return num_lines;
}

int codegen_while(SyntaxTree *tree, Token *while_token, Tape *tape) {
  // DEBUGF("codegen_while");
  int num_lines = 0;
  SyntaxTree *condition_expression = tree->first;
  SyntaxTree *body = tree->second;
  num_lines += tape->ins_no_arg(tape, NBLK, while_token) +
               codegen(condition_expression, tape);

  Tape *tmp_tape = tape_create();
  int lines_for_body = codegen(body, tmp_tape);
  int i;
  for (i = 0; i < tape_len(tmp_tape); ++i) {
    InsContainer *c = tape_get_mutable(tmp_tape, i);
    if (c->ins.op != JMP) {
      continue;
    }
    if (c->ins.val.int_val == 0) {
      c->ins.val.int_val = lines_for_body - i;
    } else if (c->ins.val.int_val == INT_MAX) {
      c->ins.val.int_val = -(i + 1);
    }
  }

  num_lines += lines_for_body +
               tape->ins_int(tape, IFN, lines_for_body + 1, while_token);
  tape_append(tape, tmp_tape);
  tape_delete(tmp_tape);

  num_lines += tape->ins_int(tape, JMP, -(num_lines + 1), while_token) +
               tape->ins_no_arg(tape, BBLK, while_token);

  return num_lines;
}

int codegen_for(SyntaxTree *tree, Token *for_token, Tape *tape) {
  // DEBUGF("codegen_for");
  int num_lines = 0;
  SyntaxTree *condition_expression = tree->first;
  SyntaxTree *body = tree->second;
  if (is_leaf(condition_expression->first) &&
      LPAREN == condition_expression->first->token->type) {
    condition_expression = condition_expression->second->first;
  }
  SyntaxTree *init = condition_expression->first;
  SyntaxTree *condition = condition_expression->second->second->first;
  SyntaxTree *incremental =
      condition_expression->second->second->second->second;
  num_lines += tape->ins_no_arg(tape, NBLK, for_token) + codegen(init, tape);
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
    // break
    if (c->ins.val.int_val == 0) {
      c->ins.val.int_val = lines_for_body + lines_for_incremental - i;
    }
    // continue
    else if (c->ins.val.int_val == INT_MAX) {
      c->ins.val.int_val = lines_for_body - i - 1;
    }
  }

  num_lines +=
      lines_for_body + lines_for_incremental +
      tape->ins_int(tape, IFN, lines_for_body + lines_for_incremental + 1,
                    for_token);
  tape_append(tape, tmp_tape);
  tape_delete(tmp_tape);

  num_lines += tape->ins_int(tape, JMP,
                             -(lines_for_body + lines_for_condition +
                               lines_for_incremental + 1),
                             for_token) +
               tape->ins_no_arg(tape, BBLK, for_token);

  return num_lines;
}

int codegen_foreach(SyntaxTree *tree, Token *for_token, Tape *tape) {
  int num_lines = 0;
  SyntaxTree *iterative = tree->first;
  SyntaxTree *body = tree->second;
  if (is_leaf(iterative->first) && LPAREN == iterative->first->token->type) {
    iterative = iterative->second->first;
  }
  SyntaxTree *var = iterative->first;
  if (var->expression == assignment_tuple) {
    var = var->second->first;
  }
  Token *var_token = is_leaf(var) ? var->token : var->first->token;
  SyntaxTree *iterable = iterative->second->second;
  Token *iter_token = iterative->second->first->token;

  num_lines += tape->ins_no_arg(tape, NBLK, for_token) +
               codegen(iterable, tape) +
               tape->ins_no_arg(tape, PUSH, iter_token) +
               tape->ins_text(tape, CALL, ITER_FN_NAME, iter_token) +
               tape->ins_no_arg(tape, PUSH, iter_token);

  Tape *tmp = tape_create();
  int body_lines =
      tape->ins_no_arg(tmp, DUP, var_token) +
      tape->ins_text(tmp, CALL, NEXT_FN_NAME, iter_token) +
      (is_leaf(var) ? tape->ins(tmp, SET, var_token) : codegen(var, tmp)) +
      codegen(body, tmp);

  int inc_lines = tape->ins_no_arg(tape, DUP, iter_token) +
                  tape->ins_text(tape, CALL, HAS_NEXT_FN_NAME, iter_token) +
                  tape->ins_int(tape, IFN, body_lines + 1, iter_token);

  int i;
  for (i = 0; i < tape_len(tmp); ++i) {
    InsContainer *c = tape_get_mutable(tmp, i);
    if (c->ins.op != JMP) {
      continue;
    }
    // break
    if (c->ins.val.int_val == 0) {
      c->ins.val.int_val = body_lines - i;
    }
    // continue
    else if (c->ins.val.int_val == INT_MAX) {
      c->ins.val.int_val = body_lines - i - 1;
    }
  }

  tape_append(tape, tmp);
  tape_delete(tmp);

  num_lines +=
      body_lines + inc_lines +
      tape->ins_int(tape, JMP, -(inc_lines + body_lines + 1), iter_token) +
      tape->ins_no_arg(tape, RES, iter_token) +
      tape->ins_no_arg(tape, BBLK, for_token);
  return num_lines;
}

int codegen_tuple1(SyntaxTree *tree, int *argument_count, Tape *tape) {
  // DEBUGF("codegen_tuple");
  int num_lines = 0;
  ParseExpression prod = tree->expression;
  if (prod != tuple_expression1) {
    ERROR("Not a tuple_expression1");
  }
  if (tree->second->second == NULL ||
      tree->second->second->expression != tuple_expression1) {
    (*argument_count)++;
    num_lines += codegen(tree->second, tape) +
                 tape->ins_no_arg(tape, PUSH, tree->first->token);
  } else {
    num_lines += codegen_tuple1(tree->second->second, argument_count, tape);
    (*argument_count)++;
    num_lines += codegen(tree->second->first, tape) +
                 tape->ins_no_arg(tape, PUSH, tree->first->token);
  }
  return num_lines;
}

int codegen_tuple(SyntaxTree *tree, int *num_args, Tape *tape) {
  int num_lines = 0;
  if (tree->expression != tuple_expression) {
    ERROR("Not a tuple_expression");
  }
  num_lines += codegen_tuple1(tree->second, num_args, tape) +
               codegen(tree->first, tape) +
               tape->ins_no_arg(tape, PUSH, tree->second->first->token);
  return num_lines;
}

int codegen_function_arguments_list(SyntaxTree *tree, Tape *tape, Q *args);

int codegen_function_single_arg(SyntaxTree *tree, Tape *tape, int index,
                                bool is_last, Q *args) {
  //  printf("codegen_function_single_arg ");
  //  expression_tree_to_str(tree, NULL, stdout);
  //  printf("\n");
  //  fflush(stdout);

  int num_lines = 0;

  const bool arg_is_const = !is_leaf(tree) && is_leaf(tree->first) &&
                            tree->first->token->type == CONST_T;
  Token *const_tok = NULL;
  if (arg_is_const) {
    const_tok = tree->first->token;
    tree = tree->second;
  }

  const bool has_default =
      !is_leaf(tree) && tree->second->expression == default_value_expression;
  SyntaxTree *default_val = tree->second;
  if (has_default) {
    tree = tree->first;
  }

  const bool is_single_arg = is_leaf(tree) && LPAREN != tree->token->type;

  const bool arg_is_complex = !is_leaf(tree) && is_leaf(tree->first) &&
                              tree->first->token->type == LPAREN;

  Token *arg_token = arg_is_complex ? tree->first->token : tree->token;

  if (index == 0) {
    num_lines += tape->ins_no_arg(tape, PUSH, arg_token);
  }

  if (!has_default) {
    num_lines += (is_last ? tape->ins_no_arg(tape, RES, arg_token) : 0);
  }

  if (has_default) {
    num_lines += tape->ins_no_arg(tape, TLEN, default_val->first->token) +
                 tape->ins_no_arg(tape, PUSH, default_val->first->token) +
                 tape->ins_int(tape, PUSH, index, arg_token) +
                 tape->ins_no_arg(tape, LTE, default_val->first->token);
    Tape *tmp = tape_create();
    int lines_for_default = codegen(default_val->second, tmp);
    num_lines += tape->ins_int(tape, IF, 3, default_val->first->token) +
                 tape->ins_no_arg(tape, PEEK, arg_token) +
                 tape->ins_int(tape, TGET, index, arg_token) +
                 tape->ins_int(tape, JMP, lines_for_default, arg_token);
    num_lines += lines_for_default;
    tape_append(tape, tmp);
    tape_delete(tmp);
  } else if (args == NULL) {
    num_lines += tape->ins_int(tape, TGET, index, arg_token);
  }

  if (is_single_arg) {
    if (args != NULL) {
      Q_enqueue(args, (char *)tree->token->text);
    } else {
      num_lines += tape->ins(tape, SET, tree->token);
    }
    return num_lines;
  }

  // Must be complex then.
  if (args != NULL) {
    Q_enqueue(args, NULL);
  }

  // TODO: Do something with const Tuple
  if (arg_is_complex) {
    num_lines +=
        codegen_function_arguments_list(tree->second->first, tape, args);
  } else if (arg_is_const) {
    num_lines += tape->ins_no_arg(tape, CNST, const_tok) +
                 tape->ins(tape, SETC, tree->token);
  }

  return num_lines;
}

int codegen_function_args(SyntaxTree *tree, int argument_index, Tape *tape,
                          Q *args) {
  //  printf("codegen_function_args ");
  //  expression_tree_to_str(tree, NULL, stdout);
  //  printf("\n");
  //  fflush(stdout);

  int num_lines = 0;

  ParseExpression prod = tree->expression;
  if (!is_leaf(tree) && !is_leaf(tree->first) &&
      tree->first->expression == function_argument_list) {
    return codegen_function_arguments_list(tree->first, tape, args) +
           tape->ins_no_arg(tape, RES, tree->second->token);
  }
  if (prod != function_argument_list1) {
    expression_tree_to_str(tree, NULL, stdout);
    ERROR("Not a function_argument_list1");
  }

  SyntaxTree *arg = tree->second;
  if (!is_leaf(arg) && arg->expression == function_argument_list) {
    num_lines +=
        tape->ins_no_arg(tape, PEEK, tree->first->token) +
        codegen_function_single_arg(arg->first, tape,
                                    argument_index, /*is_last=*/
                                    false, args) +
        codegen_function_args(arg->second, argument_index + 1, tape, args);
  } else {
    num_lines +=
        tape->ins_no_arg(tape, PEEK, tree->first->token) +
        codegen_function_single_arg(arg, tape, argument_index, /*is_last=*/
                                    true, args);
  }
  return num_lines;
}

int codegen_function_arguments_list(SyntaxTree *tree, Tape *tape, Q *args) {
  //  printf("codegen_function_arguments_list ");
  //  expression_tree_to_str(tree, NULL, stdout);
  //  printf("\n");
  //  fflush(stdout);

  int num_lines = 0;

  // Strip parens if necessary
  if (!is_leaf(tree) && is_leaf(tree->first) &&
      LPAREN == tree->first->token->type) {
    tree = tree->second->first;
  }
  num_lines += codegen_function_single_arg(tree->first, tape, 0, /*is_last=*/
                                           false, args);

  const bool has_additional_complex_args =
      is_leaf(tree->first) && !is_leaf(tree->second) &&
      !is_leaf(tree->second->first) &&
      tree->second->first->expression == function_argument_list;

  if (has_additional_complex_args) {
    num_lines +=
        codegen_function_arguments_list(tree->second->first, tape, args);
  }
  return num_lines + codegen_function_args(tree->second, 1, tape, args);
}

int codegen_class(SyntaxTree *tree, Tape *tape) {
  // DEBUGF("codegen_class");
  int num_lines = 0;
  SyntaxTree *class_inner = tree->second;
  SyntaxTree *class_body = class_inner->second;
  Token *class_name = class_inner->first->token;
  //  DEBUGF("CLASS: %s", class_name->text);
  if (is_leaf(class_body->first)) {
    num_lines += tape->class(tape, class_name);
  } else {
    Queue parents;
    queue_init(&parents);
    // Iters through all parent classes
    SyntaxTree *parent_class_iter = class_body->first->second;
    // When there are parent classes, the class body will actually be here.
    class_body = class_body->second;
    while (!is_leaf(parent_class_iter)) {
      SyntaxTree *parent_class = parent_class_iter->first;
      if (is_leaf(parent_class)) {
        queue_add(&parents, parent_class->token->text);
        //        DEBUGF("PARENT_CLASS: %s", parent_class->token->text);
      }
      parent_class_iter = parent_class_iter->second->second;
    }
    queue_add(&parents, parent_class_iter->token->text);
    //    DEBUGF("PARENT_CLASS LAST: %s", parent_class_iter->token->text);
    num_lines += tape->class_with_parents(tape, class_name, &parents);
    queue_shallow_delete(&parents);
  }

  if (is_leaf(class_body->first) && class_body->first->token->type == LBRCE) {
    num_lines += codegen(class_body->second->first, tape);
  } else {
    num_lines += codegen(class_body, tape);
  }
  num_lines += tape->endclass(tape, class_name);
  return num_lines;
}

int codegen_import(SyntaxTree *tree, Tape *tape) {
  // DEBUGF("codegen_import");
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
  return tape->ins(tape, RMDL, module_name) + tape->ins(tape, MDST, var_name);
}

int codegen_function_params(SyntaxTree *arg_begin, SyntaxTree **function_body,
                            bool *is_const, Tape *tape, Q *args) {
  int lines_for_func = 0;
  bool no_args =
      is_leaf(arg_begin->first) && arg_begin->first->token->type == RPAREN;
  if (no_args) {
    *function_body = arg_begin->second;
  } else if (arg_begin->first->expression != function_argument_list) {
    bool is_const = arg_begin->first->expression == const_expression &&
                    is_leaf(arg_begin->first->first) &&
                    arg_begin->first->first->token->type == CONST_T;
    SyntaxTree *id = is_const ? arg_begin->first->second : arg_begin->first;
    if (args != NULL) {
      Q_enqueue(args, (char *)id->token->text);
    }
    lines_for_func += tape->ins(tape, SET, id->token);

    if (is_leaf(arg_begin->second)) {
      *function_body = NULL;
    }
    *function_body = arg_begin->second->second;
  } else {
    lines_for_func +=
        codegen_function_arguments_list(arg_begin->first, tape, args);
    if (is_leaf(arg_begin->second)) {
      *function_body = NULL;
    }
    *function_body = arg_begin->second->second;
  }

  if (!is_leaf(*function_body) && is_leaf((*function_body)->first) &&
      (*function_body)->first->token->type == CONST_T) {
    *is_const = true;
    *function_body = (*function_body)->second;
  }
  return lines_for_func;
}

int codegen_function_helper(SyntaxTree *arg_begin, const Token *token,
                            bool *is_const, Tape *tape, Q *args) {
  SyntaxTree *function_body = NULL;
  int lines_for_func =
      codegen_function_params(arg_begin, &function_body, is_const, tape, args);

  lines_for_func += codegen(function_body, tape);
  return lines_for_func;
}

int codegen_anon_function(SyntaxTree *tree, Tape *tape) {
  // DEBUGF("codegen_anon_function");
  Tape *tmp_tape = tape_create();
  Token *token = tree->first->token;
  int lines_for_func = 0;
  SyntaxTree *arg_begin = tree->second->second;

  Q *args = Q_create();

  bool is_const = false;
  lines_for_func +=
      codegen_function_helper(arg_begin, token, &is_const, tmp_tape, args);

  if (is_const) {
    lines_for_func += tape->ins_no_arg(tmp_tape, CNST, token);
  }
  lines_for_func += tape->ins_no_arg(tmp_tape, RET, token);

  int num_lines = tape->ins_int(tape, JMP, lines_for_func, token);
  if (Q_size(args) == 0) {
    Q_delete(args);
    num_lines += tape->anon_label(tape, token);
  } else {
    num_lines += tape->anon_function_with_args(tape, token, args);
  }

  tape_append(tape, tmp_tape);
  tape_delete(tmp_tape);
  num_lines += lines_for_func + tape->ins_text(tape, RES, SELF, token) +
               tape->ins_anon(tape, GET, token);

  return num_lines;
}

int codegen_function(SyntaxTree *tree, Tape *tape) {
  // DEBUGF("codegen_function");
  Tape *tmp_tape = tape_create();

  SyntaxTree *arg_begin = tree->second->second->second;
  Token *token = tree->second->first->token;
  int lines_for_func = 0;

  Q *args = Q_create();

  bool is_const = false;
  lines_for_func +=
      codegen_function_helper(arg_begin, token, &is_const, tmp_tape, args);

  if (is_const) {
    lines_for_func += tape->ins_no_arg(tmp_tape, CNST, token);
  }
  lines_for_func += tape->ins_no_arg(tmp_tape, RET, token);

  int num_lines = tape->ins_int(tape, JMP, lines_for_func, token);
  if (Q_size(args) == 0) {
    Q_delete(args);
    num_lines += tape->label(tape, token);
  } else {
    num_lines += tape->function_with_args(tape, token, args);
  }

  tape_append(tape, tmp_tape);
  tape_delete(tmp_tape);
  num_lines += lines_for_func;

  return num_lines;
}

int codegen_new(SyntaxTree *tree, Tape *tape) {
  // DEBUGF("codegen_function");
  Tape *tmp_tape = tape_create();

  SyntaxTree *arg_begin = tree->second->second->second;
  Token *token = tree->second->first->token;
  int lines_for_func = 0;

  Q *args = Q_create();

  bool is_const = false;
  lines_for_func +=
      codegen_function_helper(arg_begin, token, &is_const, tmp_tape, args);

  lines_for_func += tape->ins_text(tmp_tape, RES, SELF, token);
  if (is_const) {
    lines_for_func += tape->ins_no_arg(tmp_tape, CNST, token);
  }
  lines_for_func += tape->ins_no_arg(tmp_tape, RET, token);

  int num_lines = tape->ins_int(tape, JMP, lines_for_func, token);
  if (Q_size(args) == 0) {
    Q_delete(args);
    num_lines += tape->label(tape, token);
  } else {
    num_lines += tape->function_with_args(tape, token, args);
  }

  tape_append(tape, tmp_tape);
  tape_delete(tmp_tape);
  num_lines += lines_for_func;

  return num_lines;
}

int codegen_postfix1(SyntaxTree *tree, Tape *tape) {
  // DEBUGF("codegen_postfix1");
  int num_lines = 0;
  ParseExpression prod = tree->expression;
  if (prod != postfix_expression1) {
    ERROR("Was not a postfix_expression1");
  }
  if (is_leaf(tree)) {
    ERROR("Improperly handled postfix. Did not expect leaf");
  }
  SyntaxTree *ext = tree->first;
  SyntaxTree *tail = tree->second;
  switch (ext->token->type) {
    case (PERIOD):
      if (is_leaf(tail)) {
        num_lines += tape->ins(tape, GET, tail->token);
      } else {
        if (!is_leaf(tail->second) && is_leaf(tail->second->first) &&
            tail->second->first->token->type == LPAREN) {
          num_lines += tape->ins_no_arg(tape, PUSH, tail->second->first->token);
          if (!is_leaf(tail->second->second) &&
              is_leaf(tail->second->second->first) &&
              tail->second->second->first->token->type == RPAREN) {
            num_lines += tape->ins(tape, CALL, tail->first->token);
            num_lines += codegen(tail->second->second->second, tape);
          } else {
            if (!is_leaf(tail->second->second)) {
              num_lines += codegen(tail->second->second->first, tape);
            }
            num_lines += tape->ins(tape, CALL, tail->first->token);
            if (!is_leaf(tail->second->second) &&
                !is_leaf(tail->second->second->second)) {
              num_lines += codegen(tail->second->second->second->second, tape);
            }
          }
        } else {
          num_lines += tape->ins(tape, GET, tail->first->token);
          num_lines += codegen(tail->second, tape);
        }
      }
      break;
    case (LPAREN):
      num_lines += tape->ins_no_arg(tape, PUSH, ext->token);
      if (!is_leaf(tail) && is_leaf(tail->first) &&
          tail->first->token->type == RPAREN) {
        num_lines += tape->ins_no_arg(tape, CALL, ext->token);
        num_lines += codegen(tail->second, tape);
      } else {
        if (!is_leaf(tail)) {
          num_lines += codegen(tail->first, tape);
        }
        num_lines += tape->ins_no_arg(tape, CALL, ext->token);
        if (!is_leaf(tail) && !is_leaf(tail->second)) {
          num_lines += codegen(tail->second->second, tape);
        }
      }
      break;
    case (LBRAC):
      num_lines += tape->ins_no_arg(tape, PUSH, ext->token);
      if (tail->token == NULL || tail->token->type != RBRAC) {
        num_lines += codegen(tail->first, tape);
      }
      num_lines += tape->ins_no_arg(tape, AIDX, ext->token);

      if (NULL != tail->second && !is_leaf(tail->second)) {
        num_lines += codegen(tail->second->second, tape);
      }
      break;
    default:
      ERROR("Unknown postfix.");
  }
  return num_lines;
}

int codegen_jump(SyntaxTree *tree, Tape *tape) {
  // DEBUGF("codegen_jump");
  ParseExpression prod = tree->expression;
  if (prod != jump_statement) {
    ERROR("Was not a jump_statement");
  }
  if (is_leaf(tree)) {
    if (tree->token->type != RETURN) {
      ERROR("jump_statement not implemented");
    }
    return tape->ins_no_arg(tape, RET, tree->token);
  }

  SyntaxTree *jump_type = tree->first;
  SyntaxTree *jump_value = tree->second;
  if (jump_type->token->type != RETURN) {
    ERROR("jump_statement not implemented");
  }
  // If surrounded in parens, then use what's inside.
  if (!is_leaf(jump_value) && is_leaf(jump_value->first) &&
      LPAREN == jump_value->first->token->type) {
    jump_value = jump_value->second->first;
  }
  return codegen(jump_value, tape) +
         tape->ins_no_arg(tape, RET, jump_type->token);
}

int codegen_break(SyntaxTree *tree, Tape *tape) {
  ParseExpression prod = tree->expression;
  if (prod != break_statement) {
    ERROR("Was not a break_statement");
  }
  if (BREAK == tree->token->type) {
    return tape->ins_int(tape, JMP, 0, tree->token);
  } else if (CONTINUE == tree->token->type) {
    return tape->ins_int(tape, JMP, INT_MAX, tree->token);
  } else {
    ERROR("Unknown break statement.");
    return 0;
  }
}

int codegen_exit(SyntaxTree *tree, Tape *tape) {
  ParseExpression prod = tree->expression;
  if (prod != exit_statement) {
    ERROR("Was not an exit_statement");
  }
  return tape->ins_no_arg(tape, EXIT, tree->token);
}

int codegen_raise(SyntaxTree *tree, Tape *tape) {
  ParseExpression prod = tree->expression;
  if (prod != raise_statement) {
    ERROR("Was not an raise_statement");
  }
  return codegen(tree->second, tape) +
         tape->ins_no_arg(tape, RAIS, tree->first->token);
}

int codegen_range(SyntaxTree *tree, Tape *tape) {
  ParseExpression prod = tree->expression;
  int num_lines = 0;
  if (prod != range_expression) {
    ERROR("Was not an range_expression");
  }
  SyntaxTree *start = tree->first;
  SyntaxTree *end, *inc = NULL;
  int num_args = 2;
  if (is_leaf(tree->second->second)) {
    end = tree->second->second;
  } else {
    end = tree->second->second->second->second;
    inc = tree->second->second->first;
    num_args = 3;
  }
  num_lines += tape->ins_text(tape, PUSH, strings_intern("range"),
                              tree->second->first->token);
  if (NULL != inc) {
    num_lines += codegen(inc, tape) + tape->ins_no_arg(tape, PUSH, inc->token);
  }
  num_lines += codegen(end, tape) + tape->ins_no_arg(tape, PUSH, end->token) +
               codegen(start, tape) +
               tape->ins_no_arg(tape, PUSH, start->token);

  num_lines += tape->ins_int(tape, TUPL, num_args, tree->second->first->token);
  return num_lines + tape->ins_no_arg(tape, CALL, tree->second->first->token);
}

int codegen_try(SyntaxTree *tree, Tape *tape) {
  ParseExpression prod = tree->expression;
  if (prod != try_statement) {
    ERROR("Was not a try_statement");
  }
  SyntaxTree *try_body = tree->second->first;
  SyntaxTree *error_identifier;
  SyntaxTree *catch_body;
  if (LPAREN == tree->second->second->second->first->token->type) {
    error_identifier = tree->second->second->second->second->first;
    catch_body = tree->second->second->second->second->second->second;
  } else {
    error_identifier = tree->second->second->second->first;
    catch_body = tree->second->second->second->second;
  }
  int num_lines = 0;
  Tape *try_body_tape = tape_create();
  int try_lines = codegen(try_body, try_body_tape);
  Tape *catch_body_tape = tape_create();
  int catch_lines = codegen(catch_body, catch_body_tape);

  int goto_pos = try_lines - 1;

  num_lines +=
      tape->ins_int(tape, CTCH, goto_pos, tree->first->token) + try_lines;
  tape_append(tape, try_body_tape);
  num_lines += tape->ins_int(tape, JMP, catch_lines + 1, tree->first->token);
  // Expect error to be in resval
  num_lines += tape->ins(tape, SET, error_identifier->token);
  num_lines += catch_lines;
  tape_append(tape, catch_body_tape);
  num_lines += tape->ins_text(tape, RES, NIL_KEYWORD, tree->first->token) +
               tape->ins_text(tape, SET, "$try_goto", tree->first->token);

  tape_delete(try_body_tape);
  tape_delete(catch_body_tape);

  return num_lines;
}

int codegen_assignment_ext(SyntaxTree *tree, Tape *tape) {
  // DEBUGF("codegen_assignment_ext");
  ParseExpression prod = tree->expression;
  if (prod != postfix_expression1) {
    ERROR("Was not a postfix_expression1");
  }
  SyntaxTree *ext = tree->first;
  SyntaxTree *tail = tree->second;

  int num_lines = 0;

  switch (ext->token->type) {
    case PERIOD:
      if (is_leaf(tail)) {
        num_lines += tape->ins(tape, FLD, tail->token);
      } else {
        if (!is_leaf(tail->second) &&
            tail->second->first->token->type == LPAREN) {
          num_lines += tape->ins_no_arg(tape, PUSH, tail->second->first->token);
          if (!is_leaf(tail->second->second)) {
            num_lines += codegen(tail->second->second->first, tape);
          }
          num_lines += tape->ins(tape, CALL, tail->first->token);
        } else {
          num_lines += tape->ins(tape, GET, tail->first->token) +
                       codegen_assignment_ext(tail->second, tape);
        }
      }
      break;
    case LPAREN:
      num_lines += tape->ins_no_arg(tape, PUSH, ext->token);
      if (!is_leaf(tail)) {
        num_lines += codegen(tail->first, tape);
      }
      num_lines += tape->ins_no_arg(tape, CALL, ext->token);

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
      num_lines += tape->ins_no_arg(tape, PUSH, ext->token);
      if (is_leaf(tail)) {
        ERROR("NOT ALLOWED");
      }
      num_lines += codegen(tail->first, tape);

      if (is_leaf(tail->second)) {
        num_lines += tape->ins_no_arg(tape, ASET, tail->second->token);
      } else {
        num_lines += (tape->ins_no_arg(tape, AIDX, ext->token) +
                      codegen_assignment_ext(tail->second->second, tape));
      }
      break;
    default:
      ERROR("Unknown postfix.");
  }
  return num_lines;
}

int codegen_in(SyntaxTree *lhs, SyntaxTree *rhs, Token *in, Tape *tape) {
  return codegen(rhs, tape) + tape->ins_no_arg(tape, PUSH, in) +
         codegen(lhs, tape) + tape->ins_text(tape, CALL, IN_FN_NAME, in) +
         ((NOTIN == in->type) ? tape->ins_no_arg(tape, NOT, in) : 0);
}

int codegen_assignment(SyntaxTree *lhs, Token *eq_sign, SyntaxTree *rhs,
                       Tape *tape) {
  ParseExpression prod = lhs->expression;
  int num_lines = codegen(rhs, tape);
  if (is_leaf(lhs)) {
    num_lines += tape->ins(tape, SET, lhs->token);
  } else if (prod == const_expression && is_leaf(lhs->first) &&
             lhs->first->token->type == CONST_T) {
    num_lines += tape->ins(tape, SETC, lhs->second->token);
  } else if (prod == postfix_expression) {
    num_lines +=
        (tape->ins_no_arg(tape, PUSH, eq_sign) + codegen(lhs->first, tape) +
         codegen_assignment_ext(lhs->second, tape));
  } else if (prod == function_argument_list) {
    num_lines += codegen(lhs, tape);
  } else if (prod == assignment_tuple) {
    num_lines += codegen(lhs->second->first, tape);
  } else {
    ERROR("Unknown lhs assignment.");
  }
  return num_lines;
}

int codegen_unary(SyntaxTree *tree, Tape *tape) {
  int num_lines = 0;
  if (!is_leaf(tree->first)) {
    ERROR("Unary is not a leaf");
  }
  Token *unary_tok = tree->first->token;
  TokenType unary_type = unary_tok->type;

  if (MINUS == unary_type && is_leaf(tree->second) &&
      (INTEGER == tree->second->token->type ||
       FLOATING == tree->second->token->type)) {
    return tape->ins_neg(tape, RES, tree->second->token);
  }

  num_lines += codegen(tree->second, tape);
  switch (unary_type) {
    case TILDE:
      num_lines += tape->ins_no_arg(tape, NOT, unary_tok);
      break;
    case EXCLAIM:
      num_lines += tape->ins_no_arg(tape, NOTC, unary_tok);
      break;
    case MINUS:
      num_lines += tape->ins_no_arg(tape, PUSH, unary_tok) +
                   tape->ins_int(tape, PUSH, -1, unary_tok) +
                   tape->ins_no_arg(tape, MULT, unary_tok);
      break;
    case CONST_T:
      num_lines += tape->ins_no_arg(tape, CNST, unary_tok);
      break;
    default:
      ERROR("Unknown unary");
  }
  return num_lines;
}

int codegen(SyntaxTree *tree, Tape *tape) {
  // DEBUGF("codegen_");
  int num_lines = 0;
  ParseExpression prod = tree->expression;
  if (prod == identifier || prod == string_literal || prod == constant) {
    num_lines += tape->ins(tape, RES, tree->token);
  } else if (prod == array_declaration) {
    if (!is_leaf(tree->second)) {
      if (tree->second->first->expression == tuple_expression) {
        int num_args = 1;
        num_lines += codegen_tuple(tree->second->first, &num_args, tape) +
                     tape->ins_int(tape, ANEW, num_args, tree->first->token);
      } else {
        num_lines += codegen(tree->second->first, tape) +
                     tape->ins_no_arg(tape, PUSH, tree->first->token) +
                     tape->ins_int(tape, ANEW, 1, tree->first->token);
      }
    } else {
      num_lines += tape->ins_no_arg(tape, ANEW, tree->second->token);
    }
  } else if (prod == length_expression) {
    if (!is_leaf(tree->first) || is_leaf(tree->second) ||
        !is_leaf(tree->second->second)) {
      ERROR("Invalid length_expression");
    }
    num_lines += codegen(tree->second->first, tape) +
                 tape->ins_text(tape, GET, LENGTH_KEY, tree->first->token);
  } else if (prod == primary_expression ||
             prod == primary_expression_no_constants) {
    if (is_leaf(tree->first) && tree->first->token->type == LPAREN) {
      num_lines += codegen(tree->second->first, tape);
    } else {
      expression_tree_to_str(tree, NULL, stdout);
      ERROR("Unknown primary_expression");
    }
  } else if (prod == in_expression) {
    num_lines += codegen_in(tree->first, tree->second->second,
                            tree->second->first->token, tape);
  } else if (prod == additive_expression || prod == multiplicative_expression ||
             prod == relational_expression || prod == equality_expression ||
             prod == and_expression || prod == xor_expression ||
             prod == or_expression || prod == is_expression) {
    Token *token;
    if (!is_leaf(tree->second) && !is_leaf(tree->second->second) &&
        !is_leaf(tree->second->second->second) &&
        is_leaf(tree->second->second->second->first)) {
      SyntaxTree *second = tree->second->second;
      SyntaxTree *op = second->second->first;
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
        case IS_T:
          second->expression = is_expression;
          break;
        default:
          break;
      }
    } else {
      token = tree->second->first->token;
    }
    num_lines +=
        codegen(tree->first, tape) + tape->ins_no_arg(tape, PUSH, token) +
        codegen(tree->second->second, tape) +
        tape->ins_no_arg(tape, PUSH, token) +
        tape->ins_no_arg(tape, token_to_op(tree->second->first->token), token);
  } else if (prod == unary_expression) {
    num_lines += codegen_unary(tree, tape);
  } else if (prod == assignment_expression) {
    num_lines += codegen_assignment(tree->first, tree->second->first->token,
                                    tree->second->second, tape);
  } else if (prod == conditional_expression || prod == selection_statement) {
    num_lines += codegen_if(tree, tape);
  } else if (prod == while_statement) {
    num_lines += codegen_while(tree->second, tree->first->token, tape);
  } else if (prod == for_statement) {
    num_lines += codegen_for(tree->second, tree->first->token, tape);
  } else if (prod == foreach_statement) {
    num_lines += codegen_foreach(tree->second, tree->first->token, tape);
  } else if (prod == function_definition) {
    num_lines += codegen_function(tree, tape);
  } else if (prod == class_new_statement) {
    num_lines += codegen_new(tree, tape);
  } else if (prod == anon_function) {
    num_lines += codegen_anon_function(tree, tape);
  } else if (prod == postfix_expression) {
    num_lines += codegen(tree->first, tape) + codegen(tree->second, tape);
  } else if (prod == postfix_expression1) {
    num_lines += codegen_postfix1(tree, tape);
  } else if (prod == tuple_expression) {
    int num_args = 1;
    num_lines +=
        codegen_tuple(tree, &num_args, tape) +
        tape->ins_int(tape, TUPL, num_args, tree->second->first->token);
  } else if (prod == function_argument_list) {
    num_lines += codegen_function_arguments_list(tree, tape, NULL);
  } else if (prod == statement_list || prod == statement_list1 ||
             prod == file_level_statement_list ||
             prod == file_level_statement_list1 ||
             prod == class_statement_list || prod == class_statement_list1) {
    num_lines += codegen(tree->first, tape) + codegen(tree->second, tape);
  } else if (prod == compound_statement) {
    if (!is_leaf(tree->second)) {
      num_lines += codegen(tree->second->first, tape);
    }
  } else if (prod == module_statement) {
    num_lines += tape->module(tape, tree->second->token);
  } else if (prod == import_statement) {
    num_lines += codegen_import(tree, tape);
  } else if (prod == class_definition) {
    num_lines += codegen_class(tree, tape);
  } else if (prod == jump_statement) {
    num_lines += codegen_jump(tree, tape);
  } else if (prod == break_statement) {
    num_lines += codegen_break(tree, tape);
  } else if (prod == try_statement) {
    num_lines += codegen_try(tree, tape);
  } else if (prod == exit_statement) {
    num_lines += codegen_exit(tree, tape);
  } else if (prod == raise_statement) {
    num_lines += codegen_raise(tree, tape);
  } else if (prod == range_expression) {
    num_lines += codegen_range(tree, tape);
  } else {
    expression_tree_to_str(tree, NULL, stdout);
    ERROR("Unknown");
  }
  return num_lines;
}

int codegen_file(SyntaxTree *tree, Tape *tape) {
  // DEBUGF("codegen_file");
  return codegen(tree, tape) + tape->ins_int(tape, EXIT, 0, NULL);
}
