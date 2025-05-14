#ifndef CORE_H
#define CORE_H

#include <stdbool.h>

/**
 * Debug.
 */

#include <stdio.h>

extern const char *debug_error_str;
extern const char *debug_warning_str;
extern const char *debug_ok_str;


#define printf_err(format, ...)                 (void)fprintf(stderr, "%s " format, debug_error_str, ##__VA_ARGS__);

#ifdef IGNORE_WARNING

#define printf_warning(format, ...)

#else

#define printf_warning(format, ...)             (void)fprintf(stderr, "%s " format, debug_warning_str, ##__VA_ARGS__);

#endif

#define printf_ok(format, ...)                  (void)fprintf(stderr, "%s " format, debug_ok_str, ##__VA_ARGS__);

/**
 * Integer typedefs.
 */

#include <stdint.h>

typedef int8_t      s8;
typedef uint8_t	    u8;
typedef int16_t     s16;
typedef uint16_t    u16;
typedef int32_t     s32;
typedef uint32_t    u32;
typedef int64_t     s64;
typedef uint64_t    u64;


/**
 * Allocators.
 * @Important: There are different ways to approach memory allocation situation, using malloc() calloc() and realloc() is all great, and if used correctly can benefit the program flow.
 * The problem that arises is when you want to have more flexibility over memory use.
 * For example - arena allocator uses pre-allocated memory to linearly distribute this "prepared" memory.
 * And ideally all allocations of such should be easily done, without any overhead.
 * So the use of dynamic memory becomes more convinient and logical when deciding what to use Stack or Heap.
 * It is important that the whole process doesn't become super simple, since it is not about making garbade collection system, but about making the whole process more organized and easy to follow.
 * This is why free(), malloc(), calloc(), realloc() and other std calls is fine to use. It is even convinient to provide an Allocator instance that directly calls this functions.
 * The idea is to have a workflow where if you allocate dynamically something in the function, and the function returns it to you, you pass your allocator of choice to do this job.
 *
 * Arena allocator.
 */


#include <stdlib.h>

typedef struct allocator_header {
    u64 capacity;
    u64 size_filled;
} Allocator_Header;

typedef void *(*Allocate)(Allocator_Header *header, u64 size);
typedef void *(*Zero_Allocate)(Allocator_Header *header, u64 size);
typedef void *(*Re_Allocate)(Allocator_Header *header, void *ptr, u64 size);
typedef void  (*Free)(Allocator_Header *header, void *ptr);

typedef struct allocator {
    Allocator_Header    *ptr;
    Allocate            alc_alloc;
    Zero_Allocate       alc_zero_alloc;
    Re_Allocate         alc_re_alloc;
    Free                alc_free;
} Allocator;

// Std.
extern Allocator std_allocator;

// Arena.
typedef Allocator Arena_Allocator;

Arena_Allocator arena_make(u64 capacity);
void arena_destroy(Arena_Allocator *arena);

// Allocator interface.
void *allocator_alloc(Allocator *allocator, u64 size);
void *allocator_zero_alloc(Allocator *allocator, u64 size);
void *allocator_re_alloc(Allocator *allocator, void *ptr, u64 size);
void  allocator_free(Allocator *allocator, void *ptr);


/**
 * String.
 */

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
 * Hashmap.
 */


#define hash_table_make(type, capacity, allocator_ptr)              (type *)_hash_table_make(sizeof(type), capacity, allocator_ptr) 

#define hash_table_count(ptr_list)                                  _hash_table_count((void *)*ptr_list)
#define hash_table_capacity(ptr_list)                               _hash_table_capacity((void *)*ptr_list)
#define hash_table_item_size(ptr_list)                              _hash_table_item_size((void *)*ptr_list)

#define hash_table_put(ptr_table, item, key_ptr, key_size)          _hash_table_resize_to_fit((void **)(ptr_table), hash_table_count(ptr_table) + 1); (*ptr_table)[_hash_table_push_key((void **)(ptr_table), key_ptr, key_size)] = item
#define hash_table_get(ptr_table, ptr_key, key_size)                _hash_table_get((void **)(ptr_table), ptr_key, key_size)
#define hash_table_remove(ptr_table, ptr_key, key_size)             _hash_table_remove((void **)(ptr_table), ptr_key, key_size) 
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


/**
 * Math float.
 */

#include <math.h>

#define PI 3.14159265358979323846f
#define TAU (PI * 2)

#define deg2rad(deg)                        ((deg) * PI / 180.0f)
#define rad2deg(rad)                        ((rad) * 180.0f / PI)

#define right_triangle_hypotenuse(a, b)     (sqrtf((a) * (a) + (b) * (b)))
#define sig(a)                              (((a) == 0.0f) ? (0.0f) : (fabsf(a) / (a)))
#define fequal(a, b)                        (fabsf((a) - (b)) < FLT_EPSILON)

#define randf()                             ((float)rand() / RAND_MAX)


#define lerp(a, b, t)                       ((a) + ((b) - (a)) * (t))
#define ease_in_back(x)                     (((1.70158f + 1.0f) * (x) * (x) * (x)) - (1.70158f * (x) * (x)))



typedef struct vec2f {
    float x;
    float y;
} Vec2f;

#define vec2f_make(x, y)                    ((Vec2f) { x, y } )
#define vec2f_make_angle(mag, angle)        ((Vec2f) { (mag) * cosf(angle), (mag) * sinf(angle) } )

#define VEC2F_ORIGIN                        ((Vec2f) { 0.0f,  0.0f } )
#define VEC2F_RIGHT                         ((Vec2f) { 1.0f,  0.0f } )
#define VEC2F_LEFT                          ((Vec2f) {-1.0f,  0.0f } )
#define VEC2F_UP                            ((Vec2f) { 0.0f,  1.0f } )
#define VEC2F_DOWN                          ((Vec2f) { 0.0f, -1.0f } )
#define VEC2F_UNIT                          ((Vec2f) { 1.0f,  1.0f } )

#define vec2f_print(v1)                     printf(#v1 " = ( % 2.2f , % 2.2f )\n", v1.x, v1.y)

#define vec2f_sum(v1, v2)                   vec2f_make(v1.x + v2.x, v1.y + v2.y)
#define vec2f_difference(v1, v2)            vec2f_make(v1.x - v2.x, v1.y - v2.y) 
#define vec2f_sum_constant(v1, c)           vec2f_make(v1.x + (c), v1.y + (c))
#define vec2f_difference_constant(v1, c)    vec2f_make(v1.x - (c), v1.y - (c)) 
#define vec2f_dot(v1, v2)                   (v1.x * v2.x + v1.y * v2.y)
#define vec2f_cross(v1, v2)                 (v1.x * v2.y - v1.y * v2.x)
#define vec2f_multi_constant(v1, c)         vec2f_make(v1.x * (c), v1.y * (c))
#define vec2f_divide_constant(v1, c)        vec2f_make(v1.x / (c), v1.y / (c))
#define vec2f_magnitude(v1)                 right_triangle_hypotenuse(v1.x, v1.y)
#define vec2f_distance(v1, v2)              right_triangle_hypotenuse(v1.x - v2.x, v1.y - v2.y)
#define vec2f_negate(v1)                    vec2f_make(-v1.x, -v1.y)
#define vec2f_normalize(v1)                 (((v1).x == 0.0f && (v1).y == 0.0f) ? VEC2F_ORIGIN : vec2f_divide_constant(v1, vec2f_magnitude(v1)))
#define vec2f_lerp(v1, v2, t)               vec2f_make(lerp(v1.x, v2.x, t), lerp(v1.y, v2.y, t))
#define vec2f_rotate(v1, angle)             vec2f_make(cosf(angle) * v1.x - sinf(angle) * v1.y, sinf(angle) * v1.x + cosf(angle) * v1.y)

float point_segment_min_distance(Vec2f p, Vec2f a, Vec2f b);

typedef struct vec3f {
    float x;
    float y;
    float z;
} Vec3f;

#define vec3f_make(x, y, z)                 ((Vec3f) { x, y, z } )

#define vec3f_print(v1)                     printf(#v1 " = ( % 2.2f , % 2.2f , %2.2f )\n", v1.x, v1.y, v1.z)

typedef struct vec4f {
    float x;
    float y;
    float z;
    float w;
} Vec4f;

#define vec4f_make(x, y, z, w)              ((Vec4f) { x, y, z, w } )

#define VEC4F_WHITE                         ((Vec4f) { 1.0f,  1.0f,  1.0f,  1.0f } )
#define VEC4F_BLACK                         ((Vec4f) { 0.0f,  0.0f,  0.0f,  1.0f } )
#define VEC4F_GREY                          ((Vec4f) { 0.4f,  0.4f,  0.4f,  1.0f } )
#define VEC4F_RED                           ((Vec4f) { 1.0f,  0.0f,  0.0f,  1.0f } )
#define VEC4F_GREEN                         ((Vec4f) { 0.0f,  1.0f,  0.0f,  1.0f } )
#define VEC4F_BLUE                          ((Vec4f) { 0.0f,  0.0f,  1.0f,  1.0f } )
#define VEC4F_YELLOW                        ((Vec4f) { 1.0f,  1.0f,  0.0f,  1.0f } )
#define VEC4F_PINK                          ((Vec4f) { 1.0f,  0.0f,  1.0f,  1.0f } )
#define VEC4F_CYAN                          ((Vec4f) { 0.0f,  1.0f,  1.0f,  1.0f } )

#define vec4f_print(v1)                     printf(#v1 " = ( % 2.2f , % 2.2f , % 2.2f , % 2.2f )\n", v1.x, v1.y, v1.z, v1.w)

typedef struct matrix4f {
    float array[16];
} Matrix4f;

#define MATRIX4F_ZERO                                                   ((Matrix4f) {{ 0.0f, 0.0f, 0.0f, 0.0f,   0.0f, 0.0f, 0.0f, 0.0f,    0.0f, 0.0f, 0.0f, 0.0f,    0.0f, 0.0f, 0.0f, 0.0f }} )
#define MATRIX4F_IDENTITY                                               ((Matrix4f) {{ 1.0f, 0.0f, 0.0f, 0.0f,   0.0f, 1.0f, 0.0f, 0.0f,    0.0f, 0.0f, 1.0f, 0.0f,    0.0f, 0.0f, 0.0f, 1.0f }} )

#define matrix4f_print(matrix)                                          printf(#matrix " =\n[[ % 2.2f , % 2.2f , % 2.2f , % 2.2f ]\n [ % 2.2f , % 2.2f , % 2.2f , % 2.2f ]\n [ % 2.2f , % 2.2f , % 2.2f , % 2.2f ]\n [ % 2.2f , % 2.2f , % 2.2f , % 2.2f ]]\n", matrix.array[0], matrix.array[1], matrix.array[2], matrix.array[3], matrix.array[4], matrix.array[5], matrix.array[6], matrix.array[7], matrix.array[8], matrix.array[9], matrix.array[10], matrix.array[11], matrix.array[12], matrix.array[13], matrix.array[14], matrix.array[15])

#define matrix4f_orthographic(left, right, bottom, top, near, far)     ((Matrix4f) {{ 2.0f / ((right) - (left)), 0.0f, 0.0f, ((left) + (right)) / ((left) - (right)),    0.0f, 2.0f / ((top) - (bottom)), 0.0f, ((bottom) + (top)) / ((bottom) - (top)),    0.0f, 0.0f, 2.0f / ((near) - (far)), ((near) + (far)) / ((near) - (far)),    0.0f, 0.0f, 0.0f, 1.0f }} )

#define matrix4f_vec2f_multiplication(matrix, vec2)                     ((Vec2f) { .x = matrix.array[0] * vec2.x + matrix.array[1] * vec2.y + matrix.array[3],    .y = matrix.array[4] * vec2.x + matrix.array[5] * vec2.y + matrix.array[7], } )
#define matrix4f_vec3f_multiplication(matrix, vec3)                     ((Vec3f) { .x = matrix.array[0] * vec3.x + matrix.array[1] * vec3.y + matrix.array[2] * vec3.z,    .y = matrix.array[4] * vec3.x + matrix.array[5] * vec3.y + matrix.array[6] * vec3.z,    .z = matrix.array[8] * vec3.x + matrix.array[9] * vec3.y + matrix.array[10] * vec3.z, } )
#define matrix4f_vec4f_multiplication(matrix, vec4)                     ((Vec4f) { .x = matrix.array[0] * vec4.x + matrix.array[1] * vec4.y + matrix.array[2] * vec4.z + matrix.array[3] * vec4.w,    .y = matrix.array[4] * vec4.x + matrix.array[5] * vec4.y + matrix.array[6] * vec4.z + matrix.array[7] * vec4.w,    .z = matrix.array[8] * vec4.x + matrix.array[9] * vec4.y + matrix.array[10] * vec4.z + matrix.array[11] * vec4.w,    .w = matrix.array[12] * vec4.x + matrix.array[13] * vec4.y + matrix.array[14] * vec4.z + matrix.array[15] * vec4.w, } )

Matrix4f matrix4f_multiplication(Matrix4f *multiplier, Matrix4f *target);

typedef Matrix4f Transform;

#define transform_make_translation_2d(position)                              ((Transform) {{ 1.0f, 0.0f, 0.0f, position.x,   0.0f, 1.0f, 0.0f, position.y,    0.0f, 0.0f, 1.0f, 0.0f,    0.0f, 0.0f, 0.0f, 1.0f }} )
#define transform_make_rotation_2d(angle)                                    ((Transform) {{ cosf(angle), -sinf(angle), 0.0f, 0.0f,    sinf(angle),  cosf(angle), 0.0f, 0.0f,    0.0f, 0.0f, 1.0f, 0.0f,    0.0f, 0.0f, 0.0f, 1.0f }} )
#define transform_make_scale_2d(scale)                                       ((Transform) {{ scale.x, 0.0f, 0.0f, 0.0f,   0.0f, scale.y, 0.0f, 0.0f,    0.0f, 0.0f, 1.0f, 0.0f,    0.0f, 0.0f, 0.0f, 1.0f }} )

// Transform transform_make_srt_2d(Vec2f position, float angle, Vec2f scale);
Transform transform_make_trs_2d(Vec2f position, float angle, Vec2f scale);

void transform_set_rotation_2d(Transform *transform, float angle);
void transform_set_translation_2d(Transform *transform, Vec2f position);
void transform_flip_y(Transform *transform);
void transform_flip_x(Transform *transform);



typedef float (*Function)(float);

#define func_expr(expr) ({ float __fn(float x) { return (expr); } (Function)__fn; })



/**
 * This macro uses [ domain_start, domain_end ] to specify domain boundaries, and checks if value inside that domain.
 */
#define value_inside_domain(domain_start, domain_end, value) ((domain_start) <= (value) && (value) <= (domain_end))


/**
 * Normals and verticies for all boxes, without rotation.
 *
 *                         up
 *                         ^
 *                         |
 *                         |
 *                         |
 *                p3----------------p1
 *                |                 |
 *                |                 | 
 *  left <--------|        o        |--------> right
 *                |                 |
 *                |                 |
 *                p0----------------p2
 *                         |
 *                         |
 *                         |
 *                         !
 *                         down
 *
 */


typedef struct axis_aligned_bounding_box {
    Vec2f p0;
    Vec2f p1;
} AABB;

#define aabb_make(p0, p1)                                               ((AABB) { p0, p1 } )   
#define aabb_make_dimensions(center, width, height)                     ((AABB) { .p0 = vec2f_make((center).x - (width) / 2, (center).y - (height) / 2), .p1 = vec2f_make((center).x + (width) / 2, (center).y + (height) / 2) } )   
#define aabb_center(aabb)                                               vec2f_make(aabb.p0.x + (aabb.p1.x - aabb.p0.x) / 2, aabb.p0.y + (aabb.p1.y - aabb.p0.y) / 2)

void aabb_move(AABB *box, Vec2f move);


typedef struct oriented_bounding_box {
    Vec2f center;
    Vec2f dimensions;
    float rot;
} OBB;

#define obb_make(center, width, height, rotation)                       ((OBB) { center, vec2f_make(width, height), rotation })

#define obb_p0(obb_ptr)                                                 vec2f_sum((obb_ptr)->center, vec2f_rotate(vec2f_make((obb_ptr)->dimensions.x / 2, (obb_ptr)->dimensions.y / 2), (obb_ptr)->rot))
#define obb_p1(obb_ptr)                                                 vec2f_sum((obb_ptr)->center, vec2f_rotate(vec2f_make(-(obb_ptr)->dimensions.x / 2, -(obb_ptr)->dimensions.y / 2), (obb_ptr)->rot))
#define obb_p2(obb_ptr)                                                 vec2f_sum((obb_ptr)->center, vec2f_rotate(vec2f_make((obb_ptr)->dimensions.x / 2, -(obb_ptr)->dimensions.y / 2), (obb_ptr)->rot))
#define obb_p3(obb_ptr)                                                 vec2f_sum((obb_ptr)->center, vec2f_rotate(vec2f_make(-(obb_ptr)->dimensions.x / 2, (obb_ptr)->dimensions.y / 2), (obb_ptr)->rot))

#define obb_right(obb_ptr)                                              vec2f_rotate(VEC2F_RIGHT, (obb_ptr)->rot)
#define obb_up(obb_ptr)                                                 vec2f_rotate(VEC2F_UP, (obb_ptr)->rot) 
#define obb_left (obb_ptr)                                              vec2f_rotate(VEC2F_LEFT, (obb_ptr)->rot)
#define obb_down (obb_ptr)                                              vec2f_rotate(VEC2F_DOWN, (obb_ptr)->rot)

/**
 * Returns AABB that covers or/and encloses specified OBB box.
 */
AABB obb_enclose_in_aabb(OBB *box);


typedef struct circle {
    Vec2f center;
    float radius;
} Circle;


/**
 * File utils.
 */

/**
 * @Incomplete: Write description.
 */
char *read_file_into_string(char *file_name, Allocator *allocator);
    
/**
 * Reads contents of the file into the buffer that is preemptivly allocated using malloc.
 * Return pointer to the buffer and sets buffer size in bytes into the file_size.
 * @Important: Buffer should be freed manually when not used anymore.
 */
void *read_file_into_buffer(char *file_name, u64 *file_size, Allocator *allocator);

/**
 * @Incomplete: Write description.
 */
String_8 read_file_into_str8(char *file_name, Allocator *allocator);


/**
 * Graphics.
 */

#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_video.h>
#include <SDL2/SDL_audio.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_keycode.h>

#include "stb_truetype.h"

/**
 * Input
 */

typedef struct mouse_input {
    Vec2f position;
    bool left_hold;
    bool left_pressed;
    bool left_unpressed;
    bool right_hold;
    bool right_pressed;
    bool right_unpressed;
} Mouse_Input;

void keyboard_state_init();

void keyboard_state_old_update();

void keyboard_state_free();


bool is_hold_keycode(SDL_KeyCode keycode);

bool is_pressed_keycode(SDL_KeyCode keycode);

bool is_unpressed_keycode(SDL_KeyCode keycode);


/**
 * @Incomplete: Write description.
 */
bool check_gl_error();

/**
 * @Incomplete: Write description.
 */
int init_sdl_gl();

/**
 * @Incomplete: Write description.
 */
SDL_Window* create_gl_window(const char *title, int x, int y, int width, int height);

/**
 * @Incomplete: Write description.
 */
int init_sdl_audio();


/**
 * @Incomplete: Write description.
 */
void graphics_init();


typedef struct texture {
    u32 id;             // OpenGL texture id.
    s32 width;          // Pixel width of texture.
    s32 height;         // Pixel height of texture.
} Texture;

typedef struct uv_region {
    Vec2f uv0;
    Vec2f uv1;
} UV_Region;

#define UV_DEFAULT          ((UV_Region)({ .uv0 = VEC2F_ORIGIN, .uv1 = VEC2F_UNIT }))

/**
 * Loads texture from image file and returns struct that contains it's OpenGL id with other texture parameters.
 */
Texture texture_load(char *texture_path);

/**
 * Deletes loaded OpenGL texture.
 */
void texture_unload(Texture *texture);

/**
 * Returns uv region that corresponds to the slice index of the texture that is sliced on grid of specified rows and cols.
 *
 *  If texture_example:
 *      |-------|-------|-------|
 *      |   0   |   1   |   2   |
 *      |-------|-------|-------|
 *      |   3   |   4   |   5   |
 *      |-------|-------|-------|
 *      |   6   |   7   |   8   |
 *      |-------|-------|-------|
 *  Where 0, 1, 2, ... , 8 are indexes of each slice.
 *  rows = 3, cols = 3, total count of slices is always rows * cols, 3 * 3 = 9.
 *  
 *  For example:
 *      UV_Region uv_reg = texture_slice(3, 3, 4);
 *
 *  Will return UV_Region that can be used to extract UV coordinates of center slice in the example_texture above.
 */
UV_Region uv_slice(u32 rows, u32 cols, u32 index);


typedef struct shader {
    u32 id;             // OpenGL program id.
    u32 vertex_stride;  // Stride length in bytes needed to be allocated per vertex for shader to run correctly for each vertex.
} Shader;

/**
 * Loads shader from .glsl file and returns struct that contains it's OpenGL id.
 */
Shader shader_load(char *shader_path);

/**
 * Deletes loaded OpenGL program.
 */
void shader_unload(Shader *shader);

void shader_init_uniforms(Shader *shader);


typedef struct quad_drawer {
    u32     vao;         // OpenGL id of Vertex Array Object.
    u32     vbo;         // OpenGL id of Vertex Buffer Object.
    u32     ebo;         // OpenGL id of Element Buffer Object.
    Shader  *program;    // Pointer to shader that will be used to draw.
} Quad_Drawer;

#define MAX_QUADS_PER_BATCH     32
#define VERTICIES_PER_QUAD      4
#define INDICIES_PER_QUAD       6

/**
 * @Incomplete: Write description.
 */
void drawer_init(Quad_Drawer *drawer, Shader *shader);

/**
 * @Incomplete: Write description.
 */
void drawer_free(Quad_Drawer *drawer);

/**
 * @Incomplete: Write description.
 */
float add_texture_to_slots(Texture *texture);


/**
 * Draws quad data to the screen that is stored in the buffer.
 * For example: "draw_end()" uses this function to draw verticies that are stored inside verticies array.
 */

void draw_buffer(Quad_Drawer *drawer, void *buffer, u32 size);
/**
 * @Incomplete: Write description.
 */
void draw_begin(Quad_Drawer *drawer);

/**
 * @Incomplete: Write description.
 */
void draw_end();


/**
 * Simply places specified data directly into the verticies array.
 */
void draw_quad_data(float *quad_data, u32 count);


void print_verticies();

void print_indicies();


typedef struct camera {
    Vec2f   center;         // World center.
    u32     unit_scale;     // Pixels per 1 world unit.
} Camera;

Camera camera_make(Vec2f center, u32 unit_scale);

/**
 * Calculate 4x4 projection matrix that can be used by shaders to correctly transform then draw elements to the screen.
 */
Matrix4f camera_calculate_projection(Camera *camera, float window_width, float window_height);

#define screen_calculate_projection(window_width, window_height) ((Matrix4f) { .array = { 2.0f / (float)(window_width), 0.0f, 0.0f, -1.0f,    0.0f, 2.0f / (float)(window_height), 0.0f, -1.0f,    0.0f, 0.0f, 1.0f, 0.0f,    0.0f, 0.0f, 0.0f, 1.0f } })

/**
 * Sets shader uniform projection matrix to the specified 4x4 matrix.
 */
void shader_update_projection(Shader *shader, Matrix4f *projection);


typedef struct font_baked {
    stbtt_bakedchar *chars;
    s32             chars_count;                // Number of chars baked.
    s32             first_char_code;            // ASCII value of the first character baked.
    s32             baseline;
    s32             line_height;
    Texture         bitmap;
} Font_Baked;

Font_Baked font_bake(u8 *font_data, float font_size);

void font_free(Font_Baked *font);


typedef struct line_drawer {
    u32     vao;         // OpenGL id of Vertex Array Object.
    u32     vbo;         // OpenGL id of Vertex Buffer Object.
    Shader  *program;    // Pointer to shader that will be used to draw.
} Line_Drawer;

#define MAX_LINES_PER_BATCH     1024
#define VERTICIES_PER_LINE      2

/**
 * @Incomplete: Write description.
 */
void line_drawer_init(Line_Drawer *drawer, Shader *shader);

/**
 * @Incomplete: Write description.
 */
void line_drawer_free(Line_Drawer *drawer);

/**
 * @Incomplete: Write description.
 */
void line_draw_begin(Line_Drawer *drawer);

/**
 * @Incomplete: Write description.
 */
void line_draw_end();

/**
 * @Incomplete: Write description.
 */
void draw_line_data(float *line_data, u32 count);


/**
 * Time.
 */
typedef struct time_data {
    u32 current_time;
    u32 delta_time_milliseconds;
    float delta_time;

    float delta_time_multi;
    u32 time_slow_factor;

    u32 last_update_time;
    u32 accumilated_time;
    u32 update_step_time;
} Time_Data;


typedef struct time_interpolator {
    float duration;
    float elapsed_t;
} T_Interpolator;

#define ti_make(duration)                   ((T_Interpolator) { duration, 0.0f })\

void ti_update(T_Interpolator *interpolator, float delta_time);
float ti_elapsed_percent(T_Interpolator *interpolator);




#endif
