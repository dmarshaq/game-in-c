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

#define vec2f_make(x, y)                    ((Vec2f) { x, y } )

#define VEC2F_ORIGIN                        ((Vec2f) { 0.0f,  0.0f } )
#define VEC2F_RIGHT                         ((Vec2f) { 1.0f,  0.0f } )
#define VEC2F_LEFT                          ((Vec2f) {-1.0f,  0.0f } )
#define VEC2F_UP                            ((Vec2f) { 0.0f,  1.0f } )
#define VEC2F_DOWN                          ((Vec2f) { 0.0f, -1.0f } )
#define VEC2F_UNIT                          ((Vec2f) { 1.0f,  1.0f } )

#define vec2f_print(v1)                     printf(#v1 " = ( % 2.2f , % 2.2f )\n", v1.x, v1.y)

#define vec2f_sum(v1, v2)                   vec2f_make(v1.x + v2.x, v1.y + v2.y)
#define vec2f_difference(v1, v2)            vec2f_make(v1.x - v2.x, v1.y - v2.y) 
#define vec2f_sum_constant(v1, c)           vec2f_make(v1.x + c, v1.y + c)
#define vec2f_difference_constant(v1, c)    vec2f_make(v1.x - c, v1.y - c) 
#define vec2f_dot(v1, v2)                   (v1.x * v2.x + v1.y * v2.y)
#define vec2f_multi_constant(v1, c)         vec2f_make(v1.x * c, v1.y * c)
#define vec2f_divide_constant(v1, c)        vec2f_make(v1.x / c, v1.y / c)
#define vec2f_magnitude(v1)                 right_triangle_hypotenuse(v1.x, v1.y)
#define vec2f_distance(v1, v2)              right_triangle_hypotenuse(v1.x - v2.x, v1.y - v2.y)
#define vec2f_negate(v1)                    vec2f_make(-v1.x, -v1.y)


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

#define vec4f_print(v1)                     printf(#v1 " = ( % 2.2f , % 2.2f , % 2.2f , % 2.2f )\n", v1.x, v1.y, v1.z, v1.w)

typedef struct matrix4f {
    float array[16];
} Matrix4f;

#define MATRIX4F_ZERO                                                   ((Matrix4f) {{ 0.0f, 0.0f, 0.0f, 0.0f,   0.0f, 0.0f, 0.0f, 0.0f,    0.0f, 0.0f, 0.0f, 0.0f,    0.0f, 0.0f, 0.0f, 0.0f }} )
#define MATRIX4F_IDENTITY                                               ((Matrix4f) {{ 1.0f, 0.0f, 0.0f, 0.0f,   0.0f, 1.0f, 0.0f, 0.0f,    0.0f, 0.0f, 1.0f, 0.0f,    0.0f, 0.0f, 0.0f, 1.0f }} )

#define matrix4f_print(matrix)                                          printf(#matrix " =\n[[ % 2.2f , % 2.2f , % 2.2f , % 2.2f ]\n [ % 2.2f , % 2.2f , % 2.2f , % 2.2f ]\n [ % 2.2f , % 2.2f , % 2.2f , % 2.2f ]\n [ % 2.2f , % 2.2f , % 2.2f , % 2.2f ]]\n", matrix.array[0], matrix.array[1], matrix.array[2], matrix.array[3], matrix.array[4], matrix.array[5], matrix.array[6], matrix.array[7], matrix.array[8], matrix.array[9], matrix.array[10], matrix.array[11], matrix.array[12], matrix.array[13], matrix.array[14], matrix.array[15])

#define matrix4f_orthographic(left, right, bottom, top, near, far)     ((Matrix4f) {{ 2.0f / ((right) - (left)), 0.0f, 0.0f, 0.0f,    0.0f, 2.0f / ((top) - (bottom)), 0.0f, 0.0f,    0.0f, 0.0f, 2.0f / ((near) - (far)), 0.0f,    ((left) + (right)) / ((left) - (right)), ((bottom) + (top)) / ((bottom) - (top)), ((near) + (far)) / ((near) - (far)), 1.0f }} )

#define matrix4f_vec2f_multiplication(matrix, vec2)                     ((Vec2f) { .x = multiplier->array[0] * target->x + multiplier->array[1] * target->y,    .y = multiplier->array[4] * target->x + multiplier->array[5] * target->y, } )
#define matrix4f_vec3f_multiplication(matrix, vec3)                     ((Vec3f) { .x = multiplier->array[0] * target->x + multiplier->array[1] * target->y + multiplier->array[2] * target->z,    .y = multiplier->array[4] * target->x + multiplier->array[5] * target->y + multiplier->array[6] * target->z,    .z = multiplier->array[8] * target->x + multiplier->array[9] * target->y + multiplier->array[10] * target->z, } )
#define matrix4f_vec4f_multiplication(matrix, vec4)                     ((Vec4f) { .x = multiplier->array[0] * target->x + multiplier->array[1] * target->y + multiplier->array[2] * target->z + multiplier->array[3] * target->w,    .y = multiplier->array[4] * target->x + multiplier->array[5] * target->y + multiplier->array[6] * target->z + multiplier->array[7] * target->w,    .z = multiplier->array[8] * target->x + multiplier->array[9] * target->y + multiplier->array[10] * target->z + multiplier->array[11] * target->w,    .w = multiplier->array[12] * target->x + multiplier->array[13] * target->y + multiplier->array[14] * target->z + multiplier->array[15] * target->w, } )

Matrix4f matrix4f_multiplication(Matrix4f *multiplier, Matrix4f *target);

typedef Matrix4f Transform;

#define transform_translation_2d(position)                              ((Transform) {{ 1.0f, 0.0f, 0.0f, position.x,   0.0f, 1.0f, 0.0f, position.y,    0.0f, 0.0f, 1.0f, 0.0f,    0.0f, 0.0f, 0.0f, 1.0f }} )
#define transform_rotation_2d(angle)                                    ((Transform) {{ cosf(angle), -sinf(angle), 0.0f, 0.0f,    sinf(angle),  cosf(angle), 0.0f, 0.0f,    0.0f, 0.0f, 1.0f, 0.0f,    0.0f, 0.0f, 0.0f, 1.0f }} )
#define transform_scale_2d(scale)                                       ((Transform) {{ scale.x, 0.0f, 0.0f, 0.0f,   0.0f, scale.y, 0.0f, 0.0f,    0.0f, 0.0f, 1.0f, 0.0f,    0.0f, 0.0f, 0.0f, 1.0f }} )

Transform transform_srt_2d(Vec2f position, float angle, Vec2f scale);
Transform transform_trs_2d(Vec2f position, float angle, Vec2f scale);


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
 * File utils.
 */

/**
 * Reads contents of the file into the buffer that is preemptivly allocated using malloc.
 * Return pointer to the buffer and sets buffer size in bytes into the file_size.
 * @Important: Buffer should be freed manually when not used anymore.
 */
void* read_file_into_buffer(char *file_name, u64* file_size);

/**
 * @Incomplete: Write description.
 */
char* read_file_into_string_buffer(char *file_name);
    
/**
 * @Incomplete: Write description.
 */
String_8 read_file_into_str8(char *file_name);

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

/**
 * Loads texture from image file and returns struct that contains it's OpenGL id with other texture parameters.
 */
Texture texture_load(char *texture_path);


typedef struct shader {
    u32 id;             // OpenGL program id.
    u32 vertex_stride;  // Stride length in bytes needed to be allocated per vertex for shader to run correctly for each vertex.
} Shader;

/**
 * Loads shader from .glsl file and returns struct that contains it's OpenGL id.
 */
Shader shader_load(char *shader_path);


typedef struct quad_drawer {
    u32 vao;            // OpenGL id of Vertex Array Object.
    u32 vbo;            // OpenGL id of Vertex Buffer Object.
    u32 ebo;            // OpenGL id of Element Buffer Object.

    Shader *program;    // Pointer to shader that will be used to draw.
} Quad_Drawer;

#define MAX_QUADS_PER_BATCH     1
#define VERTICIES_PER_QUAD      4
#define INDICIES_PER_QUAD       6

/**
 * @Incomplete: Write description.
 */
void drawer_init(Quad_Drawer *drawer, Shader *shader);

/**
 * @Incomplete: Write description.
 */
float add_texture_to_slots(Texture *texture);

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
void draw_quad_data(float *quad_data);


typedef struct camera {
    Vec2f center;       // World center.
    u32 unit_scale;     // Pixels per 1 world unit.
} Camera;

Camera camera_make(Vec2f center, u32 unit_scale);

/**
 * @Incomplete: Write description.
 */
void graphics_update_projection(Quad_Drawer *drawer, Camera *camera, float window_width, float window_height);


typedef struct font_baked {
    stbtt_bakedchar *chars;
    Texture texture;
} Font_Baked;

Font_Baked font_bake(u8 *font_data, float font_size);





void test_font();













#endif
