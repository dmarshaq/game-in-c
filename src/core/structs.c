#include "core/structs.h"
#include "core/core.h"
#include "core/type.h"
#include "core/mathf.h"
#include "core/str.h"

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
    String key;
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
    (void)printf(" | State: 0x%02x | Key Size: 0x%08x | Key Ptr: 0x%16p -> ", slot->state, slot->key.length, slot->key.data);

    if (slot->state == SLOT_OCCUPIED) {
        
        // Print key itself
        (void)printf("%.*s", slot->key.length, slot->key.data);
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

    u32 new_index = hash_table_hash_index_of(table, target_slot->key.data, target_slot->key.length);
    target_slot->state = SLOT_EMPTY;

    Hash_Table_Slot *slot = NULL;
    for (u32 i = 0; i < header->capacity; i++) {
        slot = hash_table_get_slot(table, (new_index + i) % header->capacity);
        if (slot->state == SLOT_EMPTY) {
            // Copy data to a new slot.
            slot->state = SLOT_OCCUPIED;

            slot->key.length = target_slot->key.length;
            slot->key.data = target_slot->key.data;

            memmove(*table + ((new_index + i) % header->capacity) * header->item_size, *table + index * header->item_size, header->item_size);

            return true;
        }
        else if (slot->state == SLOT_DEPRICATED && hash_table_readress(table, (new_index + i) % header->capacity)) {

            // @Important: There is an additional is SLOT_OCCUPIED check, because hash_table_readress can possbily readress slot to the same index as it was before, therefore additional check is needed.
            if (slot->state != SLOT_OCCUPIED) {
                // Copy data to a new slot.
                slot->state = SLOT_OCCUPIED;

                slot->key.length = target_slot->key.length;
                slot->key.data = target_slot->key.data;

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
            slot->key.data = header->keys + key_index;

            header->count++;
           
            return (index + i) % header->capacity;
        }
        else if (slot->key.length == key_size && !memcmp(slot->key.data, key, key_size)) {
            // Key already exists, return index of that slot.
            return (index + i) % header->capacity;
        }
    }

    
    printf_err("Couldn't find free hash table slot for the new key.\n");
    return UINT_MAX;
}

void *_hash_table_get(void **table, void *key, u32 key_size) {
    u32 index = hash_table_hash_index_of(table, key, key_size);
    Hash_Table_Header *header = hash_table_header(table);

    Hash_Table_Slot *slot = NULL;
    for (u32 i = 0; i < header->capacity; i++) {
        slot = hash_table_get_slot(table, (index + i) % header->capacity);
        if (slot->state == SLOT_EMPTY) {
            return NULL;
        }
        else if (slot->key.length == key_size && !memcmp(slot->key.data, key, key_size)) {
            // Right key is found, return.
            return *table + ((index + i) % header->capacity) * header->item_size;
        }
    }

    return NULL;
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
        else if (slot->key.length == key_size && !memcmp(slot->key.data, key, key_size)) {
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


