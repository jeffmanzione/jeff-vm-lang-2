/*
 * external.h
 *
 *  Created on: Jun 2, 2018
 *      Author: Jeff
 */

#ifndef EXTERNAL_H_
#define EXTERNAL_H_

#include <stdbool.h>

#include "../datastructure/map.h"
#include "../element.h"
#include "../vm.h"

// Protect with methods
typedef struct ExternalData_ {
  Map state;
  VM *vm;
  Element object;
  ExternalFunction deconstructor;
} ExternalData;

ExternalData *externaldata_create(VM *vm, Element obj, Element class);
void externaldata_delete(ExternalData *ed);
VM *externaldata_vm(const ExternalData * const ed);

void add_builtin_external(VM *vm, Element builtin);
void add_global_builtin_external(VM *vm, Element builtin);
void add_io_external(VM *vm, Element builtin);

Element add_external_function(VM *vm, Element parent, const char fn_name[],
    ExternalFunction fn);
Element add_external_method(VM *vm, Element class, const char fn_name[],
    ExternalFunction fn);
Element add_global_external_function(VM *vm, Element parent,
    const char fn_name[], ExternalFunction fn);

void merge_external_class(VM *vm, Element class, ExternalFunction constructor,
    ExternalFunction deconstructor);
Element create_external_class(VM *vm, Element module, const char class_name[],
    ExternalFunction constructor, ExternalFunction deconstructor);

void merge_object_class(VM *vm);
void merge_array_class(VM *vm);

bool is_value_type(const Element *e, int type);
bool is_object_type(const Element *e, int type);

Element throw_error(VM *vm, Thread *t, const char msg[]);

#endif /* EXTERNAL_H_ */
