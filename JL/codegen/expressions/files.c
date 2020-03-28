/*
 * files.c
 *
 *  Created on: Dec 29, 2019
 *      Author: Jeff
 */

#ifdef NEW_PARSER

#include "files.h"

#include "../../arena/strings.h"
#include "../../error.h"
#include "../../program/tape.h"
#include "../syntax.h"
#include "expression.h"
#include "expression_macros.h"

ImplPopulate(module_statement, const SyntaxTree *stree) {
  module_statement->module_token = stree->first->token;
  module_statement->module_name = stree->second->token;
}

ImplDelete(module_statement) {
  // Nothing.
}

ImplProduce(module_statement, Tape *tape) {
  return tape->module(tape, module_statement->module_name);
}

ImplPopulate(import_statement, const SyntaxTree *stree) {
  if (!IS_SYNTAX(stree->second, identifier)) {
    ERROR("import AS not yet supported.");
  }
  import_statement->import_token = stree->first->token;
  import_statement->module_name = stree->second->token;
}

ImplDelete(import_statement) {
  // Nothing.
}

ImplProduce(import_statement, Tape *tape) {
  return tape->ins(tape, LMDL, import_statement->module_name);
}

Argument populate_argument(const SyntaxTree *stree) {
  Argument arg = {.is_const = false,
                  .const_token = NULL,
                  .is_field = false,
                  .has_default = false,
                  .default_value = NULL};
  const SyntaxTree *argument = stree;
  if (IS_SYNTAX(argument, const_function_argument)) {
    arg.is_const = true;
    arg.const_token = argument->first->token;
    argument = argument->second;
  }
  if (IS_SYNTAX(argument, function_arg_elt_with_default)) {
    arg.has_default = true;
    arg.default_value = populate_expression(argument->second->second);
    argument = argument->first;
  }

  ASSERT(IS_SYNTAX(argument, identifier));
  arg.arg_name = argument->token;

  return arg;
}

void add_arg(Arguments *args, Argument *arg) {
  expando_append(args->args, arg);
  if (arg->has_default) {
    args->count_optional++;
  } else {
    args->count_required++;
  }
}

Arguments set_function_args(const SyntaxTree *stree, const Token *token) {
  Arguments args = {.token = token, .count_required = 0, .count_optional = 0};
  args.args = expando(Argument, 4);
  if (!IS_SYNTAX(stree, function_argument_list)) {
    Argument arg = populate_argument(stree);
    add_arg(&args, &arg);
    return args;
  }
  Argument arg = populate_argument(stree->first);
  add_arg(&args, &arg);
  const SyntaxTree *cur = stree->second;
  while (true) {
    if (IS_TOKEN(cur->first, COMMA)) {
      // Must be last arg.
      Argument arg = populate_argument(cur->second);
      add_arg(&args, &arg);
      break;
    }
    Argument arg = populate_argument(cur->first->second);
    add_arg(&args, &arg);
    cur = cur->second;
  }
  return args;
}

void set_function_def(const SyntaxTree *fn_identifier, Function *func) {
  func->def_token = fn_identifier->first->token;
  func->fn_name = fn_identifier->second->token;
}

Function populate_function_variant(const SyntaxTree *stree, ParseExpression def,
                                   ParseExpression signature_const,
                                   ParseExpression signature_nonconst,
                                   ParseExpression fn_identifier,
                                   ParseExpression function_arguments_no_args,
                                   ParseExpression function_arguments_present,
                                   FuncDefPopulator def_populator,
                                   FuncArgumentsPopulator args_populator) {
  Function func;
  ASSERT(IS_SYNTAX(stree, def));

  const SyntaxTree *func_sig;
  if (IS_SYNTAX(stree->first, signature_const)) {
    func_sig = stree->first->first;
    func.is_const = true;
    func.const_token = stree->first->second->token;
  } else {
    ASSERT(IS_SYNTAX(stree->first, signature_nonconst));
    func_sig = stree->first;
    func.is_const = false;
    func.const_token = NULL;
  }

  ASSERT(IS_SYNTAX(func_sig->first, fn_identifier));
  def_populator(func_sig->first, &func);

  func.has_args = !IS_SYNTAX(func_sig->second, function_arguments_no_args);
  if (func.has_args) {
    ASSERT(IS_SYNTAX(func_sig->second, function_arguments_present));
    const SyntaxTree *func_args = func_sig->second->second->first;
    func.args = args_populator(func_args, func_sig->second->first->token);
  }
  func.body = populate_expression(stree->second);
  return func;
}

Function populate_function(const SyntaxTree *stree) {
  return populate_function_variant(
      stree, function_definition, function_signature_const,
      function_signature_nonconst, def_identifier, function_arguments_no_args,
      function_arguments_present, set_function_def, set_function_args);
}

ImplPopulate(function_definition, const SyntaxTree *stree) {
  function_definition->func = populate_function(stree);
}

void delete_argument(Argument *arg) {
  if (arg->has_default) {
    delete_expression(arg->default_value);
  }
}

void delete_arguments(Arguments *args) {
  void delete_argument_elt(void *ptr) {
    Argument *arg = (Argument *)ptr;
    delete_argument(arg);
  }
  expando_iterate(args->args, delete_argument_elt);
  expando_delete(args->args);
}

void delete_function(Function *func) {
  if (func->has_args) {
    delete_arguments(&func->args);
  }
  delete_expression(func->body);
}

ImplDelete(function_definition) { delete_function(&function_definition->func); }

int produce_argument(Argument *arg, Tape *tape) {
  if (arg->is_field) {
    return tape->ins_no_arg(tape, PUSH, arg->arg_name) +
           tape->ins_text(tape, RES, SELF, arg->arg_name) +
           tape->ins(tape, arg->is_const ? FLDC : FLD, arg->arg_name);
  }
  return tape->ins(tape, arg->is_const ? LETC : LET, arg->arg_name);
}

int produce_all_arguments(Arguments *args, Tape *tape) {
  int i, num_ins = 0, num_args = expando_len(args->args);
  for (i = 0; i < num_args; ++i) {
    Argument *arg = (Argument *)expando_get(args->args, i);
    if (arg->has_default) {
      num_ins += tape->ins_no_arg(tape, PEEK, args->token) +
                 tape->ins_int(tape, TGTE, i + 1, arg->arg_name);

      Tape *tmp = tape_create();
      int default_ins = produce_instructions(arg->default_value, tmp);
      num_ins += tape->ins_int(tape, IFN, 3, arg->arg_name) +
                 tape->ins_no_arg(tape, (i == num_args - 1) ? RES : PEEK,
                                  arg->arg_name) +
                 tape->ins_int(tape, TGET, i, arg->arg_name) +
                 tape->ins_int(tape, JMP, default_ins, arg->arg_name) +
                 default_ins;
      tape_append(tape, tmp);
      tape_delete(tmp);
    } else {
      if (i == num_args - 1) {
        // Pop for last arg.
        num_ins += tape->ins_no_arg(tape, RES, args->token);
      } else {
        num_ins += tape->ins_no_arg(tape, PEEK, arg->arg_name);
      }
      num_ins += tape->ins_int(tape, TGET, i, args->token);
    }
    num_ins += produce_argument(arg, tape);
  }
  return num_ins;
}

int produce_arguments(Arguments *args, Tape *tape) {
  int num_args = expando_len(args->args);
  int i, num_ins = 0;
  if (num_args == 1) {
    Argument *arg = (Argument *)expando_get(args->args, 0);
    if (arg->has_default) {
      Tape *defaults = tape_create();
      int num_default_ins = produce_instructions(arg->default_value, defaults);
      num_ins += num_default_ins + tape->ins_no_arg(tape, PUSH, arg->arg_name) +
                 tape->ins_int(tape, TGTE, 1, arg->arg_name) +
                 tape->ins_int(tape, IF, num_default_ins + 1, arg->arg_name);
      tape_append(tape, defaults);
      num_ins += tape->ins_int(tape, JMP, 1, arg->arg_name) +
                 tape->ins_int(tape, TGET, 0, arg->arg_name);

      tape_delete(defaults);
    }
    num_ins += produce_argument(arg, tape);
    return num_ins;
  }
  num_ins += tape->ins_no_arg(tape, PUSH, args->token);

  // Handle case where only 1 arg is passed and the rest are optional.
  Argument *first = (Argument *)expando_get(args->args, 0);
  num_ins += tape->ins_no_arg(tape, TLEN, first->arg_name) +
             tape->ins_no_arg(tape, PUSH, first->arg_name) +
             tape->ins_int(tape, PUSH, -1, first->arg_name) +
             tape->ins_no_arg(tape, EQ, first->arg_name);

  Tape *defaults = tape_create();
  int defaults_ins = 0;
  defaults_ins += defaults->ins_no_arg(defaults, RES, first->arg_name) +
                  produce_argument(first, defaults);
  for (i = 1; i < num_args; ++i) {
    Argument *arg = (Argument *)expando_get(args->args, i);
    if (arg->has_default) {
      defaults_ins += produce_instructions(arg->default_value, defaults);
    } else {
      defaults_ins += defaults->ins_no_arg(defaults, RNIL, arg->arg_name);
    }
    defaults_ins += produce_argument(arg, defaults);
  }

  Tape *non_defaults = tape_create();
  int nondefaults_ins = 0;
  nondefaults_ins += produce_all_arguments(args, non_defaults);

  defaults_ins +=
      defaults->ins_int(defaults, JMP, nondefaults_ins, first->arg_name);

  num_ins += tape->ins_int(tape, IFN, defaults_ins, first->arg_name);
  tape_append(tape, defaults);
  tape_append(tape, non_defaults);
  tape_delete(defaults);
  tape_delete(non_defaults);
  num_ins += defaults_ins + nondefaults_ins;
  return num_ins;
}

int produce_function(Function *func, Tape *tape) {
  int num_ins = 0;
  num_ins += tape->label(tape, func->fn_name);
  if (func->has_args) {
    num_ins += produce_arguments(&func->args, tape);
  }
  num_ins += produce_instructions(func->body, tape);
  if (func->is_const) {
    num_ins += tape->ins_no_arg(tape, CNST, func->const_token);
  }
  num_ins += tape->ins_no_arg(tape, RET, func->def_token);
  return num_ins;
}

ImplProduce(function_definition, Tape *tape) {
  return produce_function(&function_definition->func, tape);
}

ImplPopulate(file_level_statement_list, const SyntaxTree *stree) {
  file_level_statement_list->statements =
      expando(ExpressionTree *, DEFAULT_EXPANDO_SIZE);
  ExpressionTree *first = populate_expression(stree->first);
  expando_append(file_level_statement_list->statements, &first);
  const SyntaxTree *cur = stree->second;
  while (true) {
    if (!IS_SYNTAX(cur, file_level_statement_list1)) {
      ExpressionTree *statement = populate_expression(cur);
      expando_append(file_level_statement_list->statements, &statement);
      break;
    }
    ExpressionTree *statement = populate_expression(cur->first);
    expando_append(file_level_statement_list->statements, &statement);
    cur = cur->second;
  }
}

ImplDelete(file_level_statement_list) {
  void delete_statement(void *ptr) {
    ExpressionTree *statement = *((ExpressionTree **)ptr);
    delete_expression(statement);
  }
  expando_iterate(file_level_statement_list->statements, delete_statement);
  expando_delete(file_level_statement_list->statements);
}

const Token *should_jump_over_token(ExpressionTree *exp) {
  if (IS_EXPRESSION(exp, function_definition)) {
    return exp->function_definition.func.def_token;
  } else if (IS_EXPRESSION(exp, class_definition)) {
    return exp->class_definition.class.def.name.token;
  } else {
    return NULL;
  }
}

ImplProduce(file_level_statement_list, Tape *tape) {
  int num_ins = 0;
  void produce_statement(void *ptr) {
    ExpressionTree *statement = *((ExpressionTree **)ptr);
    const Token *jump_over_token = should_jump_over_token(statement);
    if (NULL != jump_over_token) {
      Tape *tmp = tape_create();
      int func_ins = produce_instructions(statement, tmp);
      num_ins += tape->ins_int(tape, JMP, func_ins, jump_over_token) + func_ins;
      tape_append(tape, tmp);
      tape_delete(tmp);
    } else {
      num_ins += produce_instructions(statement, tape);
    }
  }
  expando_iterate(file_level_statement_list->statements, produce_statement);
  num_ins += tape->ins_int(tape, EXIT, 0, NULL);
  return num_ins;
}

#endif
