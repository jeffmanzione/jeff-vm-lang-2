/*
 * socket.h
 *
 *  Created on: Jan 27, 2019
 *      Author: Jeff
 */

#ifndef EXTERNAL_NET_IMPL_SOCKET_H_
#define EXTERNAL_NET_IMPL_SOCKET_H_

#include <stdbool.h>
#include <stdint.h>

typedef unsigned long ulong;

typedef struct Socket_ Socket;
typedef struct SocketAddress_ SocketAddress;

typedef int SocketStatus;

typedef struct SocketHandle_ SocketHandle;

void sockets_init();

void sockets_cleanup();

Socket *socket_create(short namespace, ulong style, uint16_t port,
                      int protocol);

bool socket_is_valid(const Socket *socket);

SocketStatus socket_bind(Socket *socket);

SocketStatus socket_listen(Socket *socket, int num_connections);

SocketHandle *socket_accept(Socket *socket);

void socket_close(Socket *socket);

void socket_delete(Socket *socket);

bool sockethandle_is_valid(const SocketHandle *sh);

SocketStatus sockethandle_send(SocketHandle *sh, const char *const msg,
                               int msg_len);
int32_t sockethandle_receive(SocketHandle *sh, char *buf, int buf_len);

unsigned int sockethandle_get_socket(SocketHandle *sh);

void sockethandle_close(SocketHandle *sh);

void sockethandle_delete(SocketHandle *sh);

#endif /* EXTERNAL_NET_IMPL_SOCKET_H_ */
