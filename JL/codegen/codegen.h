/*
 * codegen.h
 *
 *  Created on: Jan 22, 2017
 *      Author: Jeff
 */

#ifndef CODEGEN_H_
#define CODEGEN_H_

#include "../tape.h"
#include "syntax.h"

int codegen(SyntaxTree *tree, Tape *tape);
int codegen_file(SyntaxTree *tree, Tape *tape);

#endif /* CODEGEN_H_ */
