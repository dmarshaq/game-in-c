#include "core/str.h"
#include "core/core.h"
#include"core/type.h"
#include <ctype.h>
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

s64 str_find_non_whitespace_left(String str) {
    for (s64 i = 0; i < str.length; i++) {
        if (!isspace(str.data[i])) {
            return i;
        }
    }

    return -1;
}

s64 str_find_non_whitespace_right(String str) {
    for (s64 i = str.length - 1; i > -1; i--) {
        if (!isspace(str.data[i])) {
            return i;
        }
    }

    return -1;
}


s64 str_find_whitespace_left(String str) {
    for (s64 i = 0; i < str.length; i++) {
        if (isspace(str.data[i])) {
            return i;
        }
    }

    return -1;
}

s64 str_find_whitespace_right(String str) {
    for (s64 i = str.length - 1; i > -1; i--) {
        if (isspace(str.data[i])) {
            return i;
        }
    }

    return -1;
}

String str_eat_spaces(String str) {
    s64 i = 0;

    while(i < str.length && isspace(str.data[i])) {
        i++;
    }

    return STR(str.length - i, str.data + i);
}

String str_eat_until_space(String str) {
    s64 i = 0;

    while(i < str.length && !isspace(str.data[i])) {
        i++;
    }

    return STR(str.length - i, str.data + i);
}

String str_get_until_space(String str) {
    s64 i = 0;

    while(i < str.length && !isspace(str.data[i])) {
        i++;
    }

    return STR(i, str.data);
}


bool str_is_int(String str) {
    if (str.length <= 0) {
        return false;
    }

    s64 i = 0;

    if (str.data[i] == '+' || str.data[i] == '-' ) {
        if (str.length < 2)
            return false;
        i++;
    }
    
    while(i < str.length) {
        if (!isdigit(str.data[i])) {
            return false;
        }
        i++;
    }

    return true;
}

bool str_is_float(String str) {
    if (str.length <= 0) {
        return false;
    }

    bool only_one_dot = false;
    s64 i = 0;

    if (str.data[i] == '+' || str.data[i] == '-') {
        // Edge case: "-"
        if (str.length < 2)
            return false;

        // Edge case: "-."
        if (str.data[i + 1] == '.' && str.length < 3) {
            return false;
        }
        i++;
    }
    
    while(i < str.length) {
        if (!isdigit(str.data[i])) {
            if (!only_one_dot && str.data[i] == '.') {
                only_one_dot = true;
            } else {
                return false;
            }
        }
        i++;
    }

    return true;
}

s64 str_parse_int(String str) {
    s8 sign = 1;
    s64 i = 0;

    if (str.data[0] == '-') {
        sign = -1;
        i = 1;
    } else if (str.data[0] == '+') {
        i = 1;
    }

    int result = 0;
    for (; i < str.length; i++) {
        result = result * 10 + (str.data[i] - '0');
    }

    return sign * result;
}

float str_parse_float(String str) {
    s8 sign = 1;
    s64 i = 0;

    if (str.data[0] == '-') {
        sign = -1;
        i = 1;
    } else if (str.data[0] == '+') {
        i = 1;
    }

    double result = 0.0;

    // Parse integer part
    while (i < str.length && str.data[i] != '.') {
        result = result * 10.0 + (str.data[i] - '0');
        i++;
    }

    // Parse fractional part
    if (i < str.length && str.data[i] == '.') {
        i++;  // skip dot
        double frac = 0.0;
        double base = 0.1;
        while (i < str.length) {
            frac += (str.data[i] - '0') * base;
            base *= 0.1;
            i++;
        }
        result += frac;
    }

    return sign * result;
}

s64 str_count_chars(String str, char c) {
    s64 count = 0;

    for (s64 i = 0; i < str.length; i++) {
        if (str.data[i] == c) {
            count++;
        }
    }

    return count;
}

void str_copy(String src, String dest) {
    (void)memcpy(src.data, dest.data, src.length);
}

void str_copy_buffer(String src, void *buffer) {
    (void)memcpy(src.data, buffer, src.length);
}


