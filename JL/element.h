/*
 * element.h
 *
 *  Created on: Sep 27, 2016
 *      Author: Jeff
 */

#ifndef ELEMENT_H_
#define ELEMENT_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "datastructure/map.h"

typedef struct Array_ Array;
typedef struct Tuple_ Tuple;
typedef struct Module_ Module;
typedef struct VM_ VM;
typedef struct MemoryGraph_ MemoryGraph;
typedef struct Node_ Node;

typedef struct Element_ Element;

typedef struct ExternalData_ ExternalData;
typedef Element (*ExternalFunction)(VM *, ExternalData *, Element);

typedef struct Object_ {
  // Pointer to node owner.
  Node *node;
//  Class *class;
  bool fields_immutable;  // should be set to true for anything not OBJ
  Map fields;
  bool is_external;
  enum {
    OBJ, ARRAY, TUPLE, MODULE, FUNCTION, EXTERNAL_FUNCTION
  } type;
  union {
    Array *array;
    Tuple *tuple;
    const Module *module;
    ExternalFunction external_fn;
    ExternalData *external_data;
  };
} Object;

// Do not manually access any of these =(
typedef struct Value_ {
  enum {
    INT, FLOAT, CHAR
  } type;
  union {
    int8_t char_val;
    int64_t int_val;
    double float_val;
  };
} Value;

typedef struct Element_ {
  enum {
    NONE, OBJECT, VALUE
  } type;
  union {
    Object *obj;
    Value val;
  };
} Element;

Element create_none();
Element create_int(int64_t val);
Element create_float(double val);
Element create_char(int8_t val);
Element create_obj(MemoryGraph *graph);
Element create_obj_unsafe(MemoryGraph *graph);
Element create_obj_of_class(MemoryGraph *graph, Element class);
Element create_external_obj(VM *vm, Element class);
Element create_array(MemoryGraph *graph);

Element string_create(VM *vm, const char *str);
Element string_create_unsafe(VM *vm, const char *str);
Element string_add(VM *vm, Element str1, Element str2);

Element create_tuple(MemoryGraph *graph);
Element create_module(VM *vm, const Module *module);
Element create_function(VM *vm, Element module, uint32_t ins, const char name[]);
Element create_external_function(VM *vm, Element module, const char name[],
    ExternalFunction external_fn);
Element create_method(VM *vm, Element module, uint32_t ins, Element class,
    const char name[]);
Element val_to_elt(Value val);
Value value_negate(Value val);
void obj_set_field(Element elt, const char field_name[], Element field_val);
Element obj_get_field(Element elt, const char field_name[]);
void obj_delete_ptr(Object *obj, bool free_mem);

// Will fail if there is a cycle
void obj_to_str(Object *obj, FILE *file);
void elt_to_str(Element elt, FILE *file);
void val_to_str(Value val, FILE *file);

Element value_fmt(VM *vm, Value val, Element fmt);

Element element_true(VM *vm);
Element element_false(VM *vm);
Element element_not(VM *vm, Element elt);

bool is_true(Element elt);
bool is_false(Element elt);

char *string_to_cstr(Array *arr);

#endif /* ELEMENT_H_ */
