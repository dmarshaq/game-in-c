#include "game/editor.h"

#include "game/draw.h"
#include "game/game.h"
#include "game/graphics.h"

#include "core/mathf.h"
#include "core/structs.h"
#include "core/file.h"

typedef struct editor_quad {
    Vec2f verticies[4];
} Editor_Quad;



static Editor_Quad *quads_list;



static Quad_Drawer *drawer_ptr;

static Font_Baked font_small;
static Font_Baked font_medium;

static Camera editor_camera;



void editor_init(State *state) {
    quads_list = array_list_make(Editor_Quad, 8, &std_allocator);

    // @Copypasta: From console.c ... 
    // Get resources.
    drawer_ptr = &state->quad_drawer;

    // Load needed font... Hard coded...
    u8* font_data = read_file_into_buffer("res/font/Consolas-Regular.ttf", NULL, &std_allocator);

    font_small  = font_bake(font_data, 14.0f);
    font_medium = font_bake(font_data, 20.0f);

    free(font_data);
    

    // Copying main camera for editor.
    editor_camera = state->main_camera;
}

bool editor_update(Window_Info *window, Events_Info *events, Time_Info *t) {
    
    return false;
}

void editor_draw(Window_Info *window) {
    Matrix4f projection;

    projection = camera_calculate_projection(&editor_camera, window->width, window->height);
    shader_update_projection(drawer_ptr->program, &projection);

    draw_begin(drawer_ptr);

    // Draw editor quads.
    for (u32 i = 0; i < array_list_length(&quads_list); i++) {
        
        Vec4f color = VEC4F_CYAN;

        // Hard coded, bad.
        float quad_data[44] = {
            quads_list[i].verticies[0].x, quads_list[i].verticies[0].y, 0.0f, color.x, color.y, color.z, color.w, 0.0f, 0.0f, -1.0f, -1.0f,
            quads_list[i].verticies[1].x, quads_list[i].verticies[1].y, 0.0f, color.x, color.y, color.z, color.w, 1.0f, 0.0f, -1.0f, -1.0f,
            quads_list[i].verticies[2].x, quads_list[i].verticies[2].y, 0.0f, color.x, color.y, color.z, color.w, 0.0f, 1.0f, -1.0f, -1.0f,
            quads_list[i].verticies[3].x, quads_list[i].verticies[3].y, 0.0f, color.x, color.y, color.z, color.w, 1.0f, 1.0f, -1.0f, -1.0f,
        };

        draw_quad_data(quad_data, 1);
    }

    draw_end();



    projection = screen_calculate_projection(window->width, window->height);
    shader_update_projection(drawer_ptr->program, &projection);

    draw_begin(drawer_ptr);

    // Draw editor ui.

    draw_end();
}

void editor_get_verticies(Vec2f **verticies, s64 *verticies_count) {
    TODO("Getting editor result.");
}

void editor_add_quad() {
    array_list_append(&quads_list, ((Editor_Quad) {
                .verticies = { { -1.0f, -1.0f }, { 1.0f, -1.0f, }, { -1.0f, 1.0f }, { 1.0f, 1.0f } },
                }));
}
