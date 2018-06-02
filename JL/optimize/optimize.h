/*
 * optimize.h
 *
 *  Created on: Jan 13, 2018
 *      Author: Jeff
 */

#ifndef OPTIMIZE_H_
#define OPTIMIZE_H_

#include "../tape.h"
#include "optimizer.h"

void optimize_init();
void optimize_finalize();
Tape *optimize(Tape * const t);

void register_optimizer(const char name[], const Optimizer o);

void Int32_swap(void *x, void *y);
int Int32_compare(void *x, void *y);

#endif /* OPTIMIZE_H_ */
