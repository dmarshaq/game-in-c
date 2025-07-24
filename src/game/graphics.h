#ifndef GRAPHICS_H
#define GRAPHICS_H

/**
 * Graphics.
 */

#include "core/core.h"
#include "core/type.h"
#include "core/mathf.h"

#include <stdbool.h>
#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_video.h>
#include <SDL2/SDL_audio.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_timer.h>

#include "stb_truetype.h"


/**
 * Simple glGetError() wrapper that outputs OpenGL error code if it detects error at a time of calling.
 * Returns false - failure if error was detected.
 * Return true - success if there are no OpenGL errors.
 */
bool check_gl_error();

/**
 * Wrapper around SDL Initialization.
 */
int init_sdl_gl();


typedef struct window_info {
    SDL_Window *ptr;
    s32 width;
    s32 height;
} Window_Info;

/**
 * Wrapper around GL window creation through SDL, with OpenGL context and GLEW initialization.
 */
Window_Info create_gl_window(const char *title, s32 x, s32 y, s32 width, s32 height);

/**
 * Wrapper around SDL Mix, Audio initialization.
 */
int init_sdl_audio();


/**
 * An initialization function for general purpose rendering, makes arraylists for indicies and verticies, configures GL Blending, and gets stbi to flip image vertically on load.
 * Needs to be called before other graphics functions are called.
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

/**
 * Simply adds texture to a 32 limited array of textures being used, if its already in array returns its index, if its not, appends added texture id to the end and return it's index, if it overflows 32 textures limits, its prints error and returns -1.0f.
 */
float add_texture_to_slots(Texture *texture);



#define MAX_ATTRIBUTES_PER_SHADER 8
#define MAX_ATTRIBUTE_NAME_LENGTH 128
#define ATTRIBUTE_COMPONENT_SIZE 4
#define ATTRIBUTE_COMPONENT_TYPE GL_FLOAT

typedef struct attribute {
    char name[MAX_ATTRIBUTE_NAME_LENGTH];
    GLenum type;
    s32 length;
    s32 components;
} Attribute;

typedef struct shader {
    u32 id;             // OpenGL program id.
    u32 vertex_stride;  // Stride length in bytes needed to be allocated per vertex for shader to run correctly for each vertex.
    s32 attributes_count;
    Attribute attributes[MAX_ATTRIBUTES_PER_SHADER];
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
 * Creates GL buffers based on shader vertex stride for quad drawer.
 */
void drawer_init(Quad_Drawer *drawer, Shader *shader);

/**
 * Properly frees GL buffers from previously initted quad drawer.
 */
void drawer_free(Quad_Drawer *drawer);




typedef struct line_drawer {
    u32     vao;         // OpenGL id of Vertex Array Object.
    u32     vbo;         // OpenGL id of Vertex Buffer Object.
    Shader  *program;    // Pointer to shader that will be used to draw.
} Line_Drawer;

#define MAX_LINES_PER_BATCH     1024
#define VERTICIES_PER_LINE      2

/**
 * Creates GL buffers based on shader vertex stride for line drawer.
 */
void line_drawer_init(Line_Drawer *drawer, Shader *shader);

/**
 * Properly frees GL buffers from previously initted line drawer.
 */
void line_drawer_free(Line_Drawer *drawer);






typedef float* Vertex_Buffer;

/**
 * Allocates memory on the heap for the vertex buffer, that can be used with the rest of the interface to draw to the screen.
 */
Vertex_Buffer vertex_buffer_make();

/**
 * Frees memory occupied by the Vertex_Buffer.
 */
void vertex_buffer_free(Vertex_Buffer *buffer);

/**
 * Simply adds specified vertex data to the vertex buffer.
 */
void vertex_buffer_append_data(Vertex_Buffer *buffer, float *vertex_data, u32 length);

/**
 * Draws quad data to the screen that is stored in the buffer.
 * For example: "draw_end()" uses this function to draw verticies that are stored inside default vertex buffer.
 */
void vertex_buffer_draw_quads(Vertex_Buffer *buffer, Quad_Drawer *drawer);

/**
 * Draws line data to the screen that is stored in the buffer.
 * For example: "line_draw_end()" uses this function to draw verticies that are stored inside default vertex buffer.
 */
void vertex_buffer_draw_lines(Vertex_Buffer *buffer, Line_Drawer *drawer);

/**
 * Clears vertex buffer, by setting it's length to zero.
 */
void vertex_buffer_clear(Vertex_Buffer *buffer);







/**
 * Doesn't drawcall anything, just sets drawer to be active drawer, for the graphics to know what drawer will be used in drawing of all passed data.
 */
void draw_begin(Quad_Drawer *drawer);

/**
 * Wraps around "draw_buffer()" using active drawer as a parameter, and internally inited vertices arraylist as a buffer parameter.
 * @Important: Essentially all general drawing should happen between draw_begin() and draw_end() calls.
 */
void draw_end();

/**
 * Simply places specified data directly into the default vertex buffer.
 */
void draw_quad_data(float *quad_data, u32 count);










/**
 * Doesn't drawcall anything, just sets drawer to be active line drawer, for the graphics to know what drawer will be used in drawing of all passed data.
 */
void line_draw_begin(Line_Drawer *drawer);

/**
 * Wraps around "line_draw_buffer()" using active line drawer as a parameter, and internally inited vertices arraylist as a buffer parameter.
 * @Important: Essentially all general line drawing should happen between line_draw_begin() and line_draw_end() calls. But it cannot happen inside "draw_begin()" and "draw_end()" since both lines and quads use same default vertex buffer.
 */
void line_draw_end();

/**
 * Simply places specified line data directly into the default vertex buffer.
 */
void draw_line_data(float *line_data, u32 count);






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
    s32             line_gap;
    Texture         bitmap;
} Font_Baked;

Font_Baked font_bake(u8 *font_data, float font_size);

void font_free(Font_Baked *font);


#endif
