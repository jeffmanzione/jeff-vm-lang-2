/*
 * strings.h
 *
 *  Created on: Jun 3, 2018
 *      Author: Jeff
 */

#ifndef EXTERNAL_STRINGS_H_
#define EXTERNAL_STRINGS_H_

#include "../datastructure/arraylike.h"
#include "../element.h"

Element stringify__(VM *vm, Thread *t, ExternalData *ed, Element *argument);

DEFINE_ARRAYLIKE(String, char);

void String_insert(String *string, int index_in_string, const char src[],
                   size_t len);
void String_append_cstr(String *string, const char src[], size_t len);
void String_append_unescape(String *string, const char str[], size_t len);

String *String_of(VM *vm, ExternalData *data, const char *src, size_t len);
void String_fill(VM *vm, ExternalData *data, String *string);

const char *String_cstr(const String *const string);

String *String_extract(Element elt);

void merge_string_class(VM *vm, Element string_class);
Element string_constructor(VM *vm, Thread *t, ExternalData *data, Element *arg);
Element string_deconstructor(VM *vm, Thread *t, ExternalData *data,
                             Element *arg);

#endif /* EXTERNAL_STRINGS_H_ */
