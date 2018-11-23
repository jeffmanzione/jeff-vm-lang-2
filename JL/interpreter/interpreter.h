/*
 * interpreter.h
 *
 *  Created on: Nov 23, 2018
 *      Author: Jeffrey
 */

#ifndef INTERPRETER_INTERPRETER_H_
#define INTERPRETER_INTERPRETER_H_

#include "../element.h"
#include "../tape.h"

typedef void (*InterpretFn)(VM * vm, Element module, Tape *tape, int num_ins);

void interpret_statement(VM *vm, Element m, Tape *tape, int num_ins);
int interpret_from_file(FILE *f, const char filename[], VM *vm, InterpretFn fn);

#endif /* INTERPRETER_INTERPRETER_H_ */
