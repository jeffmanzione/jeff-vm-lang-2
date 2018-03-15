/*
 * shared.c
 *
 *  Created on: Jun 17, 2017
 *      Author: Jeff
 */

#include "shared.h"

#include <stddef.h>
#include <string.h>

#include "error.h"
#include "strings.h"

#ifdef DEBUG
bool DBG = true;
#else
bool DBG = false;
#endif

FILE *file_fn(const char fn[], const char op_type[], int line_num,
    const char func_name[], const char file_name[]) {
  if (NULL == fn) {
    error(line_num, func_name, file_name, "Unable to read file.");
  }
  if (NULL == op_type) {
    error(line_num, func_name, file_name, "Unable to read file '%s'.", fn);
  }
  FILE *file = fopen(fn, op_type);
  if (NULL == file) {
    error(line_num, func_name, file_name,
        "Unable to read file '%s' with op '%s'.", fn, op_type);
  }
  return file;
}

void file_op(FILE *file, FileHandler operation, int line_num,
    const char func_name[], const char file_name[]) {
  operation(file);
  if (0 != fclose(file)) {
    error(line_num, func_name, file_name, "File operation failed.");
  }
}

bool ends_with(const char *str, const char *suffix) {
  if (!str || !suffix) {
    return false;
  }
  size_t lenstr = strlen(str);
  size_t lensuffix = strlen(suffix);
  if (lensuffix > lenstr) {
    return false;
  }
  return 0 == strncmp(str + lenstr - lensuffix, suffix, lensuffix);
}

bool starts_with(const char *str, const char *prefix) {
  if (!str || !prefix) {
    return false;
  }
  size_t lenstr = strlen(str);
  size_t lenprefix = strlen(prefix);
  if (lenprefix > lenstr) {
    return false;
  }
  return 0 == strncmp(str, prefix, lenprefix);
}

void strcrepl(char *src, char from, char to) {
  int i;
  for (i = 0; i < strlen(src); i++) {
    if (from == src[i]) {
      src[i] = to;
    }
  }
}

bool contains_char(const char str[], char c) {
  int i;
  for (i = 0; i < strlen(str); i++) {
    if (c == str[i]) {
      return true;
    }
  }
  return false;
}

uint32_t default_hasher(const void *ptr) {
  return (uint32_t) ptr;
}

int32_t default_comparator(const void *ptr1, const void *ptr2) {
  return ptr1 - ptr2;
}

uint32_t string_hasher(const void *ptr) {
  unsigned char *s = (unsigned char *) ptr;
  uint32_t hval = FNV_1A_32_OFFSET;
  while (*s) {
    hval *= FNV_32_PRIME;
    hval ^= (uint32_t) *s++;
  }
  return hval;
}

#ifndef OLD_STRCMP
int32_t string_comparator(const void *ptr1, const void *ptr2) {
  if (ptr1 == ptr2) {
    return 0;
  }
  if (NULL == ptr1) {
    return -1;
  }
  if (NULL == ptr2) {
    return 1;
  }
  uint32_t *lhs = (uint32_t *) ptr1;
  uint32_t *rhs = (uint32_t *) ptr2;

  while (!HAS_NULL(*lhs) && !HAS_NULL(*rhs)) {
    uint32_t diff = *lhs - *rhs;
    if (diff) {
      return diff;
    }
    lhs++;
    rhs++;
  }
  return strncmp((char *) lhs, (char *) rhs, sizeof(uint32_t));
}
#endif

#ifdef OLD_STRCMP
int32_t string_comparator(const void *ptr1, const void *ptr2) {
  if (ptr1 == ptr2) {
    return 0;
  }
  if (NULL == ptr1) {
    return -1;
  }
  if (NULL == ptr2) {
    return 1;
  }

  return strcmp((char *) ptr1, (char *) ptr2);
}
#endif
