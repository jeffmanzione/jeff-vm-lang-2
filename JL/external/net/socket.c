/*
 * socket.c
 *
 *  Created on: Jan 27, 2019
 *      Author: Jeff
 */

#include "impl/socket.h"
#include "socket.h"

#include <stddef.h>

#include "../../arena/strings.h"
#include "../../class.h"
#include "../../datastructure/array.h"
#include "../../datastructure/map.h"
#include "../../datastructure/tuple.h"
#include "../../element.h"
#include "../../error.h"
#include "../../graph/memory_graph.h"
#include "../external.h"
#include "../strings.h"

#define BUFFER_SIZE 4096
#define SOCKET_ERROR (-1)

static Element sh_class;

Element SocketHandle_constructor(VM *vm, Thread *t, ExternalData *data,
                                 Element *arg);

Element Socket_constructor(VM *vm, Thread *t, ExternalData *data,
                           Element *arg) {
  ASSERT(is_object_type(arg, TUPLE));
  Tuple *tuple = arg->obj->tuple;

  //  DEBUGF("Input = (%I64d, %I64d, %I64d, %I64d %I64d)",
  //         tuple_get(tuple, 0).val.int_val, tuple_get(tuple, 1).val.int_val,
  //         tuple_get(tuple, 2).val.int_val, tuple_get(tuple, 3).val.int_val,
  //         tuple_get(tuple, 4).val.int_val);

  Socket *socket = socket_create(
      tuple_get(tuple, 0).val.int_val, tuple_get(tuple, 1).val.int_val,
      tuple_get(tuple, 2).val.int_val, tuple_get(tuple, 3).val.int_val);

  map_insert(&data->state, strings_intern("socket"), socket);
  if (!socket_is_valid(socket)) {
    return throw_error(vm, t, "Invalid socket.");
  }
  if (SOCKET_ERROR == socket_bind(socket)) {
    return throw_error(vm, t, "Could not bind to socket.");
  }
  if (SOCKET_ERROR == socket_listen(socket, tuple_get(tuple, 4).val.int_val)) {
    return throw_error(vm, t, "Could not listen to ocket.");
  }
  return data->object;
}

Element Socket_deconstructor(VM *vm, Thread *t, ExternalData *data,
                             Element *arg) {
  Socket *socket = (Socket *)map_lookup(&data->state, strings_intern("socket"));
  if (NULL == socket) {
    return throw_error(vm, t, "Weird Socket error.");
  }
  socket_close(socket);
  socket_delete(socket);
  return create_none();
}

Element Socket_close(VM *vm, Thread *t, ExternalData *data, Element *arg) {
  Socket *socket = (Socket *)map_lookup(&data->state, strings_intern("socket"));
  if (NULL == socket) {
    return throw_error(vm, t, "Weird Socket error.");
  }
  socket_close(socket);
  return create_none();
}

// To ease finding sockethandle class.
Element Socket_accept(VM *vm, Thread *t, ExternalData *data, Element *arg) {
  Socket *socket = (Socket *)map_lookup(&data->state, strings_intern("socket"));
  if (NULL == socket) {
    return throw_error(vm, t, "Weird Socket error.");
  }
  Element socket_handle = create_external_obj(vm, sh_class);
  SocketHandle_constructor(vm, t, socket_handle.obj->external_data,
                           &data->object);

  return socket_handle;
}

Element SocketHandle_constructor(VM *vm, Thread *t, ExternalData *data,
                                 Element *arg) {
  Socket *socket = (Socket *)map_lookup(&arg->obj->external_data->state,
                                        strings_intern("socket"));
  if (NULL == socket) {
    return throw_error(vm, t, "Weird Socket error.");
  }
  SocketHandle *sh = socket_accept(socket);
  map_insert(&data->state, strings_intern("handle"), sh);

  return data->object;
}

Element SocketHandle_deconstructor(VM *vm, Thread *t, ExternalData *data,
                                   Element *arg) {
  SocketHandle *sh =
      (SocketHandle *)map_lookup(&data->state, strings_intern("handle"));
  if (NULL == sh) {
    return throw_error(vm, t, "Weird Socket error.");
  }
  sockethandle_close(sh);
  sockethandle_delete(sh);
  return create_none();
}

Element SocketHandle_close(VM *vm, Thread *t, ExternalData *data,
                           Element *arg) {
  SocketHandle *sh =
      (SocketHandle *)map_lookup(&data->state, strings_intern("handle"));
  if (NULL == sh) {
    return throw_error(vm, t, "Weird Socket error.");
  }
  sockethandle_close(sh);
  return create_none();
}

Element SocketHandle_send(VM *vm, Thread *t, ExternalData *data, Element *arg) {
  SocketHandle *sh =
      (SocketHandle *)map_lookup(&data->state, strings_intern("handle"));
  if (NULL == sh) {
    return throw_error(vm, t, "Weird Socket error.");
  }
  if (ISTYPE(*arg, class_string)) {
    String *msg = String_extract(*arg);
    sockethandle_send(sh, String_cstr(msg), String_size(msg));
    return create_none();
  } else if (ISTYPE(*arg, class_array)) {
    Array *arr = extract_array(*arg);
    int i, arr_len = Array_size(arr);
    for (i = 0; i < arr_len; ++i) {
      String *msg = String_extract(Array_get(arr, i));
      sockethandle_send(sh, String_cstr(msg), String_size(msg));
    }
    return create_none();
  } else {
    return throw_error(vm, t, "Cannot send non-string.");
  }
}

Element SocketHandle_receive(VM *vm, Thread *t, ExternalData *data,
                             Element *arg) {
  SocketHandle *sh =
      (SocketHandle *)map_lookup(&data->state, strings_intern("handle"));
  if (NULL == sh) {
    return throw_error(vm, t, "Weird Socket error.");
  }

  char buf[BUFFER_SIZE];
  int chars_received = sockethandle_receive(sh, buf, BUFFER_SIZE);

  return string_create_len(vm, buf, chars_received);
}

Element add_sockethandle_class(VM *vm, Element module) {
  sh_class = create_external_class(vm, module, strings_intern("SocketHandle"),
                                   SocketHandle_constructor,
                                   SocketHandle_deconstructor);
  add_external_method(vm, sh_class, strings_intern("send"), SocketHandle_send);
  add_external_method(vm, sh_class, strings_intern("receive"),
                      SocketHandle_receive);
  add_external_method(vm, sh_class, strings_intern("close"),
                      SocketHandle_close);
  return sh_class;
}

Element add_socket_class(VM *vm, Element module) {
  Element socket_class =
      create_external_class(vm, module, strings_intern("Socket"),
                            Socket_constructor, Socket_deconstructor);
  add_external_method(vm, socket_class, strings_intern("accept"),
                      Socket_accept);
  add_external_method(vm, socket_class, strings_intern("close"), Socket_close);
  return socket_class;
}
