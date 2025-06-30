#include "core/str.h"
#include "core/core.h"
#include "core/type.h"
#include <string.h>
#include <stdbool.h>


String str_substring(String str, s64 start, s64 end) {
    return STR(end - start, str.data + start);
}

bool str_equals(String str1, String str2) {
    if (str1.length != str2.length) {
        return false;
    }
    return !memcmp(str1.data, str2.data, str1.length);
}

s64 str_find(String str, String search) {
    String substr;
    for (s64 i = 0; i + search.length <= str.length; i++) {
        if (str.data[i] == search.data[0]) {
            substr = str_substring(str, i, i + search.length);
            if (str_equals(substr, search)) {
                return i;
            }
        }
    }
    
    return -1;
}

s64 str_find_char_left(String str, char symbol) {
    for (s64 i = 0; i < str.length; i++) {
        if (str.data[i] == symbol) {
            return i;
        }
    }

    return -1;
}

s64 str_find_char_right(String str, char symbol) {
    for (s64 i = str.length - 1; i > -1; i--) {
        if (str.data[i] == symbol) {
            return i;
        }
    }

    return -1;
}

void str_copy(String src, String dest) {
    (void)memcpy(src.data, dest.data, src.length);
}

void str_copy_buffer(String src, void *buffer) {
    (void)memcpy(src.data, buffer, src.length);
}


