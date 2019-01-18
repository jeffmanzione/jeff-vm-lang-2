/*
 * error.h
 *
 *  Created on: Sep 28, 2016
 *      Author: Jeff
 */

#ifndef ERROR_H_
#define ERROR_H_

#include <stdbool.h> // Do not remove
#include <stdlib.h> // Do not remove
#include <stdio.h>

extern bool NEST_DEBUG__;
extern int LINE_NUM__;
extern const char *FUNC_NAME__;
extern const char *FILE_NAME__;

#ifdef DEBUG
#define DEB_FN(ret, fn, ...)           \
	ret fn##__(int LINE_NUM__,           \
		         const char FUNC_NAME__[], \
	           const char FILE_NAME__[], \
		         bool NEST_DEBUG__,        \
	           __VA_ARGS__)
#define CALL_FN(fn, ...) \
    fn(__LINE__, __func__, __FILE__, true, __VA_ARGS__)
#else
#define DEB_FN(ret, fn, ...) \
	ret fn##__(__VA_ARGS__)
#define CALL_FN(fn, ...) \
	fn(__VA_ARGS__)
#endif

#ifdef DEBUG
#define DEBUGF(fmt, ...) debugf(__LINE__, __func__, __FILE__, fmt, ##__VA_ARGS__)
#else
#define DEBUGF(fmt, ...) do {} while(0);
#endif

#define ERROR(fmt, ...) do {                                   \
    if (NEST_DEBUG__) {                                        \
      error_nest(__LINE__, __func__, __FILE__,                 \
                     LINE_NUM__, FUNC_NAME__, FILE_NAME__,     \
             fmt, ##__VA_ARGS__);                              \
    } else {                                                   \
      error(__LINE__, __func__, __FILE__, fmt, ##__VA_ARGS__); \
    }                                                          \
  } while (0)

#define GET_MACRO(_1,_2,_3,_4,NAME,...) NAME
#define ASSERT2(exp1, exp2) ASSERT1(exp1); ASSERT1(exp2)
#define ASSERT3(exp1, exp2, exp3) ASSERT1(exp1); ASSERT2(exp2, exp3)
#define ASSERT4(exp1, exp2, exp3, exp4) ASSERT2(exp1, exp2); ASSERT2(exp3, exp4)
#define S_ASSERT(...) GET_MACRO(__VA_ARGS__, ASSERT4, ASSERT3, ASSERT2, ASSERT1)(__VA_ARGS__)
#define IS_NULL(exp) (NULL == (exp))
#define NOT_NULL(exp) (NULL != (exp))
#define ASSERT_NOT_NULL(exp) ASSERT(NOT_NULL(exp))
#define ASSERT_NULL(exp) ASSERT(IS_NULL(exp))

#ifdef DEBUG
#define ASSERT(...) S_ASSERT(__VA_ARGS__)
#define ASSERT1(exp) do { if (!(exp)) {ERROR("Assertion failed. Expression( %s ) evaluated to false.", #exp);} } while (0)
#else
#define ASSERT(...)
#define ASSERT1(exp) {(void)(exp);}
#endif

void debugf(int line_num, const char func_name[], const char file_name[],
		const char fmt[], ...);
void error(int line_num, const char func_name[], const char file_name[],
		const char fmr[], ...);
void error_nest(int line_num, const char func_name[], const char file_name[],
		int nested_line_num, const char nested_func_name[],
		const char nested_file_name[], const char fmr[], ...);

#endif /* ERROR_H_ */
