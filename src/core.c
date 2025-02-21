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
 * @Incomplete: Fix error messages format :3 .
 */

char* read_file_into_string_buffer(char *file_name) {
    FILE *file = fopen(file_name, "rb");
    if (file == NULL) {
        printf("Error: Couldn't open the file \"%s\".\n", file_name);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    rewind(file);

    char *buffer = (char*) malloc(file_size + 1);
    if (buffer == NULL) {
        printf("Error: Memory allocation for string buffer failed while reading the file \"%s\".\n", file_name);
        fclose(file);
        return NULL;
    }

    if (fread(buffer, 1, file_size, file) != file_size) {
        printf("Error: Failure reading the file \"%s\".\n", file_name);
        fclose(file);
        free(buffer);
        return NULL;
    }

    buffer[file_size] = '\0';
    fclose(file);

    return buffer;
}

void* read_file_into_buffer(char *file_name, u64* file_size) {
    FILE *file = fopen(file_name, "rb");
    if (file == NULL) {
        printf("Error: Couldn't open the file \"%s\".\n", file_name);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    u64 size = ftell(file);
    rewind(file);

    u8 *buffer = (u8*) malloc(size);
    if (buffer == NULL) {
        printf("Error: Memory allocation for string buffer failed while reading the file \"%s\".\n", file_name);
        fclose(file);
        return NULL;
    }

    if (fread(buffer, 1, size, file) != size) {
        printf("Error: Failure reading the file \"%s\".\n", file_name);
        fclose(file);
        free(buffer);
        return NULL;
    }

    fclose(file);

    if (file_size != NULL) {
        *file_size = size;
    }

    return buffer;
}

String_8 read_file_into_str8(char *file_name) {
    FILE *file = fopen(file_name, "rb");
    if (file == NULL) {
        printf("Error: Couldn't open the file \"%s\".\n", file_name);
    }

    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    rewind(file);

    String_8 str = {
        .ptr = malloc(file_size),
        .length = (u32)file_size,
    };
    str.ptr = malloc(file_size);
    str.length = (u32)file_size;

    if (str.ptr == NULL) {
        printf("Error: Memory allocation for string buffer failed while reading the file \"%s\".\n", file_name);
        fclose(file);
        str.length = 0;
    }

    if (fread(str.ptr, 1, file_size, file) != file_size) {
        printf("Error: Failure reading the file \"%s\".\n", file_name);
        fclose(file);
        free(str.ptr);
        str.ptr = NULL;
        str.length = 0;
    }

    fclose(file);

    return str;
}

/**
 * Input.
 */

u8 *KEYBOARD_STATE_OLD;
const u8 *KEYBOARD_STATE_CURRENT;
s32 KEYBOARD_NUMBER_OF_KEYS;

void keyboard_state_free() {
    free(KEYBOARD_STATE_OLD);
}

void keyboard_state_old_update() {
    memcpy(KEYBOARD_STATE_OLD, KEYBOARD_STATE_CURRENT, KEYBOARD_NUMBER_OF_KEYS);
}

void keyboard_state_init() {
    KEYBOARD_STATE_CURRENT = SDL_GetKeyboardState(&KEYBOARD_NUMBER_OF_KEYS);
    KEYBOARD_STATE_OLD = malloc(KEYBOARD_NUMBER_OF_KEYS);
    keyboard_state_old_update();
}

bool is_hold_keycode(SDL_KeyCode keycode) {
    return *(KEYBOARD_STATE_CURRENT + SDL_GetScancodeFromKey(keycode)) == 1;
}

bool is_pressed_keycode(SDL_KeyCode keycode) {
    return *(KEYBOARD_STATE_OLD + SDL_GetScancodeFromKey(keycode)) == 0 && *(KEYBOARD_STATE_CURRENT + SDL_GetScancodeFromKey(keycode)) == 1;
}

bool is_unpressed_keycode(SDL_KeyCode keycode) {
    return *(KEYBOARD_STATE_OLD + SDL_GetScancodeFromKey(keycode)) == 1 && *(KEYBOARD_STATE_CURRENT + SDL_GetScancodeFromKey(keycode)) == 0;
}


/**
 * Graphics.
 */

#include <GL/glew.h>

bool check_gl_error() {
    s32 error = glGetError();
    if (error != 0) {
        fprintf(stderr, "%s OpenGL error: %d.\n", debug_error_str, error);
        return false;
    }
    return true;
}

int init_sdl_gl() {
    // Initialize SDL.
    if (SDL_Init(SDL_INIT_VIDEO) < 0 || SDL_Init(SDL_INIT_AUDIO) < 0) {
        fprintf(stderr, "%s SDL could not initialize! SDL_Error: %s\n", debug_error_str, SDL_GetError());
        return 1;
    }
    

    // Set OpenGL attributes.
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    
    return 0;
}

SDL_Window* create_gl_window(const char *title, int x, int y, int width, int height) {
    // Create window.
    SDL_Window *window = SDL_CreateWindow(title, x, y, width, height, SDL_WINDOW_OPENGL);
    if (window == NULL) {
        fprintf(stderr, "%s Window could not be created! SDL_Error: %s\n", debug_error_str, SDL_GetError());
        return NULL;
    }

    // Create OpenGL context.
    SDL_GLContext glContext = SDL_GL_CreateContext(window);
    if (glContext == NULL) {
        fprintf(stderr, "%s OpenGL context could not be created! SDL_Error: %s\n", debug_error_str, SDL_GetError());
        return window;
    }

    if (SDL_GL_MakeCurrent(window, glContext) < 0) {
        fprintf(stderr, "%s OpenGL current could not be created! SDL_Error: %s\n", debug_error_str, SDL_GetError());
        return window;
    }

    // Initialize GLEW.
    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "%s GLEW initialization failed!\n", debug_error_str);
        return window;
    }

    return window;
}

int init_sdl_audio() {
    // Initialize sound.
    if (Mix_Init(MIX_INIT_MP3) < 0) {
        fprintf(stderr, "%s SDL Mixer could not initialize! SDL_Error: %s\n", debug_error_str, SDL_GetError());
        return 1;
    }

    Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 1024);
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
    str8_init_statically(&vertex_shader_defines, "#define VERTEX\n");
    str8_init_statically(&fragment_shader_defines, "#define FRAGMENT\n");

    // Init string for version tage in shader, for shader creation to look for when loading shader.
    str8_init_statically(&shader_version_tag, "#version");

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
    verticies = array_list_make(float, 40); // @Leak.
    quad_indicies = array_list_make(u32, MAX_QUADS_PER_BATCH * 6); // @Leak.
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
        fprintf(stderr, "%s Stbi couldn't load image.\n", debug_error_str);
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


const char *SHADER_UNIFORM_PR_MATRIX_NAME = "pr_matrix";
const char *SHADER_UNIFORM_ML_MATRIX_NAME = "ml_matrix";
const char *SHADER_UNIFORM_SAMPLERS_NAME = "u_textures";

/**
 * @Temporary: Later, setting uniforms either will be done more automatically, or simplified to be done by uset manually. 
 * But right now it is not neccassary to care about too much, since only one shader is used anyway.
 */
void shader_set_uniforms(Shader *program) {
    // Get uniform's locations based on unifrom's name.
    s32 quad_shader_pr_matrix_loc = glGetUniformLocation(program->id, SHADER_UNIFORM_PR_MATRIX_NAME);
    if (quad_shader_pr_matrix_loc == -1) {
        fprintf(stderr, "%s Couldn't get location of %s uniform, in shader.\n", debug_error_str, SHADER_UNIFORM_PR_MATRIX_NAME);
    }

    s32 quad_shader_ml_matrix_loc = glGetUniformLocation(program->id, SHADER_UNIFORM_ML_MATRIX_NAME);
    if (quad_shader_ml_matrix_loc == -1) {
        fprintf(stderr, "%s Couldn't get location of %s uniform, in shader.\n", debug_error_str, SHADER_UNIFORM_ML_MATRIX_NAME);
    }

    s32 quad_shader_samplers_loc = glGetUniformLocation(program->id, SHADER_UNIFORM_SAMPLERS_NAME);
    if (quad_shader_samplers_loc == -1) {
        fprintf(stderr, "%s Couldn't get location of %s uniform, in shader.\n", debug_error_str, SHADER_UNIFORM_SAMPLERS_NAME);
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
        fprintf(stderr, "%s Program of %s, failed to link.\n", debug_error_str, shader_path);
       
        s32 info_log_length;
        glGetProgramiv(id, GL_INFO_LOG_LENGTH, &info_log_length);
        char *buffer = malloc(info_log_length);
        
        s32 buffer_size;
        glGetProgramInfoLog(id, info_log_length, &buffer_size, buffer);
        fprintf(stderr, "%s\n", buffer);
        
        free(buffer);

        return false;
    } 
    return true;
}

bool check_shader(u32 id, char *shader_path) {
    s32 is_compiled = 0;
    glGetShaderiv(id, GL_COMPILE_STATUS, &is_compiled); 
    if (is_compiled == GL_FALSE) {
        fprintf(stderr, "%s Shader of %s, failed to compile.\n", debug_error_str, shader_path);
        
        s32 info_log_length;
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &info_log_length);
        char *buffer = malloc(info_log_length);
        
        s32 buffer_size;
        glGetShaderInfoLog(id, info_log_length, &buffer_size, buffer);
        fprintf(stderr, "%s\n", buffer);
        
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

    shader_source = read_file_into_str8(shader_path);
    
    // Splitting on two substrings, "shader_version" and "shader_code".
    u32 start_of_version_tag = str8_index_of_str8(&shader_source, &shader_version_tag, 0, shader_source.length);
    u32 end_of_version_tag = str8_index_of_char(&shader_source, '\n', start_of_version_tag, shader_source.length);
    str8_substring(&shader_source, &shader_version, start_of_version_tag, end_of_version_tag + 1);
    str8_substring(&shader_source, &shader_code, end_of_version_tag + 1, shader_source.length);
    
    // Passing all parts in the respective shader sources, where defines inserted between version and code parts.
    const char *vertex_shader_source[3] = { (char *)shader_version.ptr, (char *)vertex_shader_defines.ptr, (char *)shader_code.ptr };
    const char *fragment_shader_source[3] = { (char *)shader_version.ptr, (char *)fragment_shader_defines.ptr, (char *)shader_code.ptr };

    // Specifying lengths, because we don't pass null terminated strings.
    // @Important: Since defines length depends on whether we are loading vertex or fragment shader, we set it's length values separately when loading each.
    shader_strings_lengths[0] = shader_version.length;
    shader_strings_lengths[2] = shader_code.length;
    

    shader_strings_lengths[1] = vertex_shader_defines.length;
    u32 vertex_shader;
    vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 3, vertex_shader_source, shader_strings_lengths);
    glCompileShader(vertex_shader);
    
    // Check results for errors.
    check_shader(vertex_shader, shader_path);

    shader_strings_lengths[1] = fragment_shader_defines.length;
    u32 fragment_shader;
    fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 3, fragment_shader_source, shader_strings_lengths);
    glCompileShader(fragment_shader);
    
    // Check results for errors.
    check_shader(fragment_shader, shader_path);
    
    Shader shader = {
        .id = glCreateProgram(),
    };

    glAttachShader(shader.id, vertex_shader);
    glAttachShader(shader.id, fragment_shader);
    glLinkProgram(shader.id);
    
    // Check results for errors.
    check_program(shader.id, shader_path);

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    

    str8_free(&shader_source);

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

void graphics_update_projection(Shader *shader, Camera *camera, float window_width, float window_height) {
    float camera_width_offset = (window_width / (float) camera->unit_scale) / 2;
    float camera_height_offset = (window_height / (float) camera->unit_scale) / 2;
    shader_uniform_pr_matrix = matrix4f_orthographic(camera->center.x - camera_width_offset, camera->center.x + camera_width_offset, camera->center.y - camera_height_offset, camera->center.y + camera_height_offset, -1.0f, 1.0f);

    s32 quad_shader_pr_matrix_loc = glGetUniformLocation(shader->id, SHADER_UNIFORM_PR_MATRIX_NAME);
    if (quad_shader_pr_matrix_loc == -1) {
        fprintf(stderr, "%s Couldn't get location of %s uniform, in quad_shader.\n", debug_error_str, SHADER_UNIFORM_PR_MATRIX_NAME);
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
    fprintf(stderr, "%s Overflow of 32 texture slots limit, can't add texture id: %d, to current draw call texture slots.\n", debug_error_str, texture->id);
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
    stbtt_InitFont(&info, font_data, 0);

    // Initing needed variables. @Temporary: Maybe move some of this variables into font struct directly and calculate when baking bitmap.
    s32 ascent, descent, line_gap;
    stbtt_GetFontVMetrics(&info, &ascent, &descent, &line_gap);

    float scale = stbtt_ScaleForPixelHeight(&info, font_size);
    result.line_height = (s32)((ascent - descent + line_gap) * scale);
    result.baseline = (s32)(ascent * -scale);

    // Create a bitmap.
    result.bitmap.width    = 512;
    result.bitmap.height   = 512;
    u8 *bitmap = calloc(result.bitmap.width * result.bitmap.height, sizeof(u8));

    // Bake the font into the bitmap.
    result.first_char_code  = 32; // ASCII value of the first character to bake.
    result.chars_count      = 96;  // Number of characters to bake.
                         
    result.chars = malloc(result.chars_count * sizeof(stbtt_bakedchar));
    stbtt_BakeFontBitmap(font_data, 0, font_size, bitmap, result.bitmap.width, result.bitmap.height, result.first_char_code, result.chars_count, result.chars);

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



void test_font() {
    // Step 1: Load the TTF file into memory
    FILE* fontFile = fopen("font.ttf", "rb");
    if (!fontFile) {
        perror("Failed to open font file");
    }

    fseek(fontFile, 0, SEEK_END);
    long fontSize = ftell(fontFile);
    fseek(fontFile, 0, SEEK_SET);

    unsigned char* fontBuffer = (unsigned char*)malloc(fontSize);
    if (!fontBuffer) {
        perror("Failed to allocate memory for font");
        fclose(fontFile);
    }

    fread(fontBuffer, 1, fontSize, fontFile);
    fclose(fontFile);

    // Step 2: Initialize the font
    stbtt_fontinfo font;
    if (!stbtt_InitFont(&font, fontBuffer, 0)) {
        fprintf(stderr, "Failed to initialize font\n");
        free(fontBuffer);
    }

    // Step 3: Get some metrics
    int ascent, descent, lineGap;
    stbtt_GetFontVMetrics(&font, &ascent, &descent, &lineGap);

    float scale = stbtt_ScaleForPixelHeight(&font, 24.0f); // Scale for 24 pixels tall
    
    char* message = "Hello World!";
    unsigned char* bitmap;

    for (s32 i = 0; i < strlen(message); i++) {
        // Step 4: Rasterize a glyph
        int codepoint = message[i];
        int width, height, xoff, yoff;
        bitmap = stbtt_GetCodepointBitmap(&font, 0, scale, codepoint, &width, &height, &xoff, &yoff);

        // Step 5: Render the glyph (for simplicity, just print to console)
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                printf("%c", bitmap[y * width + x] > 128 ? '#' : ' ');
            }
            printf("\n");
        }
    }

 

    // Clean up
    stbtt_FreeBitmap(bitmap, NULL);
    free(fontBuffer);
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
    glBufferData(GL_ARRAY_BUFFER, drawer->program->vertex_stride * VERTICIES_PER_QUAD * MAX_QUADS_PER_BATCH * sizeof(float), NULL, GL_DYNAMIC_DRAW);

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

    printf("\n---------- VERTICIES -----------\n");
    u32 length = array_list_length(&verticies);
    if (length == 0) {
        printf("[ ]\n");
    }
    else {
        printf("[ ");
        for (u32 i = 0; i < length - 1; i++) {
            printf("%6.1f, ", verticies[i]);
            if ((i + 1) % stride == 0)
                printf("\n  ");
        }
        printf("%6.1f  ]\n", verticies[length - 1]);
    }

    printf("Length   : %8d\n", length);
    printf("Capacity : %8d\n\n", array_list_capacity(&verticies));
    
}

void print_indicies() {
    printf("\n----------- INDICIES -----------\n");
    printf("[\n\n  ");
    u32 length = array_list_length(&quad_indicies);
    for (u32 i = 0; i < length; i++) {
        if ((i + 1) % 6 == 0)
            printf("%4d\n\n  ", quad_indicies[i]);
        else if ((i + 1) % 3 == 0)
            printf("%4d\n  ", quad_indicies[i]);
        else
            printf("%4d, ", quad_indicies[i]);
    }
    printf("\r]\n");
    printf("Length   : %8d\n", length);
    printf("Capacity : %8d\n\n", array_list_capacity(&quad_indicies));
}
