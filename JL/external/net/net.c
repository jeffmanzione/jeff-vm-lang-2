/*
 * net.c
 *
 *  Created on: Jan 28, 2019
 *      Author: Jeff
 */

#include "net.h"

#include "../../arena/strings.h"
#include "../external.h"
#include "impl/socket.h"
#include "socket.h"

Element net_init(VM *vm, Thread *t, ExternalData *data, Element arg) {
  sockets_init();
  return create_none();
}

Element net_cleanup(VM *vm, Thread *t, ExternalData *data, Element arg) {
  sockets_cleanup();
  return create_none();
}

void add_net_external(VM *vm, Element module_element) {
  add_external_function(vm, module_element, strings_intern("init"), net_init);
  add_external_function(vm, module_element, strings_intern("cleanup"),
      net_cleanup);
  add_sockethandle_class(vm, module_element);
  add_socket_class(vm, module_element);
}

