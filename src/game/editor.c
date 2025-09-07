#include "game/editor.h"

#include "game/draw.h"
#include "game/game.h"
#include "game/graphics.h"
#include "game/console.h"

#include "core/mathf.h"
#include "core/structs.h"
#include "core/file.h"






typedef struct editor_quad {
    Vec2f verticies[4];
    Vec4f color;
} Editor_Quad;



void draw_editor_quad(Editor_Quad *quad) {

    float quad_data[44] = {
        quad->verticies[0].x, quad->verticies[0].y, 0.0f, quad->color.x, quad->color.y, quad->color.z, quad->color.w * 0.8f, 0.0f, 0.0f, -1.0f, -1.0f,
        quad->verticies[1].x, quad->verticies[1].y, 0.0f, quad->color.x, quad->color.y, quad->color.z, quad->color.w * 0.8f, 1.0f, 0.0f, -1.0f, -1.0f,
        quad->verticies[2].x, quad->verticies[2].y, 0.0f, quad->color.x, quad->color.y, quad->color.z, quad->color.w * 0.8f, 0.0f, 1.0f, -1.0f, -1.0f,
        quad->verticies[3].x, quad->verticies[3].y, 0.0f, quad->color.x, quad->color.y, quad->color.z, quad->color.w * 0.8f, 1.0f, 1.0f, -1.0f, -1.0f,
    };

    draw_quad_data(quad_data, 1);
}


void draw_editor_quad_outline(Editor_Quad *quad) {

    float line_data[56] = {
        quad->verticies[0].x, quad->verticies[0].y, 0.0f, 0.95f, 0.95f, 0.95f, 1.0f,
        quad->verticies[1].x, quad->verticies[1].y, 0.0f, 0.95f, 0.95f, 0.95f, 1.0f,
        quad->verticies[1].x, quad->verticies[1].y, 0.0f, 0.95f, 0.95f, 0.95f, 1.0f,
        quad->verticies[3].x, quad->verticies[3].y, 0.0f, 0.95f, 0.95f, 0.95f, 1.0f,
        quad->verticies[3].x, quad->verticies[3].y, 0.0f, 0.95f, 0.95f, 0.95f, 1.0f,
        quad->verticies[2].x, quad->verticies[2].y, 0.0f, 0.95f, 0.95f, 0.95f, 1.0f,
        quad->verticies[2].x, quad->verticies[2].y, 0.0f, 0.95f, 0.95f, 0.95f, 1.0f,
        quad->verticies[0].x, quad->verticies[0].y, 0.0f, 0.95f, 0.95f, 0.95f, 1.0f,
    };

    draw_line_data(line_data, 4);
}






static Editor_Quad *quads_list;



static Quad_Drawer *quad_drawer_ptr;
static Line_Drawer *line_drawer_ptr;

static Font_Baked font_small;
static Font_Baked font_medium;

static Camera editor_camera;



void editor_init(State *state) {
    quads_list = array_list_make(Editor_Quad, 8, &std_allocator);

    // @Copypasta: From console.c ... 
    // Get resources.
    quad_drawer_ptr = &state->quad_drawer;
    line_drawer_ptr = &state->line_drawer;

    // Load needed font... Hard coded...
    u8* font_data = read_file_into_buffer("res/font/Consolas-Regular.ttf", NULL, &std_allocator);

    font_small  = font_bake(font_data, 14.0f);
    font_medium = font_bake(font_data, 20.0f);

    free(font_data);
    

    // Copying main camera for editor.
    editor_camera = state->main_camera;
}

bool editor_update(Window_Info *window, Events_Info *events, Time_Info *t) {
    

    if (events->mouse_input.left_pressed) {
        console_log("Editor: Select.\n");
    }

    return false;
}

void editor_draw(Window_Info *window) {
    Matrix4f projection;

    projection = camera_calculate_projection(&editor_camera, window->width, window->height);
    
    

    // Drawing quads.
    shader_update_projection(quad_drawer_ptr->program, &projection);

    draw_begin(quad_drawer_ptr);

    // Draw editor quads.
    for (u32 i = 0; i < array_list_length(&quads_list); i++) {
        draw_editor_quad(quads_list + i);
    }

    draw_end();


    // Drawing quad outlines.
    shader_update_projection(line_drawer_ptr->program, &projection);

    line_draw_begin(line_drawer_ptr);

    // Draw editor quads.
    for (u32 i = 0; i < array_list_length(&quads_list); i++) {
        draw_editor_quad_outline(quads_list + i);
    }

    line_draw_end();





    projection = screen_calculate_projection(window->width, window->height);
    shader_update_projection(quad_drawer_ptr->program, &projection);

    draw_begin(quad_drawer_ptr);

    // Draw editor ui.

    draw_end();
}

void editor_get_verticies(Vec2f **verticies, s64 *verticies_count) {
    TODO("Getting editor result.");
}

void editor_add_quad() {
    array_list_append(&quads_list, ((Editor_Quad) {
                .verticies = { { -1.0f, -1.0f }, { 1.0f, -1.0f, }, { -1.0f, 1.0f }, { 1.0f, 1.0f } },
                .color = { randf() * 0.6f + 0.2f, randf() * 0.6f + 0.2f, randf() * 0.6f + 0.2f, 1.0f },
                }));
}
