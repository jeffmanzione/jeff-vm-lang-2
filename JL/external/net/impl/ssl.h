/*
 * ssl.h
 *
 *  Created on: May 10, 2020
 *      Author: Jeff
 */

#ifndef EXTERNAL_NET_IMPL_SSL_H_
#define EXTERNAL_NET_IMPL_SSL_H_

#include <stdint.h>

#include "socket.h"

#define DEFAULT_HTTP_PORT 80
#define DEFAULT_HTTPS_PORT 443

typedef struct SSLSocket_ SSLSocket;

void ssl_init();

void ssl_cleanup();

SSLSocket *sslsocket_create(short namespace, ulong style, uint16_t port,
                            int protocol);

void sslsocket_delete(SSLSocket *socket);

#endif /* EXTERNAL_NET_IMPL_SSL_H_ */
