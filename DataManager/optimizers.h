/*
 * optimizers.h
 *
 *  Created on: Mar 3, 2018
 *      Author: Jeff
 */

#ifndef OPTIMIZERS_H_
#define OPTIMIZERS_H_

#include "optimizer.h"
#include "tape.h"

void optimizer_ResPush(OptimizeHelper *oh, const Tape * const tape, int start,
    int end);
void optimizer_SetRes(OptimizeHelper *oh, const Tape * const tape, int start,
    int end);
void optimizer_GetPush(OptimizeHelper *oh, const Tape * const tape, int start,
    int end);
void optimizer_JmpRes(OptimizeHelper *oh, const Tape * const tape, int start,
    int end);
void optimizer_PushRes(OptimizeHelper *oh, const Tape * const tape, int start,
    int end);
void optimizer_ResPush2(OptimizeHelper *oh, const Tape * const tape, int start,
    int end);
void optimizer_RetRet(OptimizeHelper *oh, const Tape * const tape, int start,
    int end);
//void optimizer_Increment(OptimizeHelper *oh, const Tape * const tape, int start,
//    int end);

#endif /* OPTIMIZERS_H_ */
