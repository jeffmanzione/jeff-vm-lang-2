/*
 * classes.c
 *
 *  Created on: Dec 30, 2019
 *      Author: Jeff
 */

#ifdef NEW_PARSER

#include "classes.h"

#include "../../arena/strings.h"
#include "../../datastructure/queue.h"
#include "../../program/tape.h"
#include "../syntax.h"
#include "expression_macros.h"
#include "expression.h"

void populate_class_def(ClassDef *def, const SyntaxTree *stree) {
  def->parent_classes = expando(ClassName, 2);
  // No parent classes.
  if (IS_SYNTAX(stree, identifier)) {
    def->name.token = stree->token;
  } else if (IS_SYNTAX(stree, class_name_and_inheritance)) {
    const SyntaxTree *class_inheritance = stree;
    ASSERT(IS_SYNTAX(class_inheritance->first, identifier));
    def->name.token = class_inheritance->first->token;
    ASSERT(IS_SYNTAX(class_inheritance->second, parent_classes));
    if (IS_SYNTAX(class_inheritance->second->second, identifier)) {
      ClassName name = { .token = class_inheritance->second->second->token };
      expando_append(def->parent_classes, &name);
    } else {
      ASSERT(IS_SYNTAX(class_inheritance->second->second, parent_class_list));
      ClassName name = { .token =
          class_inheritance->second->second->first->token };
      expando_append(def->parent_classes, &name);
      const SyntaxTree *parent_class = class_inheritance->second->second->second;
      while (true) {
        if (IS_SYNTAX(parent_class, parent_class_list1)) {
          if (IS_TOKEN(parent_class->first, COMMA)) {
            name.token = parent_class->second->token;
            expando_append(def->parent_classes, &name);
            break;
          } else {
            name.token = parent_class->first->second->token;
            expando_append(def->parent_classes, &name);
            parent_class = parent_class->second;
          }
        } else {
          ASSERT(IS_SYNTAX(parent_class, identifier));
          name.token = parent_class->token;
          expando_append(def->parent_classes, &name);
          break;
        }
      }
    }
  } else {
    ERROR("Unknown class name composition.");
  }
}

Function populate_constructor(const SyntaxTree *stree) {
  return populate_function_variant(stree, new_definition, new_signature_const,
      new_signature_nonconst, new_identifier, set_function_def);
}

void populate_class_statement(Class *class, const SyntaxTree *stree) {
  if (IS_SYNTAX(stree, field_statement)) {
    ASSERT(IS_TOKEN(stree->first, FIELD), IS_LEAF(stree->second));
    Field field = { .name = stree->second->token, .field_token =
        stree->first->token };
    expando_append(class->fields, &field);
  } else if (IS_SYNTAX(stree, function_definition)) {
    Function func = populate_function(stree);
    expando_append(class->methods, &func);
  } else if (IS_SYNTAX(stree, new_definition)) {
    class->constructor = populate_constructor(stree);
    class->has_constructor = true;
  } else {
    ERROR("Unknown class_statement.");
  }
}

void populate_class_statements(Class *class, const SyntaxTree *stree) {
  ASSERT(IS_TOKEN(stree->first, LBRCE));
  if (IS_TOKEN(stree->second, RBRCE)) {
    // Empty body.
    return;
  }
  if (!IS_SYNTAX(stree->second->first, class_statement_list)) {
    populate_class_statement(class, stree->second->first);
    return;
  }
  populate_class_statement(class, stree->second->first->first);
  const SyntaxTree *statement = stree->second->first->second;
  while (true) {
    if (!IS_SYNTAX(statement, class_statement_list1)) {
      populate_class_statement(class, statement);
      break;
    }
    populate_class_statement(class, statement->first);
    statement = statement->second;
  }
}

ImplPopulate(class_definition, const SyntaxTree *stree) {
  ASSERT(!IS_LEAF(stree->first), IS_TOKEN(stree->first->first, CLASS));
  class_definition->class.has_constructor = false;
  class_definition->class.fields = expando(Field, 4);
  class_definition->class.methods = expando(Function, 6);
  populate_class_def(&class_definition->class.def, stree->first->second);

  const SyntaxTree *body = stree->second;
  if (IS_SYNTAX(body, class_compound_statement)) {
    populate_class_statements(&class_definition->class, body);
  } else {
    populate_class_statement(&class_definition->class, body);
  }
}

ImplDelete(class_definition) {
  if (class_definition->class.has_constructor) {
    delete_function(&class_definition->class.constructor);
  }
  expando_delete(class_definition->class.def.parent_classes);
  expando_delete(class_definition->class.fields);
  void delete_method(void *ptr) {
    Function *func = (Function*) ptr;
    delete_function(func);
  }
  expando_iterate(class_definition->class.methods, delete_method);
  expando_delete(class_definition->class.methods);
}

int produce_constructor(Class *class, Tape *tape) {
  Function *func = &class->constructor;
  int num_ins = 0;
  if (class->has_constructor) {
    num_ins += tape->label(tape, func->fn_name);
    if (func->has_args) {
      num_ins += produce_arguments(&func->args, tape);
    }
  } else {
    num_ins += tape->label_text(tape, CONSTRUCTOR_KEY);
  }
  int num_fields = expando_len(class->fields);
  if (num_fields > 0) {
    int i;
    for (i = 0; i < num_fields; ++i) {
      Field *field = (Field*) expando_get(class->fields, i);
      num_ins += tape->ins_text(tape, PUSH, NIL_KEYWORD, field->name)
          + tape->ins_text(tape, RES, SELF, field->name)
          + tape->ins(tape, FLD, field->name);
    }
  }

  if (class->has_constructor) {
    num_ins += produce_instructions(func->body, tape);
    num_ins += tape->ins_text(tape, RES, SELF, func->fn_name);
    if (func->is_const) {
      num_ins += tape->ins_no_arg(tape, CNST, func->const_token);
    }
    num_ins += tape->ins_no_arg(tape, RET, func->def_token);
  } else {
    num_ins += tape->ins_text(tape, RES, SELF, class->def.name.token);
    num_ins += tape->ins_no_arg(tape, RET, class->def.name.token);
  }
  return num_ins;
}

ImplProduce(class_definition, Tape *tape) {
  int num_ins = 0;
  if (expando_len(class_definition->class.def.parent_classes) == 0) {
    // No parents.
    num_ins += tape->class(tape, class_definition->class.def.name.token);
  } else {
    Queue parents;
    queue_init(&parents);
    void add_parent(void *ptr) {
      ClassName *name = (ClassName*) ptr;
      queue_add(&parents, name->token->text);
    }
    expando_iterate(class_definition->class.def.parent_classes, add_parent);
    num_ins += tape->class_with_parents(tape,
        class_definition->class.def.name.token, &parents);
    queue_shallow_delete(&parents);
  }
  // Constructor
  if (class_definition->class.has_constructor
      || expando_len(class_definition->class.fields) > 0) {
    num_ins += produce_constructor(&class_definition->class, tape);
  }
  int i, num_methods = expando_len(class_definition->class.methods);
  for (i = 0; i < num_methods; ++i) {
    Function *func = (Function*) expando_get(class_definition->class.methods,
        i);
    num_ins += produce_function(func, tape);
  }
  num_ins += tape->endclass(tape, class_definition->class.def.name.token);
  return num_ins;
}

#endif
