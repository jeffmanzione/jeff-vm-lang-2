/*
 * util.c
 *
 *  Created on: Jul 4, 2019
 *      Author: Jeff
 */

#include "time.h"

#include "../../arena/strings.h"
#include "../../element.h"
#include "../external.h"
#include "impl/time.h"

#define USEC_PER_SEC 1000 * 1000;

Element time_timestamp_usec(VM *vm, Thread *t, ExternalData *data,
                            Element *arg) {
  return create_int(current_usec_since_epoch());
}

void add_time_external(VM *vm, Element module_element) {
  add_external_function(vm, module_element, strings_intern("now_usec"),
                        time_timestamp_usec);
}
