/*
 * commandline_arg.h
 *
 *  Created on: Jun 2, 2018
 *      Author: Jeff
 */

#ifndef COMMANDLINE_ARG_H_
#define COMMANDLINE_ARG_H_

#include <stdbool.h>
#include <stdint.h>

typedef enum {
  ArgType__none = 0,
  ArgType__bool,
  ArgType__int,
  ArgType__float,
  ArgType__string,
} ArgType;


typedef struct {
  bool used;
  ArgType type;
  union {
    bool bool_val;
    int32_t int_val;
    float float_val;
    const char *string_val;
  };
} Arg;

Arg arg_parse(ArgType type, const char val[]);

Arg arg_bool(bool bool_val);

Arg arg_int(int32_t int_val);

Arg arg_float(float float_val);

Arg arg_string(const char string_val[]);

#endif /* COMMANDLINE_ARG_H_ */
