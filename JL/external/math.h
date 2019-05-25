/*
 * math.h
 *
 *  Created on: Aug 3, 2018
 *      Author: Jeff
 */

#ifndef EXTERNAL_MATH_H_
#define EXTERNAL_MATH_H_

#include "../element.h"

Element log__(VM *vm, Thread *t, ExternalData *ed, Element *argument);
Element pow__(VM *vm, Thread *t, ExternalData *ed, Element *argument);

#endif /* EXTERNAL_MATH_H_ */
