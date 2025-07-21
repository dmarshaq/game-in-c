#ifndef STR_H
#define STR_H

/**
 * String.
 */

#include "core/core.h"
#include "core/type.h"
#include <string.h>
#include <stdbool.h>


typedef struct string {
    s64 length;
    char *data;
} String;

#define STR(length, data)   ((String) { length, data } )
#define CSTR(cstring)       STR(strlen(cstring), cstring)

#define STR_BUFFER(literal) STR(sizeof(literal) - 1, (char[]){literal})

#define UNPACK(str)         str.length, str.data



/**
 * Returns String that points to the memory of original "str" at index "start" with length up until index "end".
 * Note: Character at index "end" is not included in the returned String, domain for resulting substring is always [ start, end ).
 * @Important: DOESN'T COPY MEMORY. If "str" memory is freed later, returned string will not point to valid adress anymore.
 */
String str_substring(String str, s64 start, s64 end);

/**
 * Compares "str1" and "str2", checks for lengths equality first and then compares symbol by symbol.
 * Returns true if strings are identical.
 */
bool str_equals(String str1, String str2);

/**
 * Linearly searches for the first occurnse of "search" in "str" from the LEFT, by comparing them through "str_equals()" function.
 * Returns the index of first character of the occurns, otherwise, returns -1.
 */
s64 str_find(String str, String search);

/**
 * Linearly searches for the first occurnse of char "symbol" in "str" from the LEFT, by comparing each char in "str".
 * Returns the index of first character of the occurns, otherwise, returns -1.
 */
s64 str_find_char_left(String str, char symbol);

/**
 * Linearly searches for the first occurnse of char "symbol" in "str" from the RIGHT, by comparing each char in "str".
 * Returns the index of first character of the occurns, otherwise, returns -1.
 */
s64 str_find_char_right(String str, char symbol);

s64 str_find_non_whitespace_left(String str);

s64 str_find_non_whitespace_right(String str);

s64 str_find_whitespace_left(String str);

s64 str_find_whitespace_right(String str);

String str_eat_chars(String str, s64 count);

String str_eat_spaces(String str);

String str_eat_until_space(String str);

String str_get_until_space(String str);

bool str_is_int(String str);

bool str_is_float(String str);

s64 str_parse_int(String str);

float str_parse_float(String str);

/**
 * Counts the number of specific characters in the string.
 */
s64 str_count_chars(String str, char c);

/**
 * Copies contents of the src string into dest string.
 * Doesn't resize destination string.
 * @Important: dest.length >= src.length must be true!
 */
void str_copy(String src, String dest);

/**
 * Copies contents of the src string into buffer.
 * @Important: buffer should have enough space to hold src data!
 */
void str_copy_buffer(String src, void *buffer);





#endif
