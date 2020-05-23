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

#define ERR_BAD_CERTIFICATE 1001
#define ERR_BAD_PRIVATE_KEY 1002

typedef struct SSLSocket_ SSLSocket;
typedef struct SSLSocketHandle_ SSLSocketHandle;

void ssl_init();

void ssl_cleanup();

SSLSocket *sslsocket_create(Socket *socket, const char certificate_fn[],
                            const char private_key_fn[], int *status);

void sslsocket_close(SSLSocket *socket);

void sslsocket_delete(SSLSocket *socket);

SSLSocketHandle *sslsocket_accept(SSLSocket *ssl_socket);

int32_t sslsockethandle_send(SSLSocketHandle *ssl_sh, const char *const msg,
                             int msg_len);

int32_t sslsockethandle_receive(SSLSocketHandle *ssl_sh, char *buf,
                                int buf_len);

void sslsocket_delete(SSLSocket *ssl_socket);

void sslsockethandle_close(SSLSocketHandle *sh);

void sslsockethandle_delete(SSLSocketHandle *sh);

#endif /* EXTERNAL_NET_IMPL_SSL_H_ */
