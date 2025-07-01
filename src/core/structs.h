#ifndef STRUCTS_H
#define STRUCTS_H

#include "core/core.h"
#include "core/type.h"

/**
 * Array list.
 */

#define array_list_make(type, capacity, ptr_allocator)              (type *)_array_list_make(sizeof(type), capacity, ptr_allocator)

#define array_list_length(ptr_list)                                 _array_list_length((void *)*ptr_list)
#define array_list_capacity(ptr_list)                               _array_list_capacity((void *)*ptr_list)
#define array_list_item_size(ptr_list)                              _array_list_item_size((void *)*ptr_list)

#define array_list_append(ptr_list, item)                           _array_list_resize_to_fit((void **)(ptr_list), array_list_length(ptr_list) + 1); (*ptr_list)[_array_list_next_index((void **)(ptr_list))] = item
#define array_list_append_multiple(ptr_list, item_arr, count)       _array_list_append_multiple((void **)ptr_list, (void *)item_arr, count)
#define array_list_pop(ptr_list)                                    _array_list_pop((void *)*ptr_list, 1)
#define array_list_pop_multiple(ptr_list, count)                    _array_list_pop((void *)*ptr_list, count)
#define array_list_clear(ptr_list)                                  _array_list_clear((void *)*ptr_list)
#define array_list_unordered_remove(ptr_list, index)                _array_list_unordered_remove((void *)*ptr_list, index)
#define array_list_free(ptr_list)                                   _array_list_free((void **)ptr_list)

typedef struct array_list_header {
    u32 capacity;
    u32 length;
    u32 item_size;
} Array_List_Header;

void *_array_list_make(u32 item_size, u32 initial_capacity, Allocator *allocator);
u32   _array_list_length(void *list);
u32   _array_list_capacity(void *list);
u32   _array_list_item_size(void *list);
void  _array_list_resize_to_fit(void **list, u32 requiered_length);
u32   _array_list_next_index(void **list);
u32   _array_list_append_multiple(void **list, void *items, u32 count);
void  _array_list_pop(void *list, u32 count);
void  _array_list_clear(void *list);
void  _array_list_unordered_remove(void *list, u32 index);
void  _array_list_free(void **list);

/**
 * Hash table.
 */


#define hash_table_make(type, capacity, allocator_ptr)              (type *)_hash_table_make(sizeof(type), capacity, allocator_ptr) 

#define hash_table_count(ptr_list)                                  _hash_table_count((void *)*ptr_list)
#define hash_table_capacity(ptr_list)                               _hash_table_capacity((void *)*ptr_list)
#define hash_table_item_size(ptr_list)                              _hash_table_item_size((void *)*ptr_list)

#define hash_table_put(ptr_table, item, ptr_key)          _hash_table_resize_to_fit((void **)(ptr_table), hash_table_count(ptr_table) + 1); (*ptr_table)[_hash_table_push_key((void **)(ptr_table), ptr_key, strlen(ptr_key))] = item
#define hash_table_get(ptr_table, ptr_key)                _hash_table_get((void **)(ptr_table), ptr_key, strlen(ptr_key))
#define hash_table_remove(ptr_table, ptr_key)             _hash_table_remove((void **)(ptr_table), ptr_key, strlen(ptr_key)) 
#define hash_table_free(ptr_table)                                  _hash_table_free((void **)(ptr_table))

typedef u32 (*Hashfunc)(void *, u32);

typedef struct hash_table_header {
    u32 capacity;
    u32 count;
    u32 item_size;
    Hashfunc hash_func;
    u8 *keys;
} Hash_Table_Header;

void *_hash_table_make(u32 item_size, u32 initial_capacity, Allocator *allocator);
u32   _hash_table_count(void *table);
u32   _hash_table_capacity(void *table);
u32   _hash_table_item_size(void *table);
void  _hash_table_resize_to_fit(void **table, u32 requiered_length);
u32   _hash_table_push_key(void **table, void *key, u32 key_size);
void *_hash_table_get(void **table, void *key, u32 key_size);
void  _hash_table_remove(void **table, void *key, u32 key_size);
void  _hash_table_free(void **table);

u32 hashf(void *key, u32 key_size);

void hash_table_print(void **table);



#endif
