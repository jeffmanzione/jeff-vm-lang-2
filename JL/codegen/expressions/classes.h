/*
 * classes.h
 *
 *  Created on: Dec 30, 2019
 *      Author: Jeff
 */

#ifndef CODEGEN_EXPRESSIONS_CLASSES_H_
#define CODEGEN_EXPRESSIONS_CLASSES_H_

#ifdef NEW_PARSER

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

DefineExpression(class_definition) {
  Class class;
};

#endif

#endif /* CODEGEN_EXPRESSIONS_CLASSES_H_ */
