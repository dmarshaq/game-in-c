#include "meta/lexer.h"
#include "core/type.h"
#include <stdbool.h>
#include <stdio.h>
#include <ctype.h>


void token_print_block(Token *token, bool show_token_type) {
    if (show_token_type) {
        switch (token->type) {
            case (TOKEN_ZERO):
                printf("LEXING_END(");
                break;
            case (TOKEN_UNKNOWN):
                printf("UNKNOWN_TOKEN(");
                break;
            case (TOKEN_SYMBOL):
                printf("SYMBOL(");
                break;
            case (TOKEN_SEMICOLON):
                printf("SEMICOLON(");
                break;
            case (TOKEN_PARAN_OPEN):
                printf("PARENTHESIS_OPEN(");
                break;
            case (TOKEN_PARAN_CLOSE):
                printf("PARENTHESIS_CLOSE(");
                break;
            case (TOKEN_CURLY_OPEN):
                printf("CURLY_BRACES_OPEN(");
                break;
            case (TOKEN_CURLY_CLOSE):
                printf("CURLY_BRACES_CLOSE(");
                break;
            case (TOKEN_SQR_BRACES_OPEN):
                printf("SQUARE_BRACES_OPEN(");
                break;
            case (TOKEN_SQR_BRACES_CLOSE):
                printf("SQUARE_BRACES_CLOSE(");
                break;
            case (TOKEN_STRING):
                printf("STRING(");
                break;
            case (TOKEN_COMMENT):
                printf("COMMENT(");
                break;
            case (TOKEN_PREPROC):
                printf("PREPROCESSOR(");
                break;
            case (TOKEN_NUMBER):
                printf("NUMBER(");
                break;
            case (TOKEN_COMMA):
                printf("COMMA(");
                break;
            case (TOKEN_DOT):
                printf("DOT(");
                break;
            case (TOKEN_ARROW):
                printf("ARROW(");
                break;
            case (TOKEN_ASSIGN):
                printf("ASSIGNMENT(");
                break;
            case (TOKEN_ASTERISK):
                printf("ASTERISK(");
                break;
            case (TOKEN_METANOTE):
                printf("METANOTE(");
                break;
        }
    }
    printf("'%.*s'", token->str.length, token->str.data);
    if (show_token_type) {
        printf(")");
    }
    printf(" ");
}


void token_print(Token *token) {
    printf("%-30.*s", token->str.length, token->str.data);
    switch (token->type) {
        case (TOKEN_ZERO):
            printf("%s\n", "lexing end");
            break;
        case (TOKEN_UNKNOWN):
            printf("%s\n", "unknown token");
            break;
        case (TOKEN_SYMBOL):
            printf("%s\n", "symbol");
            break;
        case (TOKEN_SEMICOLON):
            printf("%s\n", "semicolon");
            break;
        case (TOKEN_PARAN_OPEN):
            printf("%s\n", "parenthesis open");
            break;
        case (TOKEN_PARAN_CLOSE):
            printf("%s\n", "parenthesis close");
            break;
        case (TOKEN_CURLY_OPEN):
            printf("%s\n", "curly braces open");
            break;
        case (TOKEN_CURLY_CLOSE):
            printf("%s\n", "curly braces close");
            break;
        case (TOKEN_SQR_BRACES_OPEN):
            printf("%s\n", "square braces open");
            break;
        case (TOKEN_SQR_BRACES_CLOSE):
            printf("%s\n", "square braces close");
            break;
        case (TOKEN_STRING):
            printf("%s\n", "string");
            break;
        case (TOKEN_COMMENT):
            printf("%s\n", "comment");
            break;
        case (TOKEN_PREPROC):
            printf("%s\n", "preprocessor");
            break;
        case (TOKEN_NUMBER):
            printf("%s\n", "number");
            break;
        case (TOKEN_COMMA):
            printf("%s\n", "comma");
            break;
        case (TOKEN_DOT):
            printf("%s\n", "dot");
            break;
        case (TOKEN_ARROW):
            printf("%s\n", "arrow");
            break;
        case (TOKEN_ASSIGN):
            printf("%s\n", "assignment operator");
            break;
        case (TOKEN_ASTERISK):
            printf("%s\n", "asterisk");
            break;
        case (TOKEN_METANOTE):
            printf("%s\n", "metanote");
            break;
        // case (TOKEN_C_NON_TYPE_KEYWORD):
        //     printf("%s\n", "C keyword");
        //     break;
    }
}


void token_type_print(Token_Type token_type) {
    switch (token_type) {
        case (TOKEN_ZERO):
            printf("TOKEN_ZERO");
            break;
        case (TOKEN_UNKNOWN):
            printf("UNKNOWN_TOKEN");
            break;
        case (TOKEN_SYMBOL):
            printf("SYMBOL");
            break;
        case (TOKEN_SEMICOLON):
            printf("SEMICOLON");
            break;
        case (TOKEN_PARAN_OPEN):
            printf("PARENTHESIS_OPEN");
            break;
        case (TOKEN_PARAN_CLOSE):
            printf("PARENTHESIS_CLOSE");
            break;
        case (TOKEN_CURLY_OPEN):
            printf("CURLY_BRACES_OPEN");
            break;
        case (TOKEN_CURLY_CLOSE):
            printf("CURLY_BRACES_CLOSE");
            break;
        case (TOKEN_SQR_BRACES_OPEN):
            printf("SQUARE_BRACES_OPEN");
            break;
        case (TOKEN_SQR_BRACES_CLOSE):
            printf("SQUARE_BRACES_CLOSE");
            break;
        case (TOKEN_STRING):
            printf("STRING");
            break;
        case (TOKEN_COMMENT):
            printf("COMMENT");
            break;
        case (TOKEN_PREPROC):
            printf("PREPROCESSOR");
            break;
        case (TOKEN_NUMBER):
            printf("NUMBER");
            break;
        case (TOKEN_COMMA):
            printf("COMMA");
            break;
        case (TOKEN_DOT):
            printf("DOT");
            break;
        case (TOKEN_ARROW):
            printf("ARROW");
            break;
        case (TOKEN_ASSIGN):
            printf("ASSIGNMENT");
            break;
        case (TOKEN_ASTERISK):
            printf("ASTERISK");
            break;
        case (TOKEN_METANOTE):
            printf("METANOTE");
            break;
    }
    printf(" ");
}




// static const String C_NON_TYPE_KEYWORDS[] = {
//     // int, char, float, double, void, _Bool, _Complex
//     STR_BUFFER("auto"), STR_BUFFER("static"), STR_BUFFER("extern"), STR_BUFFER("register"), STR_BUFFER("_Thread_local"),
//     STR_BUFFER("const"), STR_BUFFER("volatile"), STR_BUFFER("restrict"), STR_BUFFER("_Atomic"),
//     STR_BUFFER("if"), STR_BUFFER("else"), STR_BUFFER("switch"), STR_BUFFER("case"), STR_BUFFER("for"), STR_BUFFER("while"), STR_BUFFER("do"), STR_BUFFER("goto"), STR_BUFFER("break"), STR_BUFFER("continue"), STR_BUFFER("return"), STR_BUFFER("_Noreturn"),
//     STR_BUFFER("sizeof"), STR_BUFFER("typedef"), STR_BUFFER("struct"), STR_BUFFER("union"), STR_BUFFER("enum"), STR_BUFFER("inline"),
// };
// 
// #define C_KEYWORDS_LENGTH (sizeof(C_NON_TYPE_KEYWORDS) / sizeof(String))


#define LITERAL_TOKENS_LENGTH (sizeof(LITERAL_TOKENS) / sizeof(Literal_Token))

static const Literal_Token LITERAL_TOKENS[] = {
    { TOKEN_SEMICOLON,          STR_BUFFER(";") },
    { TOKEN_PARAN_OPEN,         STR_BUFFER("(") },
    { TOKEN_PARAN_CLOSE,        STR_BUFFER(")") },
    { TOKEN_CURLY_OPEN,         STR_BUFFER("{") },
    { TOKEN_CURLY_CLOSE,        STR_BUFFER("}") },
    { TOKEN_SQR_BRACES_OPEN,    STR_BUFFER("[") },
    { TOKEN_SQR_BRACES_CLOSE,   STR_BUFFER("]") },
    { TOKEN_COMMA,              STR_BUFFER(",") },
    { TOKEN_DOT,                STR_BUFFER(".") },
    { TOKEN_ARROW,              STR_BUFFER("->") },
    { TOKEN_ASSIGN,             STR_BUFFER("=") },
    { TOKEN_ASTERISK,           STR_BUFFER("*") },
};




void lexer_init(Lexer *lexer, String content) {
    lexer->cursor   = 0;
    lexer->bol      = 0;
    lexer->line_num = 1;
    lexer->content  = content;
}





bool valid_symbol_start(char c) {
    return isalpha(c) || c == '_';
}

bool valid_symbol(char c) {
    return isalnum(c) || c == '_';
}

bool valid_number_start(char c) {
    return isdigit(c) || c == '.';
}

bool valid_number(char c) {
    return isalnum(c) || c == '.';
}


// Returns true if newline character was eaten.
bool lexer_eat_char(Lexer *l) {
    l->cursor++;
    if (l->content.data[l->cursor - 1] == '\n') {
        l->bol = l->cursor;
        l->line_num++;
        return true;
    }

    return false;
}

// Returns true if stopped right after after newline character was eaten.
void lexer_consume_spaces(Lexer *l) {

    while (l->cursor < l->content.length && isspace(l->content.data[l->cursor])) {
        lexer_eat_char(l);
    }

}

Token lexer_next_token(Lexer *l) {
    lexer_consume_spaces(l);

    Token token = {
        .type = TOKEN_UNKNOWN,
        .str.data = l->content.data + l->cursor, 
        .str.length = 0,
        .line_num = l->line_num,
    };

    if (l->cursor >= l->content.length) {
        token.type = TOKEN_ZERO;
        return token;
    }


    if (valid_number_start(l->content.data[l->cursor])) {
        if (l->cursor < l->content.length - 1 && l->content.data[l->cursor] == '.' && !isdigit(l->content.data[l->cursor + 1])) {
        } else {
            token.type = TOKEN_NUMBER;
            while (l->cursor < l->content.length && valid_number(l->content.data[l->cursor])) {
                token.str.length++;
                l->cursor++;
            }
            return token;
        }
    }


    for (s64 i = 0; i < LITERAL_TOKENS_LENGTH; i++) {
        if (l->content.length - l->cursor > LITERAL_TOKENS[i].literal.length&& str_equals(LITERAL_TOKENS[i].literal, STR(LITERAL_TOKENS[i].literal.length, l->content.data + l->cursor))) {
            token.type = LITERAL_TOKENS[i].type;
            token.str = STR(LITERAL_TOKENS[i].literal.length, l->content.data + l->cursor);
            l->cursor += token.str.length;
            return token;
        }
    }


    if (l->content.data[l->cursor] == '#') {
        token.type = TOKEN_PREPROC;
        token.str.length++;
        l->cursor++;


        while (l->cursor < l->content.length) {

            if (lexer_eat_char(l)) { 
                // If we got '\n'.
                s64 index_of_last_char = str_find_non_whitespace_right(token.str);
                if (token.str.data[index_of_last_char] != '\\') { 
                    // If not multiline line macro.
                    return token;
                } 
            }
            token.str.length++;
        }

        return token;
    }


    if (l->content.data[l->cursor] == '"') {
        token.type = TOKEN_STRING;
        token.str.length++;
        l->cursor++;

        while (l->cursor < l->content.length) {
            if (l->content.data[l->cursor] == '"' && l->content.data[l->cursor - 1] != '\\') {
                // If a valid string end.
                break;
            }
            token.str.length++;
            l->cursor++;
        }

        token.str.length++;
        l->cursor++;

        return token;
    }


    if (l->content.data[l->cursor] == '/') {
        token.str.length++;
        l->cursor++;
        
        // One line comment.
        if (l->cursor < l->content.length && l->content.data[l->cursor] == '/') {
            token.type = TOKEN_COMMENT;
            token.str.length++;
            l->cursor++;

            while (l->cursor < l->content.length) {

                if (lexer_eat_char(l)) {
                    break;
                }
                token.str.length++;
            }

            return token;
        }

        // Multiline comment.
        if (l->cursor < l->content.length && l->content.data[l->cursor] == '*') {
            token.type = TOKEN_COMMENT;
            token.str.length++;
            l->cursor++;

            while (l->cursor < l->content.length && (l->content.data[l->cursor - 1] != '*' || l->content.data[l->cursor] != '/')) {

                lexer_eat_char(l);
                token.str.length++;
            }


            token.str.length++;
            l->cursor++;

            return token;
        }
        

        return token;
    }


    if (l->content.data[l->cursor] == '@') {
        token.type = TOKEN_METANOTE;
        token.str.length++;
        l->cursor++;

        while (l->cursor < l->content.length && valid_symbol(l->content.data[l->cursor])) {
            token.str.length++;
            l->cursor++;
        }
        
        return token;
    }

    if (valid_symbol_start(l->content.data[l->cursor])) {
        token.type = TOKEN_SYMBOL;
        while (l->cursor < l->content.length && valid_symbol(l->content.data[l->cursor])) {
            token.str.length++;
            l->cursor++;
        }
        
        return token;
    }
    
    token.str.length++;
    l->cursor++;
    return token;
}

