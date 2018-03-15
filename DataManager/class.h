/*
 * class.h
 *
 *  Created on: Oct 3, 2016
 *      Author: Jeff
 */

#ifndef CLASS_H_
#define CLASS_H_

#include <stdbool.h>

#include "element.h"

#define ISOBJECT(elt) ((elt).type == OBJECT)
#define ISCLASS(elt) (ISOBJECT(elt) && class_class.obj ==                      \
                                         obj_get_field((elt), CLASS_KEY).obj)
#define ISTYPE(elt, class) (ISCLASS(class) && ISOBJECT(elt) &&                 \
                               obj_get_field((elt), CLASS_KEY).obj == (class).obj)

extern Element class_class;
extern Element class_object;
extern Element class_array;
extern Element class_string;

extern Element class_tuple;
extern Element class_function;
extern Element class_method;
extern Element class_module;

void class_init(VM *vm);
void class_fill(VM *vm, Element class, const char class_name[],
    Element parent_class);
void class_fill_unsafe(VM *vm, Element class, const char class_name[],
    Element parent_class);
Element class_create(VM *vm, const char class_name[], Element parent_class);

bool inherits_from(Element class, Element super);

#endif /* CLASS_H_ */
