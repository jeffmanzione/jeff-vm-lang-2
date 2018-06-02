/*
 * tokenizer.h
 *
 *  Created on: Jan 6, 2016
 *      Author: Jeff
 */

#ifndef TOKENIZER_H_
#define TOKENIZER_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "../datastructure/queue.h"

#define CODE_DELIM      " \t"
#define CODE_COMMENT_CH ';'
#define DEFAULT_NUM_LINES   128
#define MAX_LINE_LEN 1000
#define ID_SZ 256

typedef enum {
  WORD, INTEGER, FLOATING,

  // For strings.
  STR,

  // Structural symbols
  LPAREN,
  RPAREN,
  LBRCE,
  RBRCE,
  LBRAC,
  RBRAC,

  // Math symbols
  PLUS,
  MINUS,
  STAR,
  FSLASH,
  BSLASH,
  PERCENT,

  // Binary/bool symbols
  AMPER,
  PIPE,
  CARET,
  TILDE,

  // Following not used yet
  EXCLAIM,
  QUESTION,
  AT,
  POUND,

  // Equivalence
  LTHAN,
  GTHAN,
  EQUALS,

  // Others
  ENDLINE,
  SEMICOLON,
  COMMA,
  COLON,
  PERIOD,

  // Specials
  LARROW,
  RARROW,
  INCREMENT,
  DECREMENT,
  LTHANEQ,
  GTHANEQ,
  EQUIV,
  NEQUIV,

  // Words
  IF_T = 1000,
  THEN,
  ELSE,
  DEF,
  NEW,
  FIELD,
  CLASS,
  WHILE,
  FOR,
  BREAK,
  RETURN,
  AS,
  IS_T,
//  NAN,
//  TRUE,
//  FALSE,
  IMPORT,
  MODULE_T,
} TokenType;

typedef struct {
  TokenType type;
  int col, line;
  size_t len;
  const char *text;
} Token;

typedef struct LineInfo_ {
  char *line_text;
  Token *tokens; // Sparse array of tokens corresponding to words
  int line_num;
} LineInfo;

typedef struct FileInfo_ FileInfo;

FileInfo *file_info(const char fn[]);
FileInfo *file_info_file(FILE *tmp_file);
void file_info_set_name(FileInfo *fi, const char fn[]);
LineInfo *file_info_append(FileInfo *fi, char line_text[]);
const LineInfo *file_info_lookup(const FileInfo *fi, int line_num);
int file_info_len(const FileInfo *fi);
const char *file_info_name(const FileInfo *fi);
void file_info_delete(FileInfo *fi);

void tokenize(FileInfo *fi, Queue *queue, bool escape_characters);
bool tokenize_line(int *line, FileInfo *fi, Queue *queue,
bool escape_characters);
void token_fill(Token *tok, TokenType type, int line, int col,
    const char text[]);
Token *token_create(TokenType type, int line, int col, const char text[]);
Token *token_copy(Token *tok);
void token_delete(Token *tok);

#endif /* TOKENIZER_H_ */
