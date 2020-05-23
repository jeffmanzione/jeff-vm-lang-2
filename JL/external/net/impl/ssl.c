/*
 * ssl.c
 *
 *  Created on: May 10, 2020
 *      Author: Jeff
 */

#include "ssl.h"

#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/ossl_typ.h>
#include <openssl/ssl.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "../../../error.h"
#include "../../../memory/memory.h"
#include "socket.h"

#define SUCCESS 1

struct SSLSocket_ {
  Socket *raw_socket;
  SSL_CTX *ssl_ctx;
};

struct SSLSocketHandle_ {
  SocketHandle *sh;
  SSL *ssl;
};

void ssl_init() {
  SSL_load_error_strings();
  SSL_library_init();
  OpenSSL_add_all_algorithms();
}

void ssl_cleanup() {
  ERR_free_strings();
  EVP_cleanup();
}

SSLSocket *sslsocket_create(Socket *socket, const char certificate_fn[],
                            const char private_key_fn[], int *status) {
  ASSERT(NOT_NULL(socket));
  // Assume socket is valid, bound, and listening.
  SSLSocket *ssl_sock = ALLOC2(SSLSocket);
  ssl_sock->raw_socket = socket;
  // Wrap socket in SSL
  ssl_sock->ssl_ctx = SSL_CTX_new(SSLv23_server_method());
  SSL_CTX_set_options(ssl_sock->ssl_ctx, SSL_OP_SINGLE_DH_USE);
  // Apply cert and private key encryption.
  if (SSL_CTX_use_certificate_file(ssl_sock->ssl_ctx, certificate_fn,
                                   SSL_FILETYPE_PEM) != SUCCESS) {
    *status = ERR_BAD_CERTIFICATE;
    return ssl_sock;
  }
  if (SSL_CTX_use_PrivateKey_file(ssl_sock->ssl_ctx, private_key_fn,
                                  SSL_FILETYPE_PEM) != SUCCESS) {
    *status = ERR_BAD_PRIVATE_KEY;
    return ssl_sock;
  }
  *status = SUCCESS;
  return ssl_sock;
}

SSLSocketHandle *sslsocket_accept(SSLSocket *ssl_socket) {
  ASSERT(NOT_NULL(ssl_socket));
  SSLSocketHandle *ssl_handle = ALLOC2(SSLSocketHandle);
  ssl_handle->sh = socket_accept(ssl_socket->raw_socket);
  ssl_handle->ssl = SSL_new(ssl_socket->ssl_ctx);
  SSL_set_accept_state(ssl_handle->ssl);
  if (!SSL_set_fd(ssl_handle->ssl, sockethandle_get_socket(ssl_handle->sh))) {
    return NULL;
  }
  int accept_status = SSL_accept(ssl_handle->ssl);
  if (accept_status > 0) {
    return ssl_handle;
  }
  //  int err = SSL_get_error(ssl_handle->ssl, accept_status);
  sslsockethandle_close(ssl_handle);
  sslsockethandle_delete(ssl_handle);
  return NULL;
}

int32_t sslsockethandle_send(SSLSocketHandle *ssl_sh, const char *const msg,
                             int msg_len) {
  int bytes_written = 0;
  while (bytes_written < msg_len) {
    int w =
        SSL_write(ssl_sh->ssl, msg + bytes_written, msg_len - bytes_written);
    if (w <= 0) {
      break;
    }
    bytes_written += w;
  }
  return bytes_written;
}

int32_t sslsockethandle_receive(SSLSocketHandle *ssl_sh, char *buf,
                                int buf_len) {
  int total_bytes_read = 0;
  while (total_bytes_read < buf_len) {
    size_t bytes_read = 0;
    int response_value = SSL_read_ex(ssl_sh->ssl, buf + total_bytes_read,
                                     buf_len - total_bytes_read, &bytes_read);
    switch (SSL_get_error(ssl_sh->ssl, response_value)) {
      case SSL_ERROR_NONE:
        return total_bytes_read + bytes_read;
      case SSL_ERROR_ZERO_RETURN:
        // Result was actually empty.
        return total_bytes_read;
      case SSL_ERROR_WANT_READ:
      case SSL_ERROR_WANT_WRITE:
        // Error in read/write, but it was not fatal, we can try again.
        continue;
      default:
        // Error was fatal.
        return total_bytes_read;
    }

    total_bytes_read += bytes_read;
  }
  return total_bytes_read;
}

void sslsocket_close(SSLSocket *ssl_socket) {
  socket_close(ssl_socket->raw_socket);
  SSL_CTX_SRP_CTX_free(ssl_socket->ssl_ctx);
}

void sslsocket_delete(SSLSocket *ssl_socket) { DEALLOC(ssl_socket); }

void sslsockethandle_close(SSLSocketHandle *ssl_sh) {
  SSL_shutdown(ssl_sh->ssl);
  SSL_free(ssl_sh->ssl);
  sockethandle_close(ssl_sh->sh);
}

void sslsockethandle_delete(SSLSocketHandle *ssl_sh) {
  sockethandle_delete(ssl_sh->sh);
  DEALLOC(ssl_sh);
}
