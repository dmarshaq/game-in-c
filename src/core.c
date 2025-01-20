#include "core.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

/**
 * Debug.
 */

const char* debug_error_str = "\033[31m[ERROR]\033[0m";
const char* debug_warning_str = "\033[33m[WARNING]\033[0m";
const char* debug_ok_str = "\033[32m[OK]\033[0m";


/**
 * Arena allocator.
 */

void arena_init(Arena *arena, u64 size) {
    arena->ptr = malloc(size);
    if (arena->ptr == NULL) {
        fprintf(stderr, "%s Couldn't allocate memory for inititing arena of size %lld.\n", debug_error_str, size);
        arena->size = 0;
        arena->size_filled = 0;
        return;
    }
    arena->size = size;
    arena->size_filled = 0;
}

void* arena_allocate(Arena *arena, u64 size) {
    if (arena->size_filled + size > arena->size) {
        fprintf(stderr, "%s Couldn't allocate more memory of size: %lld bytes from the arena, which is filled at %lld out of %lld bytes.\n", debug_error_str, size, arena->size_filled, arena->size);
        return NULL;
    }
    void* ptr = arena->ptr + arena->size_filled;
    arena->size_filled += size;
    return ptr;
}

void arena_free(Arena *arena) {
    free(arena->ptr);
    arena->ptr = NULL;
    arena->size = 0;
    arena->size_filled = 0;
}


/**
 * String.
 */

void str8_init_statically(String_8 *str, const char *string) {
    str->ptr = (u8 *)string;
    str->length = (u32)strlen(string);
}

void str8_init_arena(String_8 *str, Arena *arena, u32 length) {
    str->ptr = arena_allocate(arena, length);
    str->length = length;
}

void str8_init_dynamically(String_8 *str, u32 length) {
    str->ptr = malloc(length);
    str->length = length;
}

void str8_free(String_8 *str) {
    free(str->ptr);
    str->ptr = NULL;
    str->length = 0;
}

void str8_substring(String_8 *str, String_8 *output_str, u32 start, u32 end) {
    output_str->ptr = str->ptr + start;
    output_str->length = end - start;
}

void str8_memcopy_from_buffer(String_8 *str, void *buffer, u32 length) {
    memcpy(str->ptr, buffer, length);
}

void str8_memcopy_into_buffer(String_8 *str, void *buffer, u32 start, u32 end) {
    memcpy(buffer, str->ptr + start, end - start);
}

bool str8_equals_str8(String_8 *str1, String_8 *str2) {
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

u32 str8_index_of_str8(String_8 *str, String_8 *search_str, u32 start, u32 end) {
    String_8 substr;
    for (; start < end; start++) {
        if (str->ptr[start] == search_str->ptr[0] && start + search_str->length <= str->length) {
            str8_substring(str, &substr, start, start + search_str->length);
            if (str8_equals_str8(&substr, search_str)) {
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
            str8_substring(str, &substr, start, start + search_string_length);
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


/**
 * Array list.
 */

void* _array_list_make(u32 item_size, u32 capacity) {
    Array_List_Header *ptr = malloc(item_size * capacity + sizeof(Array_List_Header));

    if (ptr == NULL) {
        fprintf(stderr, "%s Couldn't allocate more memory of size: %lld bytes, for the array list.\n", debug_error_str, item_size * capacity + sizeof(Array_List_Header));
        return NULL;
    }

    ptr->capacity = capacity;
    ptr->item_size = item_size;
    ptr->length = 0;
    
    // @Important: Because ptr is of type "Array_List_Header *", compiler will automatically translate "ptr + 1" to "(void*)(ptr) + sizeof(Array_List_Header)".
    return ptr + 1;
}

u32 _array_list_length(void *list) {
    return ((Array_List_Header *)(list - sizeof(Array_List_Header)))->length;
}

u32 _array_list_capacity(void *list) {
    return ((Array_List_Header *)(list - sizeof(Array_List_Header)))->capacity;
}

u32 _array_list_item_size(void *list) {
    return ((Array_List_Header *)(list - sizeof(Array_List_Header)))->item_size;
}

void _array_list_append(void **list, void *item, u32 count) {
    Array_List_Header *header = *list - sizeof(Array_List_Header);
    
    u32 requiered_length = header->length + count;
    if (requiered_length > header->capacity) {
        u32 capacity_multiplier = 2 * ((u32)(log2f((float)requiered_length / (float)header->capacity)) + 1);
        header = realloc(header, header->item_size * header->capacity * capacity_multiplier + sizeof(Array_List_Header));
        header->capacity *= capacity_multiplier;
        *list = header + 1;
    }
    
    memcpy(*list + header->length * header->item_size, item, header->item_size * count);
    header->length += count;
}

void _array_list_pop(void *list, u32 count) {
    ((Array_List_Header *)(list - sizeof(Array_List_Header)))->length -= count;
}

void _array_list_clear(void *list) {
    ((Array_List_Header *)(list - sizeof(Array_List_Header)))->length = 0;
}

void _array_list_free(void **list) {
    free(*list - sizeof(Array_List_Header));
    *list = NULL;
}

void _array_list_unordered_remove(void *list, u32 index) {
    Array_List_Header *header = list - sizeof(Array_List_Header);
    memcpy(list + index * header->item_size, list + (header->length - 1) * header->item_size, header->item_size);
    _array_list_pop(list, 1);
}


/**
 * Math float.
 */



/**
 * Graphics.
 */



