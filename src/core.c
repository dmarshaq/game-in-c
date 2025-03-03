#include "core.h"
#include "SDL2/SDL_video.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
/**
 * Debug.
 */

const char* debug_error_str = "\033[31m[ERROR]\033[0m";
const char* debug_warning_str = "\033[33m[WARNING]\033[0m";
const char* debug_ok_str = "\033[32m[OK]\033[0m";


/**
 * Allocator.
 */

void allocator_destroy(Allocator *allocator) {
    free(allocator->ptr);
    allocator->ptr = NULL;
    allocator->alc_alloc = NULL;
    allocator->alc_free = NULL;
}

// Std.
void * std_malloc(Allocator_Header *header, u64 size) {
    return malloc(size);
}

void * std_calloc(Allocator_Header *header, u64 size) {
    return calloc(1, size);
}

void * std_realloc(Allocator_Header *header, void *ptr, u64 size) {
    return realloc(ptr, size);
}

void std_free(Allocator_Header *header, void *ptr) {
    free(ptr);
}

Allocator std_allocator = (Allocator) {
    .ptr = NULL,
    .alc_alloc = std_malloc,
    .alc_zero_alloc = std_calloc,
    .alc_re_alloc = std_realloc,
    .alc_free = std_free,
};


// Arena.
void * arena_alloc(Allocator_Header *header, u64 size) {
    if (header->size_filled + size > header->capacity) {
        (void)fprintf(stderr, "%s Couldn't allocate more memory of size: %lld bytes from the arena, which is filled at %lld out of %lld bytes.\n", debug_error_str, size, header->size_filled, header->capacity);
        return NULL;
    }
    void* ptr = (void *)header + sizeof(Allocator_Header) + header->size_filled;
    header->size_filled += size;
    return ptr;
}

Arena_Allocator arena_make(u64 capacity) {
    void *allocation = malloc(sizeof(Allocator_Header) + capacity);
    ((Allocator_Header *)allocation)->capacity = capacity;
    ((Allocator_Header *)allocation)->size_filled = 0;

    if (allocation == NULL) {
        (void)fprintf(stderr, "%s Couldn't allocate.\n", debug_error_str);
    }

    return (Arena_Allocator) {
        .ptr = allocation,
        .alc_alloc = arena_alloc,
        .alc_zero_alloc = NULL,
        .alc_re_alloc = NULL,
        .alc_free = NULL,
    };
}

void arena_destroy(Arena_Allocator *arena) {
    allocator_destroy(arena);
}

// Allocator interface.
void * allocator_alloc(Allocator *allocator, u64 size) {
    if (allocator->alc_alloc == NULL) {
        (void)fprintf(stderr, "%s Allocator couldn't allocate memory since allocataion function is not defined for the allocator.\n", debug_error_str);
        return NULL;
    }

    return allocator->alc_alloc(allocator->ptr, size);
}

void * allocator_zero_alloc(Allocator *allocator, u64 size) {
    if (allocator->alc_zero_alloc == NULL) {
        (void)fprintf(stderr, "%s Allocator couldn't zero allocate memory since zero allocataion function is not defined for the allocator.\n", debug_error_str);
        return NULL;
    }

    return allocator->alc_zero_alloc(allocator->ptr, size);
}

void * allocator_re_alloc(Allocator *allocator, void *ptr, u64 size) {
    if (allocator->alc_re_alloc == NULL) {
        (void)fprintf(stderr, "%s Allocator couldn't reallocate memory since reallocataion function is not defined for the allocator.\n", debug_error_str);
        return NULL;
    }

    return allocator->alc_re_alloc(allocator->ptr, ptr, size);
}

void allocator_free(Allocator *allocator, void *ptr) {
    if (allocator->alc_free == NULL) {
        (void)fprintf(stderr, "%s Allocator couldn't free memory since free function is not defined for the allocator.\n", debug_error_str);
        return;
    }

    return allocator->alc_free(allocator->ptr, ptr);
}


/**
 * String.
 */

String_8 str8_make_statically(const char *string) {
    return (String_8) {
        .ptr = (u8 *)string,
        .length = (u32)strlen(string),
    };
}

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

void str8_substring(String_8 *str, String_8 *output_str, u32 start, u32 end) {
    output_str->ptr = str->ptr + start;
    output_str->length = end - start;
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
            str8_substring(str, &substr, start, start + search_str->length);
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
 * Data strucutres.
 */

void * buffer_data_struct_resize(void *data, u32 new_size, u32 header_size, Allocator *allocator) {
    data = allocator_re_alloc(allocator, data - header_size, new_size * header_size);

    if (data == NULL) {
        (void)fprintf(stderr, "%s Couldn't reallocate memory of size: %u bytes, for the buffer data structure.\n", debug_error_str, new_size + header_size);
        return NULL;
    }

    return data + header_size;
}

void * buffer_data_struct_make(u32 size, u32 header_size, Allocator *allocator) {
    void *data = allocator_alloc(allocator, size * header_size);

    if (data == NULL) {
        (void)fprintf(stderr, "%s Couldn't allocate memory of size: %u bytes, for the buffer data structure.\n", debug_error_str, size + header_size);
        return NULL;
    }

    return data + header_size;
}

/**
 * Dynamic array.
 */

void * _array_list_make(u32 item_size, u32 capacity) {
    Array_List_Header *ptr = malloc(item_size * capacity + sizeof(Array_List_Header));

    if (ptr == NULL) {
        (void)fprintf(stderr, "%s Couldn't allocate more memory of size: %lld bytes, for the array list.\n", debug_error_str, item_size * capacity + sizeof(Array_List_Header));
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

void _array_list_resize(void **list, u32 new_capacity) {
    Array_List_Header *header = *list - sizeof(Array_List_Header);
    
    header = realloc(header, header->item_size * new_capacity + sizeof(Array_List_Header));
    
    if (header == NULL) {
        (void)fprintf(stderr, "%s Couldn't reallocate memory of size: %lld bytes, for the dynamic array.\n", debug_error_str, header->item_size * new_capacity + sizeof(Array_List_Header));
        return;
    }


    header->capacity = new_capacity;
    *list = header + 1;
}

void _array_list_free(void **list) {
    free(*list - sizeof(Array_List_Header));
    *list = NULL;
}

/**
 * Array list.
 */

void _array_list_resize_to_fit(void **list, u32 requiered_length) {
    Array_List_Header *header = *list - sizeof(Array_List_Header);

    if (requiered_length > header->capacity) {
        u32 capacity_multiplier = (u32)powf(2.0f, (float)((u32)(log2f((float)requiered_length / (float)header->capacity)) + 1));

        _array_list_resize(list, header->capacity * capacity_multiplier);
        header = *list - sizeof(Array_List_Header); // @Important: Resizing perfomed above changes the pointer to the list, so it is neccessary to reassign header ptr again, otherwise segfault occure.
    }

    if (header->capacity * header->item_size < requiered_length * header->item_size) {
        (void)fprintf(stderr, "%s List capacity size is less than requiered length size after being resized, capacity size: %d, size of requiered length: %d.\n", debug_error_str, header->capacity * header->item_size, requiered_length * header->item_size);
    }
}

u32 _array_list_next_index(void **list) {
    Array_List_Header *header = *list - sizeof(Array_List_Header);
    header->length += 1;
    return header->length - 1;
}

void _array_list_append_multiple(void **list, void *items, u32 count) {
    Array_List_Header *header = *list - sizeof(Array_List_Header);
    
    u32 requiered_length = header->length + count;

    _array_list_resize_to_fit(list, requiered_length);
    header = *list - sizeof(Array_List_Header); // @Important: Resizing perfomed above might change the pointer to the list, so it is neccessary to reassign header ptr again, otherwise segfault occure.
    
    memcpy(*list + header->length * header->item_size, items, header->item_size * count);
    header->length += count;
}

void _array_list_pop(void *list, u32 count) {
    ((Array_List_Header *)(list - sizeof(Array_List_Header)))->length -= count;
}

void _array_list_clear(void *list) {
    ((Array_List_Header *)(list - sizeof(Array_List_Header)))->length = 0;
}

void _array_list_unordered_remove(void *list, u32 index) {
    Array_List_Header *header = list - sizeof(Array_List_Header);
    memcpy(list + index * header->item_size, list + (header->length - 1) * header->item_size, header->item_size);
    _array_list_pop(list, 1);
}

/**
 * Hashmap. @Todo: Refactor implementation.
 */

typedef enum hashmap_slot_state : u8 {
    SLOT_EMPTY      = 0x00,
    SLOT_OCCUPIED   = 0x01,
    SLOT_DEPRICATED = 0x02,
} Hashmap_Slot_State;

/**
 * Internal function only.
 * Depricates all slots in the hashmap which are occupied.
 */
void hashmap_depricate(Hashmap *map) {
    Array_List_Header *header = map->buffer - sizeof(Array_List_Header);
    void *slot = NULL;
    for (u32 i = 0; i < header->capacity; i++) {
        slot = map->buffer + i * header->item_size;
        if (*(u8 *)slot == SLOT_OCCUPIED) {
            *(u8 *)slot = SLOT_DEPRICATED;
        }
    }
}

/**
 * Internal function only.
 * @Recursion: Recursivly readresses slots if its depricated.
 * Returns true, if it succesfully readressed a slot.
 */
bool hashmap_readress(Hashmap *map, u32 slot_index) {
    Array_List_Header *header = map->buffer - sizeof(Array_List_Header);
    void *original_slot = map->buffer + slot_index * header->item_size;

    // Check of deprication.
    if (*(u8 *)original_slot != SLOT_DEPRICATED)
        return false;

    *(u8 *)original_slot = SLOT_EMPTY;

    
    // Buffer key_size and key.
    u32 key_size = *((u32 *)(original_slot + sizeof(u8)));
    void *key = *((void **)(original_slot + sizeof(u8) + sizeof(u32)));
    
    u32 hash = map->hash_func(key, key_size);
    u32 index = hash % header->capacity;
    void *slot;
    // Trying to get slot that is not occupied.
    for (u32 j = 0; ; j++) {
        slot = map->buffer + ((index + j) % header->capacity) * header->item_size;
        if (*(u8 *)slot == SLOT_DEPRICATED) {
            if (hashmap_readress(map, ((index + j) % header->capacity))) {

                // Copying item to slot.
                *(u8 *)slot = SLOT_OCCUPIED;
                *(u32 *)(slot + sizeof(u8)) = key_size;
                *(void **)(slot + sizeof(u8) + sizeof(u32)) = key;
                memcpy(slot + sizeof(u8) + sizeof(u32) + sizeof(void *), original_slot + sizeof(u8) + sizeof(u32) + sizeof(void *), (header->item_size - sizeof(u8) - sizeof(u32) - sizeof(void *)));

                break;
            }
        }
        else if (*(u8 *)slot == SLOT_EMPTY) {

            // Copying item to slot.
            *(u8 *)slot = SLOT_OCCUPIED;
            *(u32 *)(slot + sizeof(u8)) = key_size;
            *(void **)(slot + sizeof(u8) + sizeof(u32)) = key;
            memcpy(slot + sizeof(u8) + sizeof(u32) + sizeof(void *), original_slot + sizeof(u8) + sizeof(u32) + sizeof(void *), (header->item_size - sizeof(u8) - sizeof(u32) - sizeof(void *)));

            break;
        }

        if (j >= header->capacity) {
            (void)fprintf(stderr, "%s Couldn't readress depricated slot in the hashmap.\n", debug_error_str);
            return false;
        }
    }
    
    return true;
}

Hashmap _hashmap_make(u32 item_size, u32 initial_capacity) {
    void *buffer = _array_list_make(sizeof(u8) + sizeof(u32) + sizeof(void *) + item_size, initial_capacity);
    memset(buffer, 0, initial_capacity * (sizeof(u8) + sizeof(u32) + sizeof(void *) + item_size));

    return (Hashmap) {
        .buffer = buffer, 
        .hash_func = hashf,
    };
}

void _hashmap_put(Hashmap *map, void *key, u32 key_size, void *item, u32 count) {
    Array_List_Header *header = map->buffer - sizeof(Array_List_Header);
    
    // Resize first if needed.
    u32 requiered_length = header->length + count;
    if (requiered_length > header->capacity) {
        // Fisrt depricate all occupied slots, since after resize, they will not be valid.
        hashmap_depricate(map);

        
        // Resize.
        u32 capacity_multiplier = (u32)powf(2.0f, (float)((u32)(log2f((float)requiered_length / (float)header->capacity)) + 1));
        
        _array_list_resize(&map->buffer, header->capacity * capacity_multiplier);
        header = map->buffer - sizeof(Array_List_Header);
        
        // Cleaning out new memory.
        memset(map->buffer + header->length * header->item_size, 0, (header->capacity - header->length) * header->item_size);


        // Readress all depricated slots.
        for (u32 i = 0; i < header->length; i++) {
            hashmap_readress(map, i);
        }
    }

    // Copying items one by one since every item needs its own hash.
    u32 hash = 0;
    u32 index = 0;
    void *slot = NULL;
    for (u32 i = 0; i < count; i++) {
        hash = map->hash_func(key, key_size);
        index = hash % header->capacity;

        // Trying to get slot that is not occupied.
        for (u32 j = 0; ; j++) {
            slot = map->buffer + ((index + j) % header->capacity) * header->item_size;
            if (*(u8 *)slot == SLOT_EMPTY) {
                
                // Copying item to slot.
                *(u8 *)slot = SLOT_OCCUPIED;
                *(u32 *)(slot + sizeof(u8)) = key_size;
                *(void **)(slot + sizeof(u8) + sizeof(u32)) = key;
                memcpy(slot + sizeof(u8) + sizeof(u32) + sizeof(void *), item + (header->item_size - sizeof(u8) - sizeof(u32) - sizeof(void *)) * i, (header->item_size - sizeof(u8) - sizeof(u32) - sizeof(void *)));
                
                break;
            }
            
            if (j >= header->capacity) {
                (void)fprintf(stderr, "%s Couldn't find slot for the element (%d) in the hashmap.\n", debug_error_str, i);
                return;
            }
        }
    }

    header->length += count;
}

void* _hashmap_get(Hashmap *map, void *key, u32 key_size) {
    Array_List_Header *header = map->buffer - sizeof(Array_List_Header);
    
    void *slot = NULL;
    u32 index = map->hash_func(key, key_size) % header->capacity;

    // Trying to get slot that is not occupied.
    for (u32 j = 0; ; j++) {
        slot = map->buffer + (index + j) * header->item_size;
        // printf("Checking index: %d, %d, %d, %d\n", index + j, *(u8 *)slot == SLOT_OCCUPIED, *(u32 *)(slot + sizeof(u8)) == key_size, memcmp(*(void **)(slot + sizeof(u8) + sizeof(u32)), key, key_size) == 0);
        if (*(u8 *)slot == SLOT_OCCUPIED && *(u32 *)(slot + sizeof(u8)) == key_size && memcmp(*(void **)(slot + sizeof(u8) + sizeof(u32)), key, key_size) == 0) {
            return slot + sizeof(u8) + sizeof(u32) + sizeof(void *);
        }

        if (j >= header->capacity) {
            (void)fprintf(stderr, "%s Couldn't find element stored under the key: 0x%16p of size: %4d.\n", debug_error_str, key, key_size);
            return NULL;
        }
    }
}

void _hashmap_remove(Hashmap *map, void *key, u32 key_size) {
    Array_List_Header *header = map->buffer - sizeof(Array_List_Header);
    void *slot = map->buffer + (map->hash_func(key, key_size) % header->capacity) * header->item_size;
    *(u8 *)slot = SLOT_EMPTY;
}

u32 hashf(void *key, u32 key_size) {
    if (key == NULL || key_size == 0) {
        (void)fprintf(stderr, "%s Couldn't hash a NULL or 0 sized key.\n", debug_error_str);
        return 0;
    }
    if (key_size < 2) {
        return *(u8 *)(key);
    }
    
    u32 hash = 0;
    u8 *hash_ptr = (u8 *)&(hash);
    hash_ptr[0] = *((u8 *)(key) + 0);
    hash_ptr[1] = *((u8 *)(key) + 1);
    hash_ptr[2] = *((u8 *)(key) + key_size - 1);
    hash_ptr[3] = *((u8 *)(key) + key_size - 2);

    return hash;
}

void hashmap_print_slot(u8 state, u32 key_size, void *key, void *item, u32 item_size) {
    // Print the state and key_size as hex.
    printf("State: 0x%02x | Key Size: 0x%08x | Key: 0x%16p | Item: ", state, key_size, key);
    
    // Print the item in hex based on item_size.
    for (u32 i = 0; i < item_size; i++) {
        printf("%02x ", *((u8 *)item + i));  // Print each byte of the item.
    }

    // Print newline after item.
    printf("\n");
}

void hashmap_print(Hashmap *map) {
    Array_List_Header *header = map->buffer - sizeof(Array_List_Header);
    void *slot = NULL;

    u8 state;
    u32 key_size;
    void *key;
    void *item;

    u32 item_size = header->item_size - sizeof(u8) - sizeof(u32) - sizeof(void *);

    printf("\n--------Hashmap--------\n");
    for (u32 i = 0; i < header->capacity; i++) {
        slot = map->buffer + i * header->item_size;

        state = *((u8 *)slot);
        key_size = *((u32 *)(slot + sizeof(u8)));
        key = *((void **)(slot + sizeof(u8) + sizeof(u32)));
        item = slot + sizeof(u8) + sizeof(u32) + sizeof(void *);

        hashmap_print_slot(state, key_size, key, item, item_size);
    }
}


/**
 * Math float.
 */

Matrix4f matrix4f_multiplication(Matrix4f *multiplier, Matrix4f *target) {
    Matrix4f result = {
        .array = {0}
    };
    float row_product;
    for (u8 col2 = 0; col2 < 4; col2++) {
        for (u8 row = 0; row < 4; row++) {
            row_product = 0;
            for (u8 col = 0; col < 4; col++) {
                row_product += multiplier->array[row * 4 + col] * target->array[col * 4 + col2];
            }
            result.array[row * 4 + col2] = row_product;
        }
    }
    return result;
}

Transform transform_srt_2d(Vec2f position, float angle, Vec2f scale) {
    return (Transform) {
        .array = { scale.x * cosf(angle),   scale.y * -sinf(angle), 0.0f, position.x, 
                   scale.x * sinf(angle),   scale.y *  cosf(angle), 0.0f, position.y,
                   0.0f,                    0.0f,                   1.0f, 0.0f,
                   0.0f,                    0.0f,                   0.0f, 1.0f}
    };
}

Transform transform_trs_2d(Vec2f position, float angle, Vec2f scale) {
    Transform s = transform_scale_2d(scale);
    Transform r = transform_rotation_2d(angle);
    Transform t = transform_translation_2d(position);

    r = matrix4f_multiplication(&s, &r);
    return matrix4f_multiplication(&r, &t);
}


/**
 * File utils.
 */

char* read_file_into_string(char *file_name, Allocator *allocator) {
    FILE *file = fopen(file_name, "rb");
    if (file == NULL) {
        (void)fprintf(stderr, "%s Couldn't open the file \"%s\".\n", debug_error_str, file_name);
        return NULL;
    }

    (void)fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    rewind(file);

    char *buffer = allocator_alloc(allocator, file_size + 1);
    if (buffer == NULL) {
        (void)fprintf(stderr, "%s Memory allocation for string buffer failed while reading the file \"%s\".\n", debug_error_str, file_name);
        (void)fclose(file);
        return NULL;
    }

    if (fread(buffer, 1, file_size, file) != file_size) {
        (void)fprintf(stderr, "%s Failure reading the file \"%s\".\n", debug_error_str, file_name);
        (void)fclose(file);
        free(buffer);
        return NULL;
    }

    buffer[file_size] = '\0';
    (void)fclose(file);

    return buffer;
}

void* read_file_into_buffer(char *file_name, u64 *file_size, Allocator *allocator) {
    FILE *file = fopen(file_name, "rb");
    if (file == NULL) {
        (void)fprintf(stderr, "%s Couldn't open the file \"%s\".\n", debug_error_str, file_name);
        return NULL;
    }

    (void)fseek(file, 0, SEEK_END);
    u64 size = ftell(file);
    rewind(file);

    u8 *buffer = allocator_alloc(allocator, size);
    if (buffer == NULL) {
        (void)fprintf(stderr, "%s Memory allocation for string buffer failed while reading the file \"%s\".\n", debug_error_str, file_name);
        (void)fclose(file);
        return NULL;
    }

    if (fread(buffer, 1, size, file) != size) {
        (void)fprintf(stderr, "%s Failure reading the file \"%s\".\n", debug_error_str, file_name);
        (void)fclose(file);
        free(buffer);
        return NULL;
    }

    (void)fclose(file);

    if (file_size != NULL)
        *file_size = size;

    return buffer;
}

String_8 read_file_into_str8(char *file_name, Allocator *allocator) {
    FILE *file = fopen(file_name, "rb");
    if (file == NULL) {
        (void)fprintf(stderr, "%s Couldn't open the file \"%s\".\n", debug_error_str, file_name);
    }

    (void)fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    rewind(file);

    String_8 str = str8_make_allocate((u32)file_size, allocator);

    if (str.ptr == NULL) {
        (void)fprintf(stderr, "%s Memory allocation for string buffer failed while reading the file \"%s\".\n", debug_error_str, file_name);
        (void)fclose(file);
        str.length = 0;
    }

    if (fread(str.ptr, 1, file_size, file) != file_size) {
        (void)fprintf(stderr, "%s Failure reading the file \"%s\".\n", debug_error_str, file_name);
        (void)fclose(file);
        free(str.ptr);
        str.ptr = NULL;
        str.length = 0;
    }

    (void)fclose(file);

    return str;
}

/**
 * Input.
 */

u8 *keyboard_state_old;
const u8 *keyboard_state_current;
s32 keyboard_number_of_keys;

void keyboard_state_free() {
    free(keyboard_state_old);
}

void keyboard_state_old_update() {
    (void)memcpy(keyboard_state_old, keyboard_state_current, keyboard_number_of_keys);
}

void keyboard_state_init() {
    keyboard_state_current = SDL_GetKeyboardState(&keyboard_number_of_keys);
    keyboard_state_old = malloc(keyboard_number_of_keys);
    keyboard_state_old_update();
}

bool is_hold_keycode(SDL_KeyCode keycode) {
    return *(keyboard_state_current + SDL_GetScancodeFromKey(keycode)) == 1;
}

bool is_pressed_keycode(SDL_KeyCode keycode) {
    return *(keyboard_state_old + SDL_GetScancodeFromKey(keycode)) == 0 && *(keyboard_state_current + SDL_GetScancodeFromKey(keycode)) == 1;
}

bool is_unpressed_keycode(SDL_KeyCode keycode) {
    return *(keyboard_state_old + SDL_GetScancodeFromKey(keycode)) == 1 && *(keyboard_state_current + SDL_GetScancodeFromKey(keycode)) == 0;
}


/**
 * Graphics.
 */

#include <GL/glew.h>

bool check_gl_error() {
    s32 error = glGetError();
    if (error != 0) {
        (void)fprintf(stderr, "%s OpenGL error: %d.\n", debug_error_str, error);
        return false;
    }
    return true;
}

int init_sdl_gl() {
    // Initialize SDL.
    if (SDL_Init(SDL_INIT_VIDEO) < 0 || SDL_Init(SDL_INIT_AUDIO) < 0) {
        (void)fprintf(stderr, "%s SDL could not initialize! SDL_Error: %s\n", debug_error_str, SDL_GetError());
        return 1;
    }

    // Set OpenGL attributes.
    (void)SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    (void)SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    (void)SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    
    return 0;
}

SDL_Window* create_gl_window(const char *title, int x, int y, int width, int height) {
    // Create window.
    SDL_Window *window = SDL_CreateWindow(title, x, y, width, height, SDL_WINDOW_OPENGL);
    if (window == NULL) {
        (void)fprintf(stderr, "%s Window could not be created! SDL_Error: %s\n", debug_error_str, SDL_GetError());
        return NULL;
    }

    // Create OpenGL context.
    SDL_GLContext glContext = SDL_GL_CreateContext(window);
    if (glContext == NULL) {
        (void)fprintf(stderr, "%s OpenGL context could not be created! SDL_Error: %s\n", debug_error_str, SDL_GetError());
        return window;
    }

    if (SDL_GL_MakeCurrent(window, glContext) < 0) {
        (void)fprintf(stderr, "%s OpenGL current could not be created! SDL_Error: %s\n", debug_error_str, SDL_GetError());
        return window;
    }

    // Initialize GLEW.
    if (glewInit() != GLEW_OK) {
        (void)fprintf(stderr, "%s GLEW initialization failed!\n", debug_error_str);
        return window;
    }

    return window;
}

int init_sdl_audio() {
    // Initialize sound.
    if (Mix_Init(MIX_INIT_MP3) < 0) {
        (void)fprintf(stderr, "%s SDL Mixer could not initialize! SDL_Error: %s\n", debug_error_str, SDL_GetError());
        return 1;
    }

    (void)Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 1024);
    return 0;
}


// Shader loading variables.
String_8 vertex_shader_defines;
String_8 fragment_shader_defines;

String_8 shader_version_tag;


s32 shader_strings_lengths[3];

// Texture params.
s32 texture_min_filter;
s32 texture_max_filter;
s32 texture_wrap_s;
s32 texture_wrap_t;

// Shader unifroms.
Matrix4f shader_uniform_pr_matrix;
Matrix4f shader_uniform_ml_matrix;
s32 shader_uniform_samplers[32];

// Drawing variables.
float *verticies;
u32 *quad_indicies;
u32 texture_ids[32];

void graphics_init() {
    // Enable Blending (Rendering with alpha channels in mind).
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Init strings for shader defines.
    vertex_shader_defines = str8_make_statically("#define VERTEX\n");
    fragment_shader_defines = str8_make_statically("#define FRAGMENT\n");

    // Init string for version tage in shader, for shader creation to look for when loading shader.
    shader_version_tag = str8_make_statically("#version");

    // Make stbi flip images vertically when loading.
    stbi_set_flip_vertically_on_load(true);

    // Init default texture parameters.
    texture_wrap_s = GL_CLAMP_TO_EDGE;
    texture_wrap_t = GL_CLAMP_TO_EDGE;
    texture_min_filter = GL_LINEAR;
    texture_max_filter = GL_LINEAR;

    // Init shader uniforms.
    shader_uniform_pr_matrix = MATRIX4F_IDENTITY;
    shader_uniform_ml_matrix = MATRIX4F_IDENTITY;
    for (s32 i = 0; i < 32; i++)
        shader_uniform_samplers[i] = i;

    // Setting drawing variables.
    verticies = array_list_make(float, 40); // @Leak
    quad_indicies = array_list_make(u32, MAX_QUADS_PER_BATCH * 6); // @Leak
    Array_List_Header *header = (void *)(quad_indicies) - sizeof(Array_List_Header);
    header->length = MAX_QUADS_PER_BATCH * 6;
    for (u32 i = 0; i < MAX_QUADS_PER_BATCH * 6; i++) {
        quad_indicies[i] = i - (i / 3) * 2 + (i / 6) * 2;
    }
}

Texture texture_load(char *texture_path) {
    // Process image into texture.
    Texture texture;
    s32 nrChannels;

    u8 *data = stbi_load(texture_path, &texture.width, &texture.height, &nrChannels, 0);

    if (data == NULL) {
        (void)fprintf(stderr, "%s Stbi couldn't load image.\n", debug_error_str);
    }

    // Loading a single image into texture example:
    glGenTextures(1, &texture.id);
    glBindTexture(GL_TEXTURE_2D, texture.id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, texture_wrap_s);	
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, texture_wrap_t);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, texture_min_filter);  
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, texture_max_filter);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture.width, texture.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glBindTexture(GL_TEXTURE_2D, 0);

    stbi_image_free(data);

    return texture;
}

void texture_unload(Texture *texture) {
    glDeleteTextures(1, &texture->id);

    texture->id = 0;
    texture->width = 0;
    texture->height = 0;
}


const char *shader_uniform_pr_matrix_name = "pr_matrix";
const char *shader_uniform_ml_matrix_name = "ml_matrix";
const char *shader_uniform_samplers_name = "u_textures";

/**
 * @Temporary: Later, setting uniforms either will be done more automatically, or simplified to be done by uset manually. 
 * But right now it is not neccassary to care about too much, since only one shader is used anyway.
 */
void shader_set_uniforms(Shader *program) {
    // Get uniform's locations based on unifrom's name.
    s32 quad_shader_pr_matrix_loc = glGetUniformLocation(program->id, shader_uniform_pr_matrix_name);
    if (quad_shader_pr_matrix_loc == -1) {
        (void)fprintf(stderr, "%s Couldn't get location of %s uniform, in shader.\n", debug_error_str, shader_uniform_pr_matrix_name);
    }

    s32 quad_shader_ml_matrix_loc = glGetUniformLocation(program->id, shader_uniform_ml_matrix_name);
    if (quad_shader_ml_matrix_loc == -1) {
        (void)fprintf(stderr, "%s Couldn't get location of %s uniform, in shader.\n", debug_error_str, shader_uniform_ml_matrix_name);
    }

    s32 quad_shader_samplers_loc = glGetUniformLocation(program->id, shader_uniform_samplers_name);
    if (quad_shader_samplers_loc == -1) {
        (void)fprintf(stderr, "%s Couldn't get location of %s uniform, in shader.\n", debug_error_str, shader_uniform_samplers_name);
    }
    
    // Set uniforms.
    glUseProgram(program->id);
    glUniformMatrix4fv(quad_shader_pr_matrix_loc, 1, GL_FALSE, shader_uniform_pr_matrix.array);
    glUniformMatrix4fv(quad_shader_ml_matrix_loc, 1, GL_FALSE, shader_uniform_ml_matrix.array);
    glUniform1iv(quad_shader_samplers_loc, 32, shader_uniform_samplers);
    glUseProgram(0);
}

bool check_program(u32 id, char *shader_path) {
    s32 is_linked = 0;
    glGetProgramiv(id, GL_LINK_STATUS, &is_linked); 
    if (is_linked == GL_FALSE) {
        (void)fprintf(stderr, "%s Program of %s, failed to link.\n", debug_error_str, shader_path);
       
        s32 info_log_length;
        glGetProgramiv(id, GL_INFO_LOG_LENGTH, &info_log_length);
        char *buffer = malloc(info_log_length);
        
        s32 buffer_size;
        glGetProgramInfoLog(id, info_log_length, &buffer_size, buffer);
        (void)fprintf(stderr, "%s\n", buffer);
        
        free(buffer);

        return false;
    } 
    return true;
}

bool check_shader(u32 id, char *shader_path) {
    s32 is_compiled = 0;
    glGetShaderiv(id, GL_COMPILE_STATUS, &is_compiled); 
    if (is_compiled == GL_FALSE) {
        (void)fprintf(stderr, "%s Shader of %s, failed to compile.\n", debug_error_str, shader_path);
        
        s32 info_log_length;
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &info_log_length);
        char *buffer = malloc(info_log_length);
        
        s32 buffer_size;
        glGetShaderInfoLog(id, info_log_length, &buffer_size, buffer);
        (void)fprintf(stderr, "%s\n", buffer);
        
        free(buffer);

        return false;
    } 
    return true;
}

Shader shader_load(char *shader_path) {
    /**
     * @Important: To avoid error while compiling glsl file we need to ensure that "#version ...\n" line comes before anything else in the final shader string. 
     * That said, we can't just have "#define ...\n" come before version tag. 
     * Therefore we split original shader source on it's shader version part that consists of single "#version ...\n" line, and rest of the shader code that comes after.
     * And finally we insert "define ...\n" part in between these two substrings.
     */
    String_8 shader_source, shader_version, shader_code;

    shader_source = read_file_into_str8(shader_path, &std_allocator);
    
    // Splitting on two substrings, "shader_version" and "shader_code".
    u32 start_of_version_tag = str8_index_of(&shader_source, &shader_version_tag, 0, shader_source.length);
    u32 end_of_version_tag = str8_index_of_char(&shader_source, '\n', start_of_version_tag, shader_source.length);
    str8_substring(&shader_source, &shader_version, start_of_version_tag, end_of_version_tag + 1);
    str8_substring(&shader_source, &shader_code, end_of_version_tag + 1, shader_source.length);
    
    // Passing all parts in the respective shader sources, where defines inserted between version and code parts.
    const char *vertex_shader_source[3] = { (char *)shader_version.ptr, (char *)vertex_shader_defines.ptr, (char *)shader_code.ptr };
    const char *fragment_shader_source[3] = { (char *)shader_version.ptr, (char *)fragment_shader_defines.ptr, (char *)shader_code.ptr };
    
    /**
     * Specifying lengths, because we don't pass null terminated strings.
     * @Important: Since defines length depends on whether we are loading vertex or fragment shader, we set it's length values separately when loading each.
     */
    shader_strings_lengths[0] = shader_version.length;
    shader_strings_lengths[2] = shader_code.length;
    

    shader_strings_lengths[1] = vertex_shader_defines.length;
    u32 vertex_shader;
    vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 3, vertex_shader_source, shader_strings_lengths);
    glCompileShader(vertex_shader);
    
    // Check results for errors.
    (void)check_shader(vertex_shader, shader_path);

    shader_strings_lengths[1] = fragment_shader_defines.length;
    u32 fragment_shader;
    fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 3, fragment_shader_source, shader_strings_lengths);
    glCompileShader(fragment_shader);
    
    // Check results for errors.
    (void)check_shader(fragment_shader, shader_path);
    
    Shader shader = {
        .id = glCreateProgram(),
    };

    glAttachShader(shader.id, vertex_shader);
    glAttachShader(shader.id, fragment_shader);
    glLinkProgram(shader.id);
    
    // Check results for errors.
    (void)check_program(shader.id, shader_path);

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    

    str8_free(&shader_source, &std_allocator);

    return shader;
}

void shader_unload(Shader *shader) {
    glUseProgram(0);
    glDeleteProgram(shader->id);
    
    shader->id = 0;
    shader->vertex_stride = 0;
}


Camera camera_make(Vec2f center, u32 unit_scale) {
    return (Camera) {
        .center = center,
        .unit_scale = unit_scale,
    };
}

void shader_update_projection(Shader *shader, Camera *camera, float window_width, float window_height) {
    float camera_width_offset = (window_width / (float) camera->unit_scale) / 2;
    float camera_height_offset = (window_height / (float) camera->unit_scale) / 2;
    shader_uniform_pr_matrix = matrix4f_orthographic(camera->center.x - camera_width_offset, camera->center.x + camera_width_offset, camera->center.y - camera_height_offset, camera->center.y + camera_height_offset, -1.0f, 1.0f);

    s32 quad_shader_pr_matrix_loc = glGetUniformLocation(shader->id, shader_uniform_pr_matrix_name);
    if (quad_shader_pr_matrix_loc == -1) {
        (void)fprintf(stderr, "%s Couldn't get location of %s uniform, in quad_shader.\n", debug_error_str, shader_uniform_pr_matrix_name);
    }

    // Set uniforms.
    glUseProgram(shader->id);
    glUniformMatrix4fv(quad_shader_pr_matrix_loc, 1, GL_FALSE, shader_uniform_pr_matrix.array);
    glUseProgram(0);
}

// @Incomplete: Finish attributes pointers setting for different types of shaders and strides.
void drawer_init(Quad_Drawer *drawer, Shader *shader) {
    drawer->program = shader;

    // Setting Vertex Objects for render using OpenGL. Also seeting up Element Buffer Object for indices to load.
    glGenVertexArrays(1, &drawer->vao);
    glGenBuffers(1, &drawer->vbo);
    glGenBuffers(1, &drawer->ebo);

    // 1. Bind Vertex Array Object. [VAO]
    glBindVertexArray(drawer->vao);
    
    // 2. Copy verticies array in a buffer for OpenGL to use. [VBO].
    glBindBuffer(GL_ARRAY_BUFFER, drawer->vbo);
    glBufferData(GL_ARRAY_BUFFER, drawer->program->vertex_stride * VERTICIES_PER_QUAD * MAX_QUADS_PER_BATCH * sizeof(float), NULL, GL_DYNAMIC_DRAW);

    // 3. Copy indicies array in a buffer for OpenGL to use. [EBO].
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, drawer->ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, array_list_length(&quad_indicies) * sizeof(float), quad_indicies, GL_STATIC_DRAW);
    
    // 3. Set vertex attributes pointers. [VAO, VBO, EBO].
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, drawer->program->vertex_stride * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, drawer->program->vertex_stride * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, drawer->program->vertex_stride * sizeof(float), (void*)(7 * sizeof(float)));
    glEnableVertexAttribArray(2);
    
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, drawer->program->vertex_stride * sizeof(float), (void*)(9 * sizeof(float)));
    glEnableVertexAttribArray(3);
    
    glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, drawer->program->vertex_stride * sizeof(float), (void*)(10 * sizeof(float)));
    glEnableVertexAttribArray(4);
    
    // 4. Unbind EBO, VBO and VAO.
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    shader_set_uniforms(drawer->program);
}

void drawer_free(Quad_Drawer *drawer) {
    glDeleteVertexArrays(1, &drawer->vao); 
    glDeleteBuffers(1, &drawer->vbo); 
    glDeleteBuffers(1, &drawer->ebo); 

    drawer->program = NULL;
    drawer->vao = 0;
    drawer->vbo = 0;
    drawer->ebo = 0;
}



u8 texture_ids_filled_length = 0;

float add_texture_to_slots(Texture *texture) {
    for (u8 i = 0; i < texture_ids_filled_length; i++) {
        if (texture_ids[i] == texture->id)
            return i;
    }
    if (texture_ids_filled_length < 32) {
        texture_ids[texture_ids_filled_length] = texture->id;
        texture_ids_filled_length++;
        return texture_ids_filled_length - 1;
    }
    (void)fprintf(stderr, "%s Overflow of 32 texture slots limit, can't add texture id: %d, to current draw call texture slots.\n", debug_error_str, texture->id);
    return -1.0f;
}


Quad_Drawer *active_drawer = NULL;

void draw_begin(Quad_Drawer* drawer) {
    active_drawer = drawer;
}

void draw_end() {
    // Bind buffers, program, textures.
    glUseProgram(active_drawer->program->id);

    for (u8 i = 0; i < 32; i++) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, texture_ids[i]);
    }

    glBindVertexArray(active_drawer->vao);
    glBindBuffer(GL_ARRAY_BUFFER, active_drawer->vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, active_drawer->ebo);

    u32 batch_stride = MAX_QUADS_PER_BATCH * VERTICIES_PER_QUAD * active_drawer->program->vertex_stride;
    // Spliting all data on equal batches, and processing each batch in each draw call.
    u32 batches = array_list_length(&verticies) / batch_stride;
    for (u32 i = 0; i < batches; i++) {
        glBufferSubData(GL_ARRAY_BUFFER, 0, batch_stride * sizeof(float), verticies + (i * batch_stride));
        glDrawElements(GL_TRIANGLES, array_list_length(&quad_indicies), GL_UNSIGNED_INT, 0);
    }
    
    // Data that didn't group into full batch, rendered in last draw call.
    u32 float_attributes_left = (array_list_length(&verticies) - batch_stride * batches);
    glBufferSubData(GL_ARRAY_BUFFER, 0, float_attributes_left * sizeof(float), verticies + (batches * batch_stride));
    glDrawElements(GL_TRIANGLES, float_attributes_left / active_drawer->program->vertex_stride / VERTICIES_PER_QUAD * INDICIES_PER_QUAD, GL_UNSIGNED_INT, 0);

    // Unbinding of buffers after use.
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    
    // Unbind texture ids.
    for (u8 i = 0; i < 32; i++) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    texture_ids_filled_length = 0;

    glUseProgram(0);

    // Clean up.
    active_drawer = NULL;
    array_list_clear(&verticies);
}

void draw_quad_data(float *quad_data, u32 count) {
    array_list_append_multiple(&verticies, quad_data, count * VERTICIES_PER_QUAD * active_drawer->program->vertex_stride);
}


Font_Baked font_bake(u8 *font_data, float font_size) {
    Font_Baked result;
    
    // Init font info.
    stbtt_fontinfo info;
    (void)stbtt_InitFont(&info, font_data, 0);

    // Initing needed variables. @Temporary: Maybe move some of this variables into font struct directly and calculate when baking bitmap.
    s32 ascent, descent, line_gap;
    stbtt_GetFontVMetrics(&info, &ascent, &descent, &line_gap);

    float scale = stbtt_ScaleForPixelHeight(&info, font_size);
    result.line_height = (s32)((float)(ascent - descent + line_gap) * scale);
    result.baseline = (s32)((float)ascent * -scale);

    // Create a bitmap.
    result.bitmap.width    = 512;
    result.bitmap.height   = 512;
    u8 *bitmap = calloc(result.bitmap.width * result.bitmap.height, sizeof(u8));

    // Bake the font into the bitmap.
    result.first_char_code  = 32; // ASCII value of the first character to bake.
    result.chars_count      = 96;  // Number of characters to bake.
                         
    result.chars = malloc(result.chars_count * sizeof(stbtt_bakedchar));
    (void)stbtt_BakeFontBitmap(font_data, 0, font_size, bitmap, result.bitmap.width, result.bitmap.height, result.first_char_code, result.chars_count, result.chars);

    // Create an OpenGL texture.
    glGenTextures(1, &result.bitmap.id);
    glBindTexture(GL_TEXTURE_2D, result.bitmap.id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, texture_wrap_s);	
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, texture_wrap_t);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, texture_min_filter);  
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, texture_max_filter);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, result.bitmap.width, result.bitmap.height, 0, GL_RED, GL_UNSIGNED_BYTE, bitmap);
    glBindTexture(GL_TEXTURE_2D, 0);

    free(bitmap);

    return result;
}

void font_free(Font_Baked *font) {
    free(font->chars);
    font->line_height = 0;
    font->baseline = 0;
    font->first_char_code = 0;
    font->chars_count = 0;
    texture_unload(&font->bitmap);
}


void line_drawer_init(Line_Drawer *drawer, Shader *shader) {
    drawer->program = shader;

    // Setting Vertex Objects for render using OpenGL. Also seeting up Element Buffer Object for indices to load.
    glGenVertexArrays(1, &drawer->vao);
    glGenBuffers(1, &drawer->vbo);

    // 1. Bind Vertex Array Object. [VAO]
    glBindVertexArray(drawer->vao);
    
    // 2. Copy verticies array in a buffer for OpenGL to use. [VBO].
    glBindBuffer(GL_ARRAY_BUFFER, drawer->vbo);
    // printf("max: %d\n", drawer->program->vertex_stride * VERTICIES_PER_QUAD * MAX_QUADS_PER_BATCH);
    glBufferData(GL_ARRAY_BUFFER, drawer->program->vertex_stride * VERTICIES_PER_LINE * MAX_LINES_PER_BATCH * sizeof(float), NULL, GL_DYNAMIC_DRAW);

    // 3. Set vertex attributes pointers. [VAO, VBO].
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, drawer->program->vertex_stride * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, drawer->program->vertex_stride * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // 4. Unbind EBO, VBO and VAO.
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    shader_set_uniforms(drawer->program);
}

void line_drawer_free(Line_Drawer *drawer) {
    glDeleteVertexArrays(1, &drawer->vao); 
    glDeleteBuffers(1, &drawer->vbo); 

    drawer->program = NULL;
    drawer->vao = 0;
    drawer->vbo = 0;
}


Line_Drawer *active_line_drawer = NULL;

void line_draw_begin(Line_Drawer* drawer) {
    active_line_drawer = drawer;
}

void line_draw_end() {
    // Bind buffers, program, textures.
    glUseProgram(active_line_drawer->program->id);

    glBindVertexArray(active_line_drawer->vao);
    glBindBuffer(GL_ARRAY_BUFFER, active_line_drawer->vbo);


    u32 batch_stride = MAX_LINES_PER_BATCH * VERTICIES_PER_LINE * active_line_drawer->program->vertex_stride;
    // Spliting all data on equal batches, and processing each batch in each draw call.
    u32 batches = array_list_length(&verticies) / batch_stride;
    for (u32 i = 0; i < batches; i++) {
        glBufferSubData(GL_ARRAY_BUFFER, 0, batch_stride * sizeof(float), verticies + (i * batch_stride));
        glDrawArrays(GL_LINES, 0, MAX_LINES_PER_BATCH * VERTICIES_PER_LINE);
    }
    
    // Data that didn't group into full batch, rendered in last draw call.
    u32 float_attributes_left = (array_list_length(&verticies) - batch_stride * batches);
    glBufferSubData(GL_ARRAY_BUFFER, 0, float_attributes_left * sizeof(float), verticies + (batches * batch_stride));
    glDrawArrays(GL_LINES, 0, float_attributes_left / active_line_drawer->program->vertex_stride);


    // Unbinding of buffers after use.
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    
    glUseProgram(0);

    // Clean up.
    active_line_drawer = NULL;
    array_list_clear(&verticies);
}

void draw_line_data(float *line_data, u32 count) {
    array_list_append_multiple(&verticies, line_data, count * VERTICIES_PER_LINE * active_line_drawer->program->vertex_stride);
}






void print_verticies() {
    u32 stride = 1;
    if (active_drawer != NULL) {
        stride = active_drawer->program->vertex_stride;
    }
    if (active_line_drawer != NULL) {
        stride = active_line_drawer->program->vertex_stride;
    }

    (void)printf("\n---------- VERTICIES -----------\n");
    u32 length = array_list_length(&verticies);
    if (length == 0) {
        (void)printf("[ ]\n");
    }
    else {
        (void)printf("[ ");
        for (u32 i = 0; i < length - 1; i++) {
            (void)printf("%6.1f, ", verticies[i]);
            if ((i + 1) % stride == 0)
                (void)printf("\n  ");
        }
        (void)printf("%6.1f  ]\n", verticies[length - 1]);
    }

   (void)printf("Length   : %8d\n", length);
   (void)printf("Capacity : %8d\n\n", array_list_capacity(&verticies));
    
}

void print_indicies() {
    (void)printf("\n----------- INDICIES -----------\n");
    (void)printf("[\n\n  ");
    u32 length = array_list_length(&quad_indicies);
    for (u32 i = 0; i < length; i++) {
        if ((i + 1) % 6 == 0)
            (void)printf("%4d\n\n  ", quad_indicies[i]);
        else if ((i + 1) % 3 == 0)
            (void)printf("%4d\n  ", quad_indicies[i]);
        else
            (void)printf("%4d, ", quad_indicies[i]);
    }
   (void)printf("\r]\n");
   (void)printf("Length   : %8d\n", length);
   (void)printf("Capacity : %8d\n\n", array_list_capacity(&quad_indicies));
}
