/*
 * file_load.h
 *
 *  Created on: Jun 17, 2017
 *      Author: Jeff
 */

#ifndef FILE_LOAD_H_
#define FILE_LOAD_H_

#include "command/commandline.h"
#include "module.h"

char *guess_file_extension(const char dir[], const char file_prefix[]);

Module *load_fn(const char fn[], const ArgStore *store);

#endif /* FILE_LOAD_H_ */
