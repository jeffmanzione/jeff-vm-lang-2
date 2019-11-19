/*
 * module_merge.h
 *
 *  Created on: Jan 5, 2019
 *      Author: Jeff
 */

#ifndef VM_PRELOADED_MODULES_H_
#define VM_PRELOADED_MODULES_H_

#include "../element.h"

void maybe_merge_existing_source(VM *vm, Element module_element,
                                 const char fn[]);

#endif /* VM_PRELOADED_MODULES_H_ */
