#ifndef LEXER_H
#define LEXER_H

#include "core/str.h"
#include <stdbool.h>



typedef enum token_type {
    TOKEN_ZERO,
    TOKEN_UNKNOWN,
    TOKEN_SYMBOL,
    TOKEN_STRING,
    TOKEN_COMMENT,
    TOKEN_PREPROC,
    TOKEN_NUMBER,
    

    // Literal tokens.
    TOKEN_SEMICOLON,
    TOKEN_PARAN_OPEN,
    TOKEN_PARAN_CLOSE,
    TOKEN_CURLY_OPEN,
    TOKEN_CURLY_CLOSE,
    TOKEN_SQR_BRACES_OPEN,
    TOKEN_SQR_BRACES_CLOSE,
    TOKEN_COMMA,
    TOKEN_DOT,
    TOKEN_ARROW,
    TOKEN_ASSIGN,
    TOKEN_ASTERISK,
    TOKEN_METANOTE,

} Token_Type;

typedef struct token {
    Token_Type type;
    String str;
    u64 line_num;
} Token;

typedef struct literal_token {
    Token_Type type;
    String literal;
} Literal_Token;

typedef struct lexer {
    u64 cursor;
    u64 bol;            // BOL -> Beginning Of Line
    u64 line_num;
    String content;
} Lexer;

void token_print(Token *token);

void lexer_init(Lexer *lexer, String content);

Token lexer_next_token(Lexer *l);

#endif

