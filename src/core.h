#ifndef CORE_H
#define CORE_H

#include <stdbool.h>

/**
 * Debug.
 */

#include <stdio.h>

extern const char* debug_error_str;
extern const char* debug_warning_str;
extern const char* debug_ok_str;

/**
 * Integer typedefs.
 */

#include <stdint.h>

typedef int8_t      s8;
typedef uint8_t		u8;
typedef int16_t	    s16;
typedef uint16_t	u16;
typedef int32_t		s32;
typedef uint32_t	u32;
typedef int64_t     s64;
typedef uint64_t    u64;

/**
 * Arena allocator.
 */

#include <stdlib.h>

typedef struct arena_allocator {
    void *ptr;
    u64 size;
    u64 size_filled;
} Arena;

void arena_init(Arena *arena, u64 size);
void* arena_allocate(Arena *arena, u64 size);
void arena_free(Arena *arena);

/**
 * String.
 */

#include <string.h>

/**
 * The idea behind String_8 is to make working with dynamically allocated strings easier.
 * They are NOT null terminated, but are designed to be compatible with regural C strings, if used correctly.
 */
typedef struct string_8 {
    u8 *ptr;
    u32 length;
} String_8;

/**
 * Points "str" to statically created string, and modified "str" length accordingly.
 * Note: "str" will not contain null terminator at the end.
 */
void str8_init_statically(String_8 *str, const char *string);

/**
 * Allocates memory located in "arena" to "str", where "length" is new length of the string being allocated in "arena".
 * @Important: This allocated memory, doesn't have to be freed separatly, since when you free the "arena" this memory will be freed automatically.
 * Note: After you free the "arena", "str" will point to invalid memory adress.
 */
void str8_init_arena(String_8 *str, Arena *arena, u32 length);

/**
 * Allocates memory for the "str" through "malloc()" function, where "length" is new length of the string being allocated.
 * Note: Memory should be freed through "str8_free()" after it is not longer used.
 */
void str8_init_dynamically(String_8 *str, u32 length);

/**
 * Frees previously allocated memory through "malloc()", this includes "str8_init_dynamically()" since it also uses "malloc()" for memory allocation.
 */
void str8_free(String_8 *str);

void str8_memcopy_from_buffer(String_8 *str, void *buffer, u32 length);

void str8_memcopy_into_buffer(String_8 *str, void *buffer, u32 start, u32 end);

/**
 * Modifies "output_str", so it points to the memory of original "str" at index "start" with length up untill index "end".
 * Note: Character at index "end" is not included in "output_str", domain for resulting substring is always [ start, end ).
 * @Important: DOESN'T COPY MEMORY. If "str" memory is freed later, "output_str" will not point to valid adress anymore.
 */
void str8_substring(String_8 *str, String_8 *output_str, u32 start, u32 end);

bool str8_equals_str8(String_8 *str1, String_8 *str2);

bool str8_equals_string(String_8 *str, char *string);

/**
 * Linearly searches for the first occurnse of "search_str" in "str", by comparing them through "str8_equals_str8()" function.
 * Returns the index of first character of the occurns, otherwise, returns UINT_MAX.
 * "start" refers to the index where searching begins, character at this index IS included in the search comparising.
 * "end" refers to the index where searching stops, character at this index is NOT included in the search comparising.
 */
u32 str8_index_of_str8(String_8 *str, String_8 *search_str, u32 start, u32 end);

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

#define array_list_make(type, capacity)                             (type *)_array_list_make(sizeof(type), capacity)
#define array_list_length(ptr_list)                                 _array_list_length((void*)*ptr_list)
#define array_list_capacity(ptr_list)                               _array_list_capacity((void*)*ptr_list)
#define array_list_item_size(ptr_list)                              _array_list_item_size((void*)*ptr_list)
#define array_list_append(ptr_list, ptr_item)                       _array_list_append((void**)ptr_list, (void*)ptr_item, 1)
#define array_list_append_multiple(ptr_list, item_arr, count)       _array_list_append((void**)ptr_list, (void*)item_arr, count)
#define array_list_pop(ptr_list)                                    _array_list_pop((void*)*ptr_list, 1)
#define array_list_pop_multiple(ptr_list, count)                    _array_list_pop((void*)*ptr_list, count)
#define array_list_clear(ptr_list)                                  _array_list_clear((void*)*ptr_list)
#define array_list_free(ptr_list)                                   _array_list_free((void**)ptr_list)
#define array_list_unordered_remove(ptr_list, index)                _array_list_unordered_remove((void*)*ptr_list, index)

typedef struct array_list_header {
    u32 capacity;
    u32 length;
    u32 item_size;
} Array_List_Header;

void*   _array_list_make(u32 item_size, u32 capacity);
u32     _array_list_length(void *list);
u32     _array_list_capacity(void *list);
u32     _array_list_item_size(void *list);
void    _array_list_append(void **list, void *item, u32 count);
void    _array_list_pop(void *list, u32 count);
void    _array_list_clear(void *list);
void    _array_list_free(void **list);
void    _array_list_unordered_remove(void *list, u32 index);

/**
 * Math float.
 */

#include <math.h>

#define PI 3.14159265358979323846f
#define TAU (PI * 2)

#define right_triangle_hypotenuse(a, b)     (sqrtf((a) * (a) + (b) * (b)))

typedef struct vec2f {
    float x;
    float y;
} Vec2f;

#define vec2f_make(x, y)                    ((Vec2f) { x, y })

#define VEC2F_ORIGIN                        ((Vec2f) { 0.0f,  0.0f})
#define VEC2F_RIGHT                         ((Vec2f) { 1.0f,  0.0f})
#define VEC2F_LEFT                          ((Vec2f) {-1.0f,  0.0f})
#define VEC2F_UP                            ((Vec2f) { 0.0f,  1.0f})
#define VEC2F_DOWN                          ((Vec2f) { 0.0f, -1.0f})

#define vec2f_print(v1)                     printf(#v1 " = ( %2.2f , %2.2f )\n", v1.x, v1.y)

#define vec2f_sum(v1, v2)                   vec2f_make(v1.x + v2.x, v1.y + v2.y)
#define vec2f_difference(v1, v2)            vec2f_make(v1.x - v2.x, v1.y - v2.y) 
#define vec2f_sum_constant(v1, c)           vec2f_make(v1.x + c, v1.y + c)
#define vec2f_difference_constant(v1, c)    vec2f_make(v1.x - c, v1.y - c) 
#define vec2f_dot(v1, v2)                   (v1.x * v2.x + v1.y * v2.y)
#define vec2f_multi_constant(v1, c)         vec2f_make(v1.x * c, v1.y * c)
#define vec2f_divide_constant(v1, c)        vec2f_make(v1.x / c, v1.y / c)
#define vec2f_magnitude(v1)                 right_triangle_hypotenuse(v1.x, v1.y)
#define vec2f_distance(v1, v2)              right_triangle_hypotenuse(v1.x - v2.x, v1.y - v2.y)


typedef struct vec3f {
    float x;
    float y;
    float z;
} Vec3f;

#define vec3f_make(x, y, z)    ((Vec3f) { x, y, z })

typedef struct vec4f {
    float x;
    float y;
    float z;
    float w;
} Vec4f;

#define vec4f_make(x, y, z, w)    ((Vec3f) { x, y, z, w })


typedef struct matrix4f {
    float array[16];
} Matrix4f;


typedef Matrix4f Transform;

/**
 * This macro uses [ domain_start, domain_end ] to specify domain boundaries, and checks if value inside that domain.
 */
#define value_inside_domain(domain_start, domain_end, value) (domain_start <= value && value <= domain_end)


typedef struct axis_aligned_bounding_box {
    Vec2f p0;
    Vec2f p1;
} AABB;


typedef struct oriented_bounding_box {
    Vec2f center;
    Vec2f dimensions;
    float rotation;
} OBB;


typedef struct circle {
    Vec2f center;
    float radius;
} Circle;


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

typedef struct texture {
    u32 id;             // OpenGL texture id.
    s32 width;          // Pixel width of texture.
    s32 height;         // Pixel height of texture.
    Vec2f uv0;          // Bottom left uv coordinate of the texture.
    Vec2f uv1;          // Top right uv coordinate of the texture.
} Texture;

typedef struct shader {
    u32 id;             // OpenGL program id.
} Shader;

typedef struct quad_drawer {
    u32 vao;            // OpenGL id of Vertex Array Object.
    u32 vbo;            // OpenGL id of Vertex Buffer Object.
    u32 ebo;            // OpenGL id of Element Buffer Object.

    Shader *program;    // Pointer to shader that will be used to draw.
} Quad_Drawer;

typedef struct camera {
    Vec2f center;       // World center.
    u32 unit_scale;     // Pixels per 1 world unit.
} Camera;

Camera camera_make(Vec2f center, u32 unit_scale);

#endif
