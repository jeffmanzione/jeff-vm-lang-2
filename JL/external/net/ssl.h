/*
 * ssl.h
 *
 *  Created on: May 13, 2020
 *      Author: Jeff
 */

#ifndef EXTERNAL_NET_SSL_H_
#define EXTERNAL_NET_SSL_H_

#include "../../element.h"

Element add_sslsockethandle_class(VM *vm, Element *module);
Element add_sslsocket_class(VM *vm, Element *module);

#endif /* EXTERNAL_NET_SSL_H_ */
