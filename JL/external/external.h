/*
 * external.h
 *
 *  Created on: Jun 2, 2018
 *      Author: Jeff
 */

#ifndef EXTERNAL_H_
#define EXTERNAL_H_

#include "../datastructure/map.h"
#include "../element.h"

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
void add_io_external(VM *vm, Element builtin);

void add_external_function(VM *vm, Element parent, const char fn_name[],
    ExternalFunction fn);
Element create_external_class(VM *vm, Element module, const char class_name[],
    ExternalFunction constructor, ExternalFunction deconstructor);


bool is_value_type(const Element *e, int type);
bool is_object_type(const Element *e, int type);


#endif /* EXTERNAL_H_ */
