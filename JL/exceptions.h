/*
 * exceptions.h
 *
 *  Created on: Jun 8, 2018
 *      Author: Jeff
 */

#ifndef EXCEPTIONS_H_
#define EXCEPTIONS_H_


Element error_create(const char name[]) {
  create_class(strings_intern(name), class_error);
}


#endif /* EXCEPTIONS_H_ */
