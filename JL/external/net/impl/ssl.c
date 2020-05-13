/*
 * ssl.c
 *
 *  Created on: May 10, 2020
 *      Author: Jeff
 */

#include "ssl.h"

#include <stddef.h>

struct SSLSocket_ {
  Socket *raw_socket;
};

void ssl_init() { InitializeSSL(); }

void ssl_cleanup() { ShutdownSSL(); }

SSLSocket *sslsocket_create(short namespace, ulong style, uint16_t port,
                            int protocol) {
  return NULL;
}

void sslsocket_delete(SSLSocket *socket) {}
