/*
 * codegen.h
 *
 *  Created on: Jan 22, 2017
 *      Author: Jeff
 */

#ifndef CODEGEN_H_
#define CODEGEN_H_

#include "../tape.h"
#include "expression.h"

int codegen(ExpressionTree *tree, Tape *tape);
int codegen_file(ExpressionTree *tree, Tape *tape);

#endif /* CODEGEN_H_ */
