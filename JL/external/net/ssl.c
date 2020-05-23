/*
 * ssl.c
 *
 *  Created on: May 13, 2020
 *      Author: Jeff
 */

#include "impl/ssl.h"

#include <stddef.h>

#include "../../arena/strings.h"
#include "../../class.h"
#include "../../datastructure/array.h"
#include "../../datastructure/map.h"
#include "../../datastructure/tuple.h"
#include "../../element.h"
#include "../../error.h"
#include "../../memory/memory_graph.h"
#include "../external.h"
#include "../strings.h"
#include "socket.h"

#define BUFFER_SIZE 4096
#define SOCKET_ERROR (-1)

static Element ssl_sh_class;

Element SSLSocketHandle_constructor(VM *vm, Thread *t, ExternalData *data,
                                    Element *arg);

Element SSLSocket_constructor(VM *vm, Thread *t, ExternalData *data,
                              Element *arg) {
  ASSERT(is_object_type(arg, TUPLE));
  Tuple *tuple = arg->obj->tuple;

  if (tuple_size(tuple) != 3) {
    return throw_error(vm, t, "Expected 3 arguments to SSLSocket().");
  }
  Element socket = tuple_get(tuple, 0);
  if (!ISTYPE(socket, class_socket)) {
    return throw_error(vm, t, "First argument must be a Socket.");
  }
  Element certificate_file_name = tuple_get(tuple, 1);
  if (!ISTYPE(certificate_file_name, class_string)) {
    return throw_error(vm, t, "Second argumet must be certificate file name.");
  }
  Element private_key_file_name = tuple_get(tuple, 2);
  if (!ISTYPE(private_key_file_name, class_string)) {
    return throw_error(vm, t, "Second argumet must be private key file name.");
  }
  Socket *raw_socket = (Socket *)map_lookup(&socket.obj->external_data->state,
                                            strings_intern("socket"));
  if (NULL == raw_socket || !socket_is_valid(raw_socket)) {
    return throw_error(vm, t, "Invalid socket.");
  }
  int status;
  SSLSocket *ssl_socket =
      sslsocket_create(raw_socket, string_to_cstr(certificate_file_name),
                       string_to_cstr(private_key_file_name), &status);

  if (1 != status) {
    DEBUGF("status=%d", status);
    return throw_error(vm, t, "Invalid socket.");
  }

  map_insert(&data->state, strings_intern("ssl_socket"), ssl_socket);
  return data->object;
}

Element SSLSocket_deconstructor(VM *vm, Thread *t, ExternalData *data,
                                Element *arg) {
  SSLSocket *ssl_socket =
      (SSLSocket *)map_lookup(&data->state, strings_intern("ssl_socket"));
  if (NULL == ssl_socket) {
    return throw_error(vm, t, "Weird SSLSocket error. (deconstructor)");
  }
  sslsocket_close(ssl_socket);
  sslsocket_delete(ssl_socket);
  return create_none();
}

Element SSLSocket_close(VM *vm, Thread *t, ExternalData *data, Element *arg) {
  SSLSocket *ssl_socket =
      (SSLSocket *)map_lookup(&data->state, strings_intern("ssl_socket"));
  if (NULL == ssl_socket) {
    return throw_error(vm, t, "Weird SSLSocket error. (close)");
  }
  sslsocket_close(ssl_socket);
  return create_none();
}

// To ease finding sockethandle class.
Element SSLSocket_accept(VM *vm, Thread *t, ExternalData *data, Element *arg) {
  SSLSocket *ssl_socket =
      (SSLSocket *)map_lookup(&data->state, strings_intern("ssl_socket"));
  if (NULL == ssl_socket) {
    return throw_error(vm, t, "Weird SSLSocket error. (accept)");
  }
  Element sslsocket_handle = create_external_obj(vm, ssl_sh_class);
  SSLSocketHandle_constructor(vm, t, sslsocket_handle.obj->external_data,
                              &data->object);
  return sslsocket_handle;
}

Element SSLSocketHandle_constructor(VM *vm, Thread *t, ExternalData *data,
                                    Element *arg) {
  SSLSocket *ssl_socket = (SSLSocket *)map_lookup(
      &arg->obj->external_data->state, strings_intern("ssl_socket"));
  if (NULL == ssl_socket) {
    return throw_error(vm, t, "Weird SSLSocketHandle error. (constructor)");
  }
  SSLSocketHandle *sh = sslsocket_accept(ssl_socket);
  if (NULL == sh) {
    return throw_error(vm, t, "SSLSocket.accept() failure.");
  }
  map_insert(&data->state, strings_intern("ssl_handle"), sh);
  return data->object;
}

Element SSLSocketHandle_deconstructor(VM *vm, Thread *t, ExternalData *data,
                                      Element *arg) {
  SSLSocketHandle *ssl_sh =
      (SSLSocketHandle *)map_lookup(&data->state, strings_intern("ssl_handle"));
  if (NULL == ssl_sh) {
    return create_none();
    // return throw_error(vm, t, "Weird SSLSocketHandle error.
    // (deconstructor)");
  }
  sslsockethandle_close(ssl_sh);
  sslsockethandle_delete(ssl_sh);
  return create_none();
}

Element SSLSocketHandle_close(VM *vm, Thread *t, ExternalData *data,
                              Element *arg) {
  SSLSocketHandle *ssl_sh =
      (SSLSocketHandle *)map_lookup(&data->state, strings_intern("ssl_handle"));
  if (NULL == ssl_sh) {
    return throw_error(vm, t, "Weird SSLSocketHandle error. (close)");
  }
  sslsockethandle_close(ssl_sh);
  return create_none();
}

Element SSLSocketHandle_send(VM *vm, Thread *t, ExternalData *data,
                             Element *arg) {
  SSLSocketHandle *ssl_sh =
      (SSLSocketHandle *)map_lookup(&data->state, strings_intern("ssl_handle"));
  if (NULL == ssl_sh) {
    return throw_error(vm, t, "Weird SSLSocketHandle error. (send)");
  }
  if (ISTYPE(*arg, class_string)) {
    String *msg = String_extract(*arg);
    return create_int(
        sslsockethandle_send(ssl_sh, String_cstr(msg), String_size(msg)));
  } else if (ISTYPE(*arg, class_array)) {
    Array *arr = extract_array(*arg);
    int i, arr_len = Array_size(arr), bytes_sent = 0;
    for (i = 0; i < arr_len; ++i) {
      String *msg = String_extract(Array_get(arr, i));
      bytes_sent +=
          sslsockethandle_send(ssl_sh, String_cstr(msg), String_size(msg));
    }
    return create_int(bytes_sent);
  } else {
    return throw_error(vm, t, "Cannot send non-string.");
  }
}

Element SSLSocketHandle_receive(VM *vm, Thread *t, ExternalData *data,
                                Element *arg) {
  SSLSocketHandle *ssl_sh =
      (SSLSocketHandle *)map_lookup(&data->state, strings_intern("ssl_handle"));
  if (NULL == ssl_sh) {
    return throw_error(vm, t, "Weird SSLSocketHandle error. (receive)");
  }
  char buf[BUFFER_SIZE];
  int chars_received = sslsockethandle_receive(ssl_sh, buf, BUFFER_SIZE);
  return string_create_len(vm, buf, chars_received);
}

Element add_sslsockethandle_class(VM *vm, Element *module) {
  ssl_sh_class = create_external_class(
      vm, *module, strings_intern("SSLSocketHandle"),
      SSLSocketHandle_constructor, SSLSocketHandle_deconstructor);
  add_external_method(vm, ssl_sh_class, strings_intern("send"),
                      SSLSocketHandle_send);
  add_external_method(vm, ssl_sh_class, strings_intern("receive"),
                      SSLSocketHandle_receive);
  add_external_method(vm, ssl_sh_class, strings_intern("close"),
                      SSLSocketHandle_close);
  return ssl_sh_class;
}

Element add_sslsocket_class(VM *vm, Element *module) {
  Element ssl_socket_class =
      create_external_class(vm, *module, strings_intern("SSLSocket"),
                            SSLSocket_constructor, SSLSocket_deconstructor);
  add_external_method(vm, ssl_socket_class, strings_intern("accept"),
                      SSLSocket_accept);
  add_external_method(vm, ssl_socket_class, strings_intern("close"),
                      SSLSocket_close);
  return ssl_socket_class;
}
