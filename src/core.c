#include "core.h"
#include "SDL2/SDL_video.h"

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

Camera camera_make(Vec2f center, u32 unit_scale) {
    return (Camera) {
        .center = center,
        .unit_scale = unit_scale,
    };
}

