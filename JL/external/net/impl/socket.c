/*
 * socket.c
 *
 *  Created on: Jan 27, 2019
 *      Author: Jeff
 */

#include "socket.h"

#include <windows.h>
#include <winsock2.h>

#undef max
#undef min

#include "../../../memory/memory.h"

struct Socket_ {
  struct sockaddr_in in;
  SOCKET sock;
};

struct SocketHandle_ {
  struct sockaddr_in client;
  SOCKET client_sock;
};


void sockets_init() {
  WSADATA wsaData;
  WSAStartup(MAKEWORD(2, 2), &wsaData);
}

void sockets_cleanup() {
  WSACleanup();
}

Socket *socket_create(short namespace, ulong style, uint16_t port, int protocol) {
  Socket *sock = ALLOC2(Socket);
  sock->in.sin_family = namespace;
  sock->in.sin_addr.s_addr = INADDR_ANY;
  sock->in.sin_port = htons(port);
  sock->sock = socket(namespace, style, protocol);
  return sock;
}

bool socket_is_valid(const Socket *socket) {
  return socket->sock != INVALID_SOCKET ;
}

SocketStatus socket_bind(Socket *socket) {
  return bind(socket->sock, (struct sockaddr *) &socket->in, sizeof(socket->in));
}

SocketStatus socket_listen(Socket *socket, int num_connections) {
  return listen(socket->sock, num_connections);
}

SocketHandle *socket_accept(Socket *socket) {
  SocketHandle *sh = ALLOC2(SocketHandle);
  int addr_len = sizeof(sh->client);
  sh->client_sock = accept(socket->sock, (struct sockaddr *) &sh->client,
      &addr_len);
  return sh;
}

void socket_close(Socket *socket) {
  closesocket(socket->sock);
}

void socket_delete(Socket *socket) {
  DEALLOC(socket);
}

bool sockethandle_is_valid(const SocketHandle *sh) {
  return sh->client_sock != INVALID_SOCKET && sh->client_sock != -1;
}

int32_t sockethandle_send(SocketHandle *sh, const char * const msg, int msg_len) {
  return send(sh->client_sock, msg, msg_len, 0);
}

int32_t sockethandle_receive(SocketHandle *sh, char * buf, int buf_len) {
  return recv(sh->client_sock, buf, buf_len, 0);
}

void sockethandle_close(SocketHandle *sh) {
  closesocket(sh->client_sock);
}

void sockethandle_delete(SocketHandle *sh) {
  DEALLOC(sh);
}
