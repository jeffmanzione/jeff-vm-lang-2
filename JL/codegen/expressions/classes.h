/*
 * classes.h
 *
 *  Created on: Dec 30, 2019
 *      Author: Jeff
 */

#ifndef CODEGEN_EXPRESSIONS_CLASSES_H_
#define CODEGEN_EXPRESSIONS_CLASSES_H_

#include <stdbool.h>

#include "../../datastructure/expando.h"
#include "../tokenizer.h"
#include "expression_macros.h"
#include "files.h"

typedef struct {
  const Token *token;
} ClassName;

typedef struct {
  ClassName name;
  Expando *parent_classes;
} ClassDef;

typedef struct {
  const Token *name;
  const Token *field_token;
} Field;

typedef struct {
  ClassDef def;
  Expando *fields;
  bool has_constructor;
  Function constructor;
  Expando *methods;  // Function.
} Class;


Class populate_class(const SyntaxTree *stree);
int produce_class(Class *class, Tape *tape);
void delete_class(Class *class);

#endif /* CODEGEN_EXPRESSIONS_CLASSES_H_ */
