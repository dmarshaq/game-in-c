#ifndef STR_H
#define STR_H

/**
 * String.
 */

#include "core/core.h"
#include "core/type.h"
#include <string.h>

/**
 * The idea behind String_8 is to make working with dynamically allocated strings easier.
 * They are NOT null terminated, but are designed to be compatible with regural C strings, if used correctly.
 */
typedef struct string_8 {
    u8  *ptr;
    u32 length;
} String_8;


#define str8_make(cstring)                                 ((String_8) { .ptr = (u8 *)cstring, .length = (u32)strlen(cstring) } )
#define str8_make_ptr(ptr, length)                         ((String_8) { .ptr = (u8 *)ptr, .length = (u32)length } )

/**
 * @Todo: Write description.
 */
String_8 str8_make_allocate(u32 length, Allocator *allocator);

/**
 * Frees previously allocated memory through "str8_make_allocate()".
 * @Important: Only use on strings that were allocated.
 */
void str8_free(String_8 *str, Allocator *allocator);

void str8_memcopy_from_buffer(String_8 *str, void *buffer, u32 length);

void str8_memcopy_into_buffer(String_8 *str, void *buffer, u32 start, u32 end);

/**
 * Returns String_8 that points to the memory of original "str" at index "start" with length up untill index "end".
 * Note: Character at index "end" is not included in the returned String_8, domain for resulting substring is always [ start, end ).
 * @Important: DOESN'T COPY MEMORY. If "str" memory is freed later, returned string will not point to valid adress anymore.
 */
#define str8_substring(ptr_str8, start, end)                    ((String_8) { .ptr = (u8 *)((ptr_str8)->ptr) + (start), .length = (u32)((end) - (start)) } )

bool str8_equals(String_8 *str1, String_8 *str2);

bool str8_equals_string(String_8 *str, char *string);

/**
 * Linearly searches for the first occurnse of "search_str" in "str", by comparing them through "str8_equals_str8()" function.
 * Returns the index of first character of the occurns, otherwise, returns UINT_MAX.
 * "start" refers to the index where searching begins, character at this index IS included in the search comparising.
 * "end" refers to the index where searching stops, character at this index is NOT included in the search comparising.
 */
u32 str8_index_of(String_8 *str, String_8 *search_str, u32 start, u32 end);

/**
 * Linearly searches for the first occurnse of "search_string" in "str", by comparing them through "str8_equals_string()" function.
 * Returns the index of first character of the occurns, otherwise, returns UINT_MAX.
 * "start" refers to the index where searching begins, character at this index IS included in the search comparising.
 * "end" refers to the index where searching stops, character at this index is NOT included in the search comparising.
 */
u32 str8_index_of_string(String_8 *str, char *search_string, u32 start, u32 end);

/**
 * Linearly searches for the first occurnse of char "character" in "str", by comparing each char in "str".
 * Returns the index of first character of the occurns, otherwise, returns UINT_MAX.
 * "start" refers to the index where searching begins, character at this index IS included in the search comparising.
 * "end" refers to the index where searching stops, character at this index is NOT included in the search comparising.
 */
u32 str8_index_of_char(String_8 *str, char character, u32 start, u32 end);

#endif
