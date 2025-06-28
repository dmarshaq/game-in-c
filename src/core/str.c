#include "core/str.h"
#include "core/core.h"
#include "core/type.h"
#include <string.h>

/**
 * String.
 */

String_8 str8_make_allocate(u32 length, Allocator *allocator) {
    return (String_8) {
        .ptr = allocator_alloc(allocator, length),
        .length = length,
    };
}

void str8_free(String_8 *str, Allocator *allocator) {
    allocator_free(allocator, str->ptr);
    str->ptr = NULL;
    str->length = 0;
}

void str8_memcopy_from_buffer(String_8 *str, void *buffer, u32 length) {
    (void)memcpy(str->ptr, buffer, length);
}

void str8_memcopy_into_buffer(String_8 *str, void *buffer, u32 start, u32 end) {
    (void)memcpy(buffer, str->ptr + start, end - start);
}

bool str8_equals(String_8 *str1, String_8 *str2) {
    if (str1->length != str2->length) {
        return false;
    }
    return !memcmp(str1->ptr, str2->ptr, str1->length);
}

bool str8_equals_string(String_8 *str, char *string) {
    if (str->length != strlen(string)) {
        return false;
    }
    return !memcmp(str->ptr, string, str->length);
}

u32 str8_index_of(String_8 *str, String_8 *search_str, u32 start, u32 end) {
    String_8 substr;
    for (; start < end; start++) {
        if (str->ptr[start] == search_str->ptr[0] && start + search_str->length <= str->length) {
            substr = str8_substring(str, start, start + search_str->length);
            if (str8_equals(&substr, search_str)) {
                return start;
            }
        }
    }
    return UINT_MAX;
}

u32 str8_index_of_string(String_8 *str, char *search_string, u32 start, u32 end) {
    u32 search_string_length = (u32)strlen(search_string);
    String_8 substr;
    for (; start < end; start++) {
        if (str->ptr[start] == search_string[0] && start + search_string_length <= str->length) {
            substr = str8_substring(str,  start, start + search_string_length);
            if (str8_equals_string(&substr, search_string)) {
                return start;
            }
        }
    }
    
    return UINT_MAX;
}

u32 str8_index_of_char(String_8 *str, char character, u32 start, u32 end) {
    for (; start < end; start++) {
        if (str->ptr[start] == character) {
            return start;
        }
    }
    return UINT_MAX;
}
