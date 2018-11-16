/*
 * modules.h
 *
 *  Created on: Jun 24, 2018
 *      Author: Jeff
 */

#ifndef EXTERNAL_MODULES_H_
#define EXTERNAL_MODULES_H_

#include "../element.h"
#include "../threads/thread.h"

Element load_module__(VM *vm, Thread *t, ExternalData *ed, Element argument);

#endif /* EXTERNAL_MODULES_H_ */
