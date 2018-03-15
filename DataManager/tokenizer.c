/*
 * tokenizer.c
 *
 *  Created on: Jan 6, 2016
 *      Author: Jeff
 */

#include "tokenizer.h"

#include <stdbool.h>
#include <string.h>

#include "arena.h"
#include "error.h"
#include "memory.h"
#include "shared.h"
#include "strings.h"

char *keyword_to_type[] = { "if", "then", "else", "def", "field", "class",
    "while", "for", "break", "return", "as", "is",
//    "Nil", "True", "False",
    "import", "module" };

struct FileInfo_ {
  char *name;
  FILE *fp;
  LineInfo **lines;
  int num_lines;
  int array_len;
};

LineInfo *line_info(FileInfo *fi, char line_text[], int line_num) {
  LineInfo *li = ALLOC(LineInfo);
  li->line_text = strings_intern(line_text);
  li->line_num = line_num;
  return li;
}

void line_info_delete(LineInfo *li) {
  DEALLOC(li);
}

FileInfo *file_info(const char fn[]) {
  FILE *file = FILE_FN(fn, "r");
  FileInfo *fi = file_info_file(file);
  file_info_set_name(fi, fn);
  return fi;
}

void file_info_set_name(FileInfo *fi, const char fn[]) {
  ASSERT(NOT_NULL(fi), NOT_NULL(fn));
  fi->name = strings_intern(fn);
}

FileInfo *file_info_file(FILE *tmp_file) {
  FileInfo *fi = ALLOC(FileInfo);
  fi->name = NULL;
  fi->fp = tmp_file;
  ASSERT_NOT_NULL(fi->fp);
  fi->num_lines = 0;
  fi->array_len = DEFAULT_NUM_LINES;
  fi->lines = ALLOC_ARRAY(LineInfo *, fi->array_len);
  return fi;
}

void file_info_delete(FileInfo *fi) {
  ASSERT_NOT_NULL(fi);
  ASSERT_NOT_NULL(fi->lines);
  ASSERT_NOT_NULL(fi->fp);
  int i;
  for (i = 0; i < fi->num_lines; i++) {
    line_info_delete(fi->lines[i]);
  }
  DEALLOC(fi->lines);
  fclose(fi->fp);
  DEALLOC(fi);
}

void file_info_append(FileInfo *fi, char line_text[]) {
  int num_lines = fi->num_lines;
  fi->lines[fi->num_lines++] = line_info(fi, line_text, num_lines);
  if (fi->num_lines >= fi->array_len) {
    fi->array_len += DEFAULT_NUM_LINES;
    fi->lines = REALLOC(fi->lines, LineInfo *, fi->array_len);
  }

}

const LineInfo *file_info_lookup(FileInfo *fi, int line_num) {
  if (line_num < 1 || line_num > fi->num_lines) {
    return NULL;
  }
  return fi->lines[line_num - 1];
}

void token_fill(Token *tok, TokenType type, int line, int col,
    const char text[]) {
  tok->type = type;
  tok->line = line;
  tok->col = col;
  tok->len = strlen(text);
  tok->text = strings_intern(text);
}

Token *token_create(TokenType type, int line, int col, const char text[]) {
  Token *tok = ARENA_ALLOC(Token);
  token_fill(tok, type, line, col, text);
  return tok;
}

void token_delete(Token *token) {
  ASSERT_NOT_NULL(token);
//  DEALLOC(token);
}

Token *token_copy(Token *tok) {
  return token_create(tok->type, tok->line, tok->col, tok->text);
}

bool is_special_char(const char c) {
  switch (c) {
  case '(':
  case ')':
  case '{':
  case '}':
  case '[':
  case ']':
  case '+':
  case '-':
  case '*':
  case '/':
  case '\\':
  case '%':
  case '&':
  case '|':
  case '^':
  case '~':
  case '!':
  case '?':
  case '@':
  case '#':
  case '<':
  case '>':
  case '=':
  case ',':
  case ':':
  case '.':
  case '\'':
    return true;
  default:
    return false;
  }
}

bool is_numeric(const char c) {
  return ('0' <= c && '9' >= c);
}

bool is_number(const char c) {
  return is_numeric(c) || '.' == c;
}

bool is_alphabetic(const char c) {
  return ('A' <= c && 'Z' >= c) || ('a' <= c && 'z' >= c);
}

bool is_alphanumeric(const char c) {
  return is_numeric(c) || is_alphabetic(c) || '_' == c || '$' == c;
}

bool is_whitespace(const char c) {
  return ' ' == c || '\t' == c;
}

void read_word_from_stream(FILE *stream, char *buff) {
  int i = 0;
  while ('\0' != (buff[i++] = fgetc(stream)))
    ;
}

void advance_to_next(char **ptr, const char c) {
  while (c != **ptr) {
    (*ptr)++;
  }
}

// Ex: class Doge:fields{age,breed,}methods{new(2),speak(0),}
void fill_str(char buff[], char *start, char *end) {
  buff[0] = '\0';
  strncat(buff, start, end - start);
}

char char_unesc(char u) {
  switch (u) {
  case 'a':
    return '\a';
  case 'b':
    return '\b';
  case 'f':
    return '\f';
  case 'n':
    return '\n';
  case 'r':
    return '\r';
  case 't':
    return '\t';
  case 'v':
    return '\v';
  case '\\':
    return '\\';
  case '\'':
    return '\'';
  case '\"':
    return '\"';
  case '\?':
    return '\?';
  default:
    return u;
  }
}

TokenType word_type(const char word[], int word_len) {
  int i;
  for (i = 0; i < sizeof(keyword_to_type) / sizeof(keyword_to_type[0]); i++) {
    if (word_len != strlen(keyword_to_type[i])) {
      continue;
    }
    if (0 == strncmp(word, keyword_to_type[i], word_len)) {
      return i + 1000;
    }
  }
  return WORD;
}

TokenType resolve_type(const char word[], int word_len, int *in_comment) {
  TokenType type;
  if (0 == word_len) {
    type = ENDLINE;
    *in_comment = false;
  } else {
    switch (word[0]) {
    case '(':
      type = LPAREN;
      break;
    case ')':
      type = RPAREN;
      break;
    case '{':
      type = LBRCE;
      break;
    case '}':
      type = RBRCE;
      break;
    case '[':
      type = LBRAC;
      break;
    case ']':
      type = RBRAC;
      break;
    case '+':
      type = word[1] == '+' ? INC : PLUS;
      break;
    case '-':
      type = word[1] == '>' ? RARROW : (word[1] == '-' ? DEC : MINUS);
      break;
    case '*':
      type = STAR;
      break;
    case '/':
      type = FSLASH;
      break;
    case '\\':
      type = BSLASH;
      break;
    case '%':
      type = PERCENT;
      break;
    case '&':
      type = AMPER;
      break;
    case '|':
      type = PIPE;
      break;
    case '^':
      type = CARET;
      break;
    case '~':
      type = TILDE;
      break;
    case '!':
      type = word[1] == '=' ? NEQUIV : EXCLAIM;
      break;
    case '?':
      type = QUESTION;
      break;
    case '@':
      type = AT;
      break;
    case '#':
      type = POUND;
      break;
    case '<':
      type = word[1] == '-' ? LARROW : (word[1] == '=' ? LTHANEQ : LTHAN);
      break;
    case '>':
      type = word[1] == '=' ? GTHANEQ : GTHAN;
      break;
    case '=':
      type = word[1] == '=' ? EQUIV : EQUALS;
      break;
    case ',':
      type = COMMA;
      break;
    case ':':
      type = COLON;
      break;
    case '.':
      type = PERIOD;
      break;
    case '\'':
      type = STR;
      break;
    case CODE_COMMENT_CH:
      type = SEMICOLON;
      *in_comment = true;
      break;
    default:
      if (is_number(word[0])) {
        if (ends_with(word, "f") || contains_char(word, '.')) {
          type = FLOATING;
        } else {
          type = INTEGER;
        }
      } else {
        type = word_type(word, word_len);
      }
    }
  }
  return type;
}

int is_start_complex(const char c) {
  switch (c) {
  case '+':
  case '-':
  case '<':
  case '>':
  case '=':
  case '!':
    return true;
  default:
    return false;
  }
}

int is_second_complex(const char c) {
  switch (c) {
  case '+':
  case '-':
  case '>':
  case '<':
  case '=':
    return true;
  default:
    return false;
  }
}

int read_word(char **ptr, char word[], int *word_len) {
  char *index = *ptr, *tmp;
  int wrd_ln = 0;

  if (0 == index[0]) {
    word[0] = '\0';
    *word_len = 0;
    tmp = *ptr;
    return 0;
  }

  while (is_whitespace(index[0])) {
    index++;
  }

  if ((is_special_char(index[0]) || CODE_COMMENT_CH == index[0])) {
    if (is_start_complex(index[0]) && is_second_complex(index[1])) {
      word[wrd_ln++] = index[0];
      index++;
    }
    word[wrd_ln++] = index[0];
    index++;
  } else {
    if ('\n' != index[0]) {
      word[wrd_ln++] = index[0];
      index++;

      if (is_alphanumeric(word[0])) {
        if (is_number(word[0])) {
          while (is_number(index[0]) || 'f' == index[0]) {
            word[wrd_ln++] = index[0];
            index++;
          }
        } else {
          while (is_alphanumeric(index[0])) {
            word[wrd_ln++] = index[0];
            index++;
          }
        }
      }
    }
  }

  word[wrd_ln] = '\0';
  *word_len = wrd_ln;
  tmp = *ptr;
  *ptr = index;

//printf("\t'%s' %d %d\n", word, wrd_ln, *ptr - tmp);

  return *ptr - tmp;
}

int read_string(const char line[], char **index, char *word,
bool escape_characters) {
  int word_i = 1;
  word[0] = '\'';
  while ('\'' != **index) {
    if ('\\' == **index && escape_characters) {
      word[word_i++] = char_unesc(*++*index);
    } else {
      word[word_i++] = **index;
    }
    (*index)++;
  }
  word[word_i++] = **index;
  (*index)++;
  word[word_i] = '\0';
  return word_i;
}

bool tokenize_line(int *line_num, FileInfo *fi, Queue *queue,
bool escape_characters) {
  char line[MAX_LINE_LEN];
  char word[MAX_LINE_LEN];
  char *index;
  int in_comment = false;
  TokenType type;
  int col_num = 0, word_len = 0, chars_consumed;

  if (NULL == fgets(line, MAX_LINE_LEN, fi->fp)) {
    return false;
  }

  col_num = 1;
  index = line;

  file_info_append(fi, line);

  while (true) {
    chars_consumed = read_word(&index, word, &word_len);

    type = resolve_type(word, word_len, &in_comment);

    col_num += chars_consumed;

    if (in_comment) {
      continue;
    }

    if (STR == type) {
      read_string(line, &index, word, escape_characters);
    }

    if (ENDLINE == type && queue->size > 0 && queue_last(queue) != NULL
        && ((Token *) queue_last(queue))->type == BSLASH) {
      Token *bslash = queue_last(queue);
      queue_remove_elt(queue, bslash);
      token_delete(bslash);
      tokenize_line(line_num, fi, queue, escape_characters);
    } else if (ENDLINE != type
        || (queue->size != 0 && ENDLINE != ((Token *) queue_last(queue))->type)) {
      queue_add(queue,
          token_create(type, *line_num, col_num - strlen(word), word));
    }

    if (0 == chars_consumed) {
      break;
    }
  }
  (*line_num)++;
  return true;
}

void tokenize(FileInfo *fi, Queue *queue, bool escape_characters) {
  ASSERT_NOT_NULL(queue);
  ASSERT_NOT_NULL(fi);
  int line_num = 1;
  while (tokenize_line(&line_num, fi, queue, escape_characters)) {
  }
}
