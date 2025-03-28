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
        printf_err("Couldn't allocate more memory of size: %lld bytes from the arena, which is filled at %lld out of %lld bytes.\n", size, header->size_filled, header->capacity);
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
        printf_err("Couldn't allocate.\n");
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
        printf_err("Allocator couldn't allocate memory since allocataion function is not defined for the allocator.\n");
        return NULL;
    }

    return allocator->alc_alloc(allocator->ptr, size);
}

void * allocator_zero_alloc(Allocator *allocator, u64 size) {
    if (allocator->alc_zero_alloc == NULL) {
        printf_err("Allocator couldn't zero allocate memory since zero allocataion function is not defined for the allocator.\n");
        return NULL;
    }

    return allocator->alc_zero_alloc(allocator->ptr, size);
}

void * allocator_re_alloc(Allocator *allocator, void *ptr, u64 size) {
    if (allocator->alc_re_alloc == NULL) {
        printf_err("Allocator couldn't reallocate memory since reallocataion function is not defined for the allocator.\n");
        return NULL;
    }

    return allocator->alc_re_alloc(allocator->ptr, ptr, size);
}

void allocator_free(Allocator *allocator, void *ptr) {
    if (allocator->alc_free == NULL) {
        printf_err("Allocator couldn't free memory since free function is not defined for the allocator.\n");
        return;
    }

    return allocator->alc_free(allocator->ptr, ptr);
}


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


/**
 * Data strucutres.
 */

typedef struct buffer_data_struct_header {
    Allocator *allocator;
} Buffer_Data_Struct_Header;

void *buffer_data_struct_make(u32 size, u32 header_size, Allocator *allocator) {
    void *data = allocator_alloc(allocator, size + header_size + sizeof(Buffer_Data_Struct_Header));
    ((Buffer_Data_Struct_Header *)data)->allocator = allocator;

    if (data == NULL) {
        printf_err("Couldn't allocate memory of size: %llu bytes, for the buffer data structure.\n", size + header_size + sizeof(Buffer_Data_Struct_Header));
        return NULL;
    }

    return data + header_size + sizeof(Buffer_Data_Struct_Header);
}

void *buffer_data_struct_resize(void *data, u32 new_size, u32 header_size) {
    Allocator *allocator = ((Buffer_Data_Struct_Header *)(data - header_size - sizeof(Buffer_Data_Struct_Header)))->allocator;
    data = allocator_re_alloc(allocator, data - header_size - sizeof(Buffer_Data_Struct_Header), new_size + header_size + sizeof(Buffer_Data_Struct_Header));

    if (data == NULL) {
        printf_err("Couldn't reallocate memory of size: %llu bytes, for the buffer data structure.\n", new_size + header_size + sizeof(Buffer_Data_Struct_Header));
        return NULL;
    }

    return data + header_size + sizeof(Buffer_Data_Struct_Header);
}

void buffer_data_struct_free(void *data, u32 header_size) {
    Allocator *allocator = ((Buffer_Data_Struct_Header *)(data - header_size - sizeof(Buffer_Data_Struct_Header)))->allocator;
    allocator_free(allocator, data - header_size - sizeof(Buffer_Data_Struct_Header));
}

/**
 * Array list.
 */

void *_array_list_make(u32 item_size, u32 capacity, Allocator *allocator) {
    Array_List_Header *ptr = buffer_data_struct_make(item_size * capacity, sizeof(Array_List_Header), allocator) - sizeof(Array_List_Header);

    if (ptr == NULL) {
        printf_err("Couldn't allocate more memory of size: %lld bytes, for the array list.\n", item_size * capacity + sizeof(Array_List_Header));
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

void _array_list_free(void **list) {
    buffer_data_struct_free(*list, sizeof(Array_List_Header));
    *list = NULL;
}

void _array_list_resize_to_fit(void **list, u32 requiered_length) {
    Array_List_Header *header = *list - sizeof(Array_List_Header);

    if (requiered_length > header->capacity) {
        u32 capacity_multiplier = (u32)powf(2.0f, (float)((u32)(log2f((float)requiered_length / (float)header->capacity)) + 1));

        
        *list = buffer_data_struct_resize(*list, header->capacity * capacity_multiplier * header->item_size, sizeof(Array_List_Header));
        header = *list - sizeof(Array_List_Header); // @Important: Resizing perfomed above changes the pointer to the list, so it is neccessary to reassign header ptr again, otherwise segfault occure.
        header->capacity *= capacity_multiplier; // @Important: Using "buffer_data_struct_resize" will not update capacity in the header, because this function is only designed to only resize the whole data structure, there for it is needed to manually set capacity to the right value, which was intended.
    }

    if (header->capacity * header->item_size < requiered_length * header->item_size) {
        printf_err("List capacity size is less than requiered length size after being resized, capacity size: %d, size of requiered length: %d.\n", header->capacity * header->item_size, requiered_length * header->item_size);
    }
}

u32 _array_list_next_index(void **list) {
    Array_List_Header *header = *list - sizeof(Array_List_Header);
    header->length += 1;
    return header->length - 1;
}

u32 _array_list_append_multiple(void **list, void *items, u32 count) {
    Array_List_Header *header = *list - sizeof(Array_List_Header);
    
    u32 requiered_length = header->length + count;

    _array_list_resize_to_fit(list, requiered_length);
    header = *list - sizeof(Array_List_Header); // @Important: Resizing perfomed above might change the pointer to the list, so it is neccessary to reassign header ptr again, otherwise segfault occure.

    (void)memcpy(*list + header->length * header->item_size, items, header->item_size * count);
    header->length += count;

    return header->length - count;
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
 * Hash Table. 
 */

#define hash_table_header(table_ptr)        ((Hash_Table_Header *)(*(table_ptr) - sizeof(Hash_Table_Header)))

typedef enum hash_table_slot_state : u8 {
    SLOT_EMPTY      = 0x00,
    SLOT_OCCUPIED   = 0x01,
    SLOT_DEPRICATED = 0x02,
} Hash_Table_Slot_State;

typedef struct hash_table_slot {
    Hash_Table_Slot_State state;
    String_8 key;
} Hash_Table_Slot;

/**
 * Internal function.
 */
Hash_Table_Slot *hash_table_get_slot(void **table, u32 index) {
    return *table + hash_table_header(table)->capacity * hash_table_header(table)->item_size + index * sizeof(Hash_Table_Slot);
}

/**
 * Internal function.
 * Returns index of the corresponding key by calculating hash.
 * @Careful: Doesn't perfom any slot checks.
 */
u32 hash_table_hash_index_of(void **table, void *key, u32 key_size) {
    Hash_Table_Header *header = hash_table_header(table);
    return header->hash_func(key, key_size) % header->capacity;
}

/**
 * Internal function.
 * Sets all occupied slots to depricated.
 */
void hash_table_depricate_slots(void **table) {
    u32 cap = hash_table_capacity(table);
    Hash_Table_Slot *slot = NULL;
    for (u32 i = 0; i < cap; i++) {
        slot = hash_table_get_slot(table, i);
        if (slot->state == SLOT_OCCUPIED) {
            slot->state = SLOT_DEPRICATED;
        }
    }
}

/**
 * @Internal function.
 */
void hash_table_print_slot(void *item, u32 item_size, Hash_Table_Slot *slot) {
    
    // Print the item in hex based on item_size.
    printf("Item: 0x");
    for (u32 i = 0; i < item_size; i++) {
        (void)printf("%02x", *((u8 *)item + i));  // Print each byte of the item.
    }

    // Print the state and key_size as hex.
    (void)printf(" | State: 0x%02x | Key Size: 0x%08x | Key Ptr: 0x%16p -> ", slot->state, slot->key.length, slot->key.ptr);

    if (slot->state == SLOT_OCCUPIED) {
        
        // Print key itself
        (void)printf("%.*s", slot->key.length, slot->key.ptr);
    }
    (void)printf("\n");
}

/**
 * Internal function.
 * @Recursion: Recursivly readresses slots if they are depricated.
 * Returns true, if it succesfully readressed a slot.
 */
bool hash_table_readress(void **table, u32 index) {
    Hash_Table_Header *header = hash_table_header(table);
    Hash_Table_Slot *target_slot = hash_table_get_slot(table, index);

    u32 new_index = hash_table_hash_index_of(table, target_slot->key.ptr, target_slot->key.length);
    target_slot->state = SLOT_EMPTY;

    Hash_Table_Slot *slot = NULL;
    for (u32 i = 0; i < header->capacity; i++) {
        slot = hash_table_get_slot(table, (new_index + i) % header->capacity);
        if (slot->state == SLOT_EMPTY) {
            // Copy data to a new slot.
            slot->state = SLOT_OCCUPIED;

            slot->key.length = target_slot->key.length;
            slot->key.ptr = target_slot->key.ptr;

            memmove(*table + ((new_index + i) % header->capacity) * header->item_size, *table + index * header->item_size, header->item_size);

            return true;
        }
        else if (slot->state == SLOT_DEPRICATED && hash_table_readress(table, (new_index + i) % header->capacity)) {

            // @Important: There is an additional is SLOT_OCCUPIED check, because hash_table_readress can possbily readress slot to the same index as it was before, therefore additional check is needed.
            if (slot->state != SLOT_OCCUPIED) {
                // Copy data to a new slot.
                slot->state = SLOT_OCCUPIED;

                slot->key.length = target_slot->key.length;
                slot->key.ptr = target_slot->key.ptr;

                memmove(*table + ((new_index + i) % header->capacity) * header->item_size, *table + index * header->item_size, header->item_size);

                return true;
            }
        }
    }

    
    printf_err("Couldn't find new free hash slot when readressing.\n");
    return false;

}

void *_hash_table_make(u32 item_size, u32 capacity, Allocator *allocator) {
    u32 table_size = (sizeof(Hash_Table_Slot) + item_size) * capacity;
    void *buffer = buffer_data_struct_make(table_size, sizeof(Hash_Table_Header), allocator);

    if (buffer == NULL) {
        printf_err("Couldn't allocate more memory of size: %lld bytes, for the hash table.\n", item_size * capacity + sizeof(Hash_Table_Header));
        return NULL;
    }

    u8 *keys = array_list_make(u8, capacity * 8, allocator);    

    if (keys == NULL) {
        printf_err("Couldn't allocate more memory of size: %u bytes, for the keys for the hash table.\n", item_size * capacity * 8);
        buffer_data_struct_free(buffer, sizeof(Hash_Table_Header));
        return NULL;
    }

    Hash_Table_Header *header = buffer - sizeof(Hash_Table_Header);
    header->capacity = capacity;
    header->item_size = item_size;
    header->count = 0;
    header->hash_func = hashf;
    header->keys = keys;

    // Set slots to SLOT_EMPTY.
    for (u32 i = 0; i < header->capacity; i++) {
        hash_table_get_slot(&buffer, i)->state = SLOT_EMPTY;
    }
    
    // @Important: Because header is of type "Hash_Table_Header *", compiler will automatically translate "header + 1" to "(void *)(header) + sizeof(Hash_Table_Header)".
    return header + 1;
}

u32 _hash_table_count(void *table) {
    return ((Hash_Table_Header *)(table - sizeof(Hash_Table_Header)))->count;
}

u32 _hash_table_capacity(void *table) {
    return ((Hash_Table_Header *)(table - sizeof(Hash_Table_Header)))->capacity;
}

u32 _hash_table_item_size(void *table) {
    return ((Hash_Table_Header *)(table - sizeof(Hash_Table_Header)))->item_size;
}

void _hash_table_free(void **table) {
    array_list_free(&(hash_table_header(table)->keys));
    buffer_data_struct_free(*table, sizeof(Hash_Table_Header));
    *table = NULL;
}

void _hash_table_resize_to_fit(void **table, u32 requiered_length) {
    Hash_Table_Header *header = hash_table_header(table);

    if (requiered_length > header->capacity) {
        // Before resizing, depricate slots.
        hash_table_depricate_slots(table);

        u32 capacity_multiplier = (u32)powf(2.0f, (float)((u32)(log2f((float)requiered_length / (float)header->capacity)) + 1));

        *table = buffer_data_struct_resize(*table, header->capacity * capacity_multiplier * (header->item_size + sizeof(Hash_Table_Slot)), sizeof(Hash_Table_Header));
        header = hash_table_header(table); // @Important: Resizing perfomed above changes the pointer to the table, so it is neccessary to reassign header ptr again, otherwise segfault occure.
        header->capacity *= capacity_multiplier; // @Important: Using "buffer_data_struct_resize" will not update capacity in the header, because this function is only designed to only resize the whole data structure, there for it is needed to manually set capacity to the right value, which was intended.
        
        /**
         * After resize, all data is copied and buffer is expanded to the right.
         * But due to the nature of hash table structure the slot area of the buffer would not be properly shifter in the resulting array.
         * For example diagrams (NOT TO SCALE):
         *
         *      BEFORE:
         *                  
         *                  |-----------------------------Buffer-Capacity-4-------------------------------|
         *                  |                                                                             |
         *                  |------Array-of-Data-------|-----------------Array-of-Slots-------------------|
         *      Indicies:   | 0     1     2     3      | 0           1           2           3            |
         *      Data:       | [1111][1111][1111][____] | [10][0x2342][01][0x2344][11][0x2348][__][______] |
         *                  ^
         *                  |
         *                  *table
         *
         *
         *      AFTER RESIZE (BAD):
         *                  
         *                  |-------------------------------------------------------------------Buffer-Capacity-8-----------------------------------------------------------------------|
         *                  |                                                                                                                                                           |
         *                  |------Array-of-Data-------|-----------------Array-of-Slots-------------------|--------------------------|--------------------------------------------------|
         *      Indicies:   | 0     1     2     3      | 0           1           2           3            |                          |                                                  |
         *      Data:       | [1111][1111][1111][____] | [10][0x2342][01][0x2344][11][0x2348][__][______] | [____][____][____][____] | [__][______][__][______][__][______][__][______] |
         *                  ^
         *                  |
         *                  *table
         *
         *
         *      WHAT IS EXPECTED / NEEDED (GOOD):
         *                  
         *                  |----------------------------------------------------------------Buffer-Capacity-8--------------------------------------------------------------------|
         *                  |                                                                                                                                                     |
         *                  |------------------Array-of-Data-------------------|------------------------------------------Array-of-Slots------------------------------------------|
         *      Indicies:   | 0     1     2     3     4     5     6     7      | 0           1           2           3           4           5           6           7            |
         *      Data:       | [1111][1111][1111][____][____][____][____][____] | [10][0x2342][01][0x2344][11][0x2348][__][______][__][______][__][______][__][______][__][______] |
         *                  ^
         *                  |
         *                  *table
         *
         * To achieve this good layout, it is needed to shift array of slots to the right.
         * Should be done with memmove cause, destination and source can overlap.
         */

        memmove(*table + header->capacity * header->item_size, *table + (header->capacity / capacity_multiplier) * header->item_size, (header->capacity / capacity_multiplier) * sizeof(Hash_Table_Slot));

        // Set new slots to SLOT_EMPTY.
        for (u32 i = header->capacity / capacity_multiplier; i < header->capacity; i++) {
            hash_table_get_slot(table, i)->state = SLOT_EMPTY;
        }

        // Readress slots after resizing.
        for (u32 i = 0; i < header->capacity; i++) {
            if (hash_table_get_slot(table, i)->state == SLOT_DEPRICATED) {
                (void)hash_table_readress(table, i);
            }
        }
    }

    if (header->capacity * (header->item_size + sizeof(Hash_Table_Slot)) < requiered_length * (header->item_size + sizeof(Hash_Table_Slot))) {
        printf_err("Table capacity size is less than requiered length size after being resized, capacity size: %llu, size of requiered length: %llu.\n", header->capacity * (header->item_size + sizeof(Hash_Table_Slot)), requiered_length * (header->item_size + sizeof(Hash_Table_Slot)));
    }
}

u32 _hash_table_push_key(void **table, void *key, u32 key_size) {
    u32 index = hash_table_hash_index_of(table, key, key_size);
    Hash_Table_Header *header = hash_table_header(table);

    Hash_Table_Slot *slot = NULL;
    for (u32 i = 0; i < header->capacity; i++) {
        slot = hash_table_get_slot(table, (index + i) % header->capacity);
        if (slot->state == SLOT_EMPTY) {
            // Write all data to hash table by corresponding index.
            slot->state = SLOT_OCCUPIED;

            slot->key.length = key_size;
            
            /**
             * @Important: "array_list_append_multiple" is not inlined because it might change the "header->keys" value when resizing array list.
             * So it is neccessary to call this macro like function in a separate line, otherwise undefined behaviour will occure.
             */
            u32 key_index = array_list_append_multiple(&(header->keys), key, key_size);
            slot->key.ptr = header->keys + key_index;

            header->count++;
           
            return (index + i) % header->capacity;
        }
        else if (slot->key.length == key_size && !memcmp(slot->key.ptr, key, key_size)) {
            // Key already exists, return index of that slot.
            return (index + i) % header->capacity;
        }
    }

    
    printf_err("Couldn't find free hash table slot for the new key.\n");
    return UINT_MAX;
}

u32 _hash_table_index_of(void **table, void *key, u32 key_size) {
    u32 index = hash_table_hash_index_of(table, key, key_size);
    Hash_Table_Header *header = hash_table_header(table);

    Hash_Table_Slot *slot = NULL;
    for (u32 i = 0; i < header->capacity; i++) {
        slot = hash_table_get_slot(table, (index + i) % header->capacity);
        if (slot->state == SLOT_EMPTY) {
            return UINT_MAX;
        }
        else if (slot->key.length == key_size && !memcmp(slot->key.ptr, key, key_size)) {
            // Right key is found, return.
            return (index + i) % header->capacity;
        }
    }

    return UINT_MAX;
}

void _hash_table_remove(void **table, void *key, u32 key_size) {
    u32 index = hash_table_hash_index_of(table, key, key_size);
    Hash_Table_Header *header = hash_table_header(table);

    Hash_Table_Slot *slot = NULL;
    for (u32 i = 0; i < header->capacity; i++) {
        slot = hash_table_get_slot(table, (index + i) % header->capacity);
        if (slot->state == SLOT_EMPTY) {
            return;
        }
        else if (slot->key.length == key_size && !memcmp(slot->key.ptr, key, key_size)) {
            // Right key is found.
            // @Incomplete: Doesn't delete key.
            header->count--;
            slot->state = SLOT_EMPTY;
            return;
        }
    }
}


u32 hashf(void *key, u32 key_size) {
    if (key == NULL || key_size == 0) {
        printf_err("Couldn't hash a NULL or 0 sized key.\n");
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

void hash_table_print(void **table) {
    Hash_Table_Header *header = hash_table_header(table);

    printf("\n--------\tHash Table\t--------\n");
    for (u32 i = 0; i < header->capacity; i++) {
        hash_table_print_slot(*table + i * header->item_size, header->item_size, hash_table_get_slot(table, i));
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
        printf_err("Couldn't open the file \"%s\".\n", file_name);
        return NULL;
    }

    (void)fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    rewind(file);

    char *buffer = allocator_alloc(allocator, file_size + 1);
    if (buffer == NULL) {
        printf_err("Memory allocation for string buffer failed while reading the file \"%s\".\n", file_name);
        (void)fclose(file);
        return NULL;
    }

    if (fread(buffer, 1, file_size, file) != file_size) {
        printf_err("Failure reading the file \"%s\".\n", file_name);
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
        printf_err("Couldn't open the file \"%s\".\n", file_name);
        return NULL;
    }

    (void)fseek(file, 0, SEEK_END);
    u64 size = ftell(file);
    rewind(file);

    u8 *buffer = allocator_alloc(allocator, size);
    if (buffer == NULL) {
        printf_err("Memory allocation for string buffer failed while reading the file \"%s\".\n", file_name);
        (void)fclose(file);
        return NULL;
    }

    if (fread(buffer, 1, size, file) != size) {
        printf_err("Failure reading the file \"%s\".\n", file_name);
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
        printf_err("Couldn't open the file \"%s\".\n", file_name);
    }

    (void)fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    rewind(file);

    String_8 str = str8_make_allocate((u32)file_size, allocator);

    if (str.ptr == NULL) {
        printf_err("Memory allocation for string buffer failed while reading the file \"%s\".\n", file_name);
        (void)fclose(file);
        str.length = 0;
    }

    if (fread(str.ptr, 1, file_size, file) != file_size) {
        printf_err("Failure reading the file \"%s\".\n", file_name);
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
        printf_err("OpenGL error: %d.\n", error);
        return false;
    }
    return true;
}

int init_sdl_gl() {
    // Initialize SDL.
    if (SDL_Init(SDL_INIT_VIDEO) < 0 || SDL_Init(SDL_INIT_AUDIO) < 0) {
        printf_err("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
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
        printf_err("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        return NULL;
    }

    // Create OpenGL context.
    SDL_GLContext glContext = SDL_GL_CreateContext(window);
    if (glContext == NULL) {
        printf_err("OpenGL context could not be created! SDL_Error: %s\n", SDL_GetError());
        return window;
    }

    if (SDL_GL_MakeCurrent(window, glContext) < 0) {
        printf_err("OpenGL current could not be created! SDL_Error: %s\n", SDL_GetError());
        return window;
    }

    // Initialize GLEW.
    if (glewInit() != GLEW_OK) {
        printf_err("GLEW initialization failed!\n");
        return window;
    }

    return window;
}

int init_sdl_audio() {
    // Initialize sound.
    if (Mix_Init(MIX_INIT_MP3) < 0) {
        printf_err("SDL Mixer could not initialize! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }

    (void)Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 1024);
    return 0;
}


// Shader loading key words.
String_8 shader_version_tag = str8_make("#version");
String_8 vertex_shader_defines = str8_make("#define VERTEX\n");
String_8 fragment_shader_defines = str8_make("#define FRAGMENT\n");


s32 shader_strings_lengths[3];

// Texture params.
s32 texture_min_filter = GL_CLAMP_TO_EDGE;
s32 texture_max_filter = GL_CLAMP_TO_EDGE;
s32 texture_wrap_s = GL_LINEAR;
s32 texture_wrap_t = GL_LINEAR;

// Shader unifroms.
Matrix4f shader_uniform_pr_matrix = MATRIX4F_IDENTITY;
Matrix4f shader_uniform_ml_matrix = MATRIX4F_IDENTITY;
s32 shader_uniform_samplers[32] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31 };

// Drawing variables.
float *verticies;
u32 *quad_indicies;
u32 texture_ids[32];

void graphics_init() {
    // Enable Blending (Rendering with alpha channels in mind).
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Make stbi flip images vertically when loading.
    stbi_set_flip_vertically_on_load(true);

    // Setting drawing variables.
    verticies = array_list_make(float, MAX_QUADS_PER_BATCH * VERTICIES_PER_QUAD * 11, &std_allocator); // @Leak
    quad_indicies = array_list_make(u32, MAX_QUADS_PER_BATCH * 6, &std_allocator); // @Leak
    
    // Initing quad indicies.
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
        printf_err("Stbi couldn't load image.\n");
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
        printf_warning("Couldn't get location of %s uniform, in shader with id: %d.\n", shader_uniform_pr_matrix_name, program->id);
    }

    s32 quad_shader_ml_matrix_loc = glGetUniformLocation(program->id, shader_uniform_ml_matrix_name);
    if (quad_shader_ml_matrix_loc == -1) {
        printf_warning("Couldn't get location of %s uniform, in shader with id: %d.\n", shader_uniform_ml_matrix_name, program->id);
    }

    s32 quad_shader_samplers_loc = glGetUniformLocation(program->id, shader_uniform_samplers_name);
    if (quad_shader_samplers_loc == -1) {
        printf_warning("Couldn't get location of %s uniform, in shader with id: %d.\n", shader_uniform_samplers_name, program->id);
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
        printf_err("Program of %s, failed to link.\n", shader_path);
       
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
        printf_err("Shader of %s, failed to compile.\n", shader_path);
        
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

    // Getting full shader file.
    String_8 shader_source = read_file_into_str8(shader_path, &std_allocator);
    
    // Splitting on two substrings, "shader_version" and "shader_code".
    u32 start_of_version_tag = str8_index_of(&shader_source, &shader_version_tag, 0, shader_source.length);
    u32 end_of_version_tag = str8_index_of_char(&shader_source, '\n', start_of_version_tag, shader_source.length);
    String_8 shader_version = str8_substring(&shader_source, start_of_version_tag, end_of_version_tag + 1);
    String_8 shader_code = str8_substring(&shader_source, end_of_version_tag + 1, shader_source.length);
    
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
        printf_err("Couldn't get location of %s uniform, in shader, when updating projection.\n", shader_uniform_pr_matrix_name);
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
    printf_err("Overflow of 32 texture slots limit, can't add texture id: %d, to current draw call texture slots.\n", texture->id);
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
    (void)array_list_append_multiple(&verticies, quad_data, count * VERTICIES_PER_QUAD * active_drawer->program->vertex_stride);
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
    (void)array_list_append_multiple(&verticies, line_data, count * VERTICIES_PER_LINE * active_line_drawer->program->vertex_stride);
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
