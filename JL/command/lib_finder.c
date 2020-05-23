/*
 * lib_finder.c
 *
 *  Created on: May 22, 2020
 *      Author: Jeff
 */

#include "lib_finder.h"

#include <stdlib.h>

#include "../arena/strings.h"

#define DEFAULT_PATH_TO_LIB "./lib/bin/"

char *path_to_libs() {
  char *path_var = getenv(PATH_ENV_VAR_NAME);
  if (NULL == path_var) {
    path_var = DEFAULT_PATH_TO_LIB;
  }
  return strings_intern(path_var);
}
