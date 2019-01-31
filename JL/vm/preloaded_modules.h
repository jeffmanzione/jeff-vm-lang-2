/*
 * module_merge.h
 *
 *  Created on: Jan 5, 2019
 *      Author: Jeff
 */

#ifndef VM_PRELOADED_MODULES_H_
#define VM_PRELOADED_MODULES_H_

#include "../element.h"


#ifdef DEBUG
#define BUILTIN_SRC "builtin.jl"
#else
#define BUILTIN_SRC "builtin.jb"
#endif


extern const char *PRELOADED[];

int preloaded_size();

void maybe_merge_existing_source(VM *vm, Element module_element,
    const char fn[]);

#endif /* VM_PRELOADED_MODULES_H_ */
