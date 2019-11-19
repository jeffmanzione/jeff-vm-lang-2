/*
 * shared.c
 *
 *  Created on: Jun 17, 2017
 *      Author: Jeff
 */

#include "shared.h"

#include <stddef.h>
#include <string.h>
#include <sys/types.h>

#include "arena/strings.h"
#include "error.h"
#include "graph/memory.h"

#ifdef _WIN32
#define SLASH_CHAR '\\';
#else
#define SLASH_CHAR '/';
#endif

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

char *find_str(char *haystack, size_t haystack_len, const char *needle,
               size_t needle_len) {
  //  DEBUGF("haystack='%*s'(%d), needle='%*s'(%d)", haystack_len, haystack,
  //      haystack_len, needle_len, needle, needle_len);
  int i, j;
  for (i = 0; i <= (haystack_len - needle_len); ++i) {
    for (j = 0; j < needle_len; ++j) {
      //      DEBUGF("i=%d, j=%d", i, j);
      if (haystack[i + j] != needle[j]) {
        break;
      } else if (j == (needle_len - 1)) {
        //        DEBUGF("found at i=%d", i);
        return haystack + i;
      }
    }
  }
  return NULL;
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

uint32_t default_hasher(const void *ptr) { return (uint32_t)ptr; }

int32_t default_comparator(const void *ptr1, const void *ptr2) {
  return ptr1 - ptr2;
}

uint32_t string_hasher(const void *ptr) {
  unsigned char *s = (unsigned char *)ptr;
  uint32_t hval = FNV_1A_32_OFFSET;
  while (*s) {
    hval *= FNV_32_PRIME;
    hval ^= (uint32_t)*s++;
  }
  return hval;
}

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
  uint32_t *lhs = (uint32_t *)ptr1;
  uint32_t *rhs = (uint32_t *)ptr2;

  while (!HAS_NULL(*lhs) && !HAS_NULL(*rhs)) {
    uint32_t diff = *lhs - *rhs;
    if (diff) {
      return diff;
    }
    lhs++;
    rhs++;
  }
  return strncmp((char *)lhs, (char *)rhs, sizeof(uint32_t));
}

// int getline(char **lineptr, size_t *n, FILE *stream) {
//  static char line[256];
//  char *ptr;
//  unsigned int len;
//
//  if (lineptr == NULL || n == NULL) {
//    errno = EINVAL;
//    return -1;
//  }
//
//  if (ferror(stream))
//    return -1;
//
//  if (feof(stream))
//    return -1;
//
//  fgets(line, 256, stream);
//
//  ptr = strchr(line, '\n');
//  if (ptr)
//    *ptr = '\0';
//
//  len = strlen(line);
//
//  if ((len + 1) < 256) {
//    ptr = realloc(*lineptr, 256);
//    if (ptr == NULL)
//      return (-1);
//    *lineptr = ptr;
//    *n = 256;
//  }
//
//  strcpy(*lineptr, line);
//  return (len);
//}

ssize_t getline(char **lineptr, size_t *n, FILE *stream) {
  char *bufptr = NULL;
  char *p = bufptr;
  size_t size;
  int c;

  if (lineptr == NULL) {
    return -1;
  }
  if (stream == NULL) {
    return -1;
  }
  if (n == NULL) {
    return -1;
  }
  bufptr = *lineptr;
  size = *n;

  c = fgetc(stream);
  if (c == EOF) {
    return -1;
  }
  if (bufptr == NULL) {
    bufptr = ALLOC_ARRAY2(char, 128);
    if (bufptr == NULL) {
      return -1;
    }
    size = 128;
  }
  p = bufptr;
  while (c != EOF) {
    if ((p - bufptr) > (size - 1)) {
      size = size + 128;
      // Handle realloc
      int offset = p - bufptr;
      bufptr = REALLOC(bufptr, char, size);
      if (bufptr == NULL) {
        return -1;
      }
      p = bufptr + offset;
    }
    *(p++) = c;
    if (c == '\n') {
      break;
    }
    c = fgetc(stream);
  }

  *(p++) = '\0';
  *lineptr = bufptr;
  *n = size;

  return p - bufptr - 1;
}

void split_path_file(const char path_file[], char **path, char **file_name,
                     char **ext) {
  char *slash = (char *)path_file, *next;
  while ((next = strpbrk(slash + 1, "\\/"))) slash = next;
  if (path_file != slash) slash++;
  int path_len = slash - path_file;
  *path = strings_intern_range(path_file, 0, path_len);
  char *dot = (ext == NULL) ? NULL : strchr(slash + 1, '.');
  int filename_len = (dot == NULL) ? strlen(path_file) - path_len : dot - slash;
  *file_name = strings_intern_range(slash, 0, filename_len);
  if (dot != NULL) {
    *ext = strings_intern(dot);
  } else if (ext != NULL) {
    *ext = NULL;
  }
}

char *combine_path_file(const char path[], const char file_name[],
                        const char ext[]) {
  int path_len = (ends_with(path, "/") || ends_with(path, "\\"))
                     ? strlen(path) - 1
                     : strlen(path);
  int filename_len = strlen(file_name);
  int ext_len = (NULL == ext) ? 0 : strlen(ext);
  int full_len = path_len + 1 + filename_len + ext_len;
  char *tmp = ALLOC_ARRAY2(char, full_len);
  memmove(tmp, path, path_len);
  tmp[path_len] = SLASH_CHAR;
  memmove(tmp + path_len + 1, file_name, filename_len);
  if (NULL != ext) {
    memmove(tmp + path_len + 1 + filename_len, ext, ext_len);
  }
  char *result = strings_intern_range(tmp, 0, full_len);
  DEALLOC(tmp);
  return result;
}
