/*
 * interpreter.h
 *
 *  Created on: Nov 23, 2018
 *      Author: Jeffrey
 */

#ifndef INTERPRETER_INTERPRETER_H_
#define INTERPRETER_INTERPRETER_H_

#include <stdio.h>

#include "../element.h"
#include "../tape.h"

typedef int (*InterpretFn)(VM *vm, Thread *thread, Element module, Tape *tape,
                           int num_ins);

int interpret_statement(VM *vm, Thread *t, Element m, Tape *tape, int num_ins);
int interpret_from_file(FILE *f, const char filename[], VM *vm, InterpretFn fn);

#endif /* INTERPRETER_INTERPRETER_H_ */
