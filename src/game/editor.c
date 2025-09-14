#include "game/editor.h"

#include "meta_generated.h"

#include "game/draw.h"
#include "game/game.h"
#include "game/graphics.h"
#include "game/console.h"

#include "core/mathf.h"
#include "core/structs.h"
#include "core/arena.h"
#include "core/str.h"
#include "core/file.h"





typedef enum editor_quad_flags : u8 {
    EDITOR_QUAD_FLAG_P0_SELECTED = 0x01, // 00000001
    EDITOR_QUAD_FLAG_P2_SELECTED = 0x02, // 00000010
    EDITOR_QUAD_FLAG_P3_SELECTED = 0x04, // 00000100
    EDITOR_QUAD_FLAG_P1_SELECTED = 0x08, // 00001000
} Editor_Quad_Flags;

static const u8 EDITOR_QUAD_BITMASK_ANY_POINT_SELECTED = EDITOR_QUAD_FLAG_P0_SELECTED | EDITOR_QUAD_FLAG_P1_SELECTED | EDITOR_QUAD_FLAG_P2_SELECTED | EDITOR_QUAD_FLAG_P3_SELECTED;



typedef struct editor_quad {
    Editor_Quad_Flags flags;
    Quad quad;
    Vec4f color;
} Editor_Quad;



void draw_editor_quad(Editor_Quad *quad) {

    // float quad_data[44] = {
    //     quad->quad.verts[0].x, quad->quad.verts[0].y, 0.0f, quad->color.x, quad->color.y, quad->color.z, quad->color.w * 0.8f, 0.0f, 0.0f, -1.0f, -1.0f,
    //     quad->quad.verts[1].x, quad->quad.verts[1].y, 0.0f, quad->color.x, quad->color.y, quad->color.z, quad->color.w * 0.8f, 1.0f, 0.0f, -1.0f, -1.0f,
    //     quad->quad.verts[2].x, quad->quad.verts[2].y, 0.0f, quad->color.x, quad->color.y, quad->color.z, quad->color.w * 0.8f, 0.0f, 1.0f, -1.0f, -1.0f,
    //     quad->quad.verts[3].x, quad->quad.verts[3].y, 0.0f, quad->color.x, quad->color.y, quad->color.z, quad->color.w * 0.8f, 1.0f, 1.0f, -1.0f, -1.0f,
    // };

    // draw_quad_data(quad_data, 1);
}


void draw_editor_quad_outline(Editor_Quad *quad) {

    // float line_data[56] = {
    //     quad->quad.verts[0].x, quad->quad.verts[0].y, 0.0f, 0.95f, 0.95f, 0.95f, 1.0f,
    //     quad->quad.verts[1].x, quad->quad.verts[1].y, 0.0f, 0.95f, 0.95f, 0.95f, 1.0f,
    //     quad->quad.verts[1].x, quad->quad.verts[1].y, 0.0f, 0.95f, 0.95f, 0.95f, 1.0f,
    //     quad->quad.verts[3].x, quad->quad.verts[3].y, 0.0f, 0.95f, 0.95f, 0.95f, 1.0f,
    //     quad->quad.verts[3].x, quad->quad.verts[3].y, 0.0f, 0.95f, 0.95f, 0.95f, 1.0f,
    //     quad->quad.verts[2].x, quad->quad.verts[2].y, 0.0f, 0.95f, 0.95f, 0.95f, 1.0f,
    //     quad->quad.verts[2].x, quad->quad.verts[2].y, 0.0f, 0.95f, 0.95f, 0.95f, 1.0f,
    //     quad->quad.verts[0].x, quad->quad.verts[0].y, 0.0f, 0.95f, 0.95f, 0.95f, 1.0f,
    // };

    // draw_line_data(line_data, 4);
}


@Introspect;
typedef struct editor_params {
    float selection_radius;
    float camera_speed;
    float camera_move_lerp_t;
    float camera_zoom_min;
    float camera_zoom_max;
    float camera_zoom_speed;
    float camera_zoom_lerp_t;
} Editor_Params;

static Editor_Params editor_params;




static Editor_Quad *quads_list;




static Vec2f world_mouse_position;
static Vec2f world_mouse_position_change;

typedef enum editor_selected_type : u8 {
    EDITOR_NONE,
    EDITOR_QUAD,
} Editor_Selected_Type;



typedef struct editor_selected {
    Editor_Selected_Type type;

    union {
        Editor_Quad *quad;
    };
} Editor_Selected;


static Editor_Selected *editor_selected;

static Vec2f editor_camera_current_vel;
static float editor_camera_current_zoom;
static float editor_camera_current_zoom_vel;



static Font_Baked font_small;
static Font_Baked font_medium;

static Camera editor_camera;




#define ARENA_SIZE 1024

static Arena arena;

static String info_buffer;









// Pointers to global state.
static Quad_Drawer *quad_drawer_ptr;
static Quad_Drawer *ui_quad_drawer_ptr;
static Line_Drawer *line_drawer_ptr;
static Window_Info *window_ptr;
static Mouse_Input *mouse_input_ptr;
static Time_Info   *time_ptr;


void editor_init(State *state) {
    // Tweak vars default values.
    editor_params.selection_radius      = 0.1f;
    editor_params.camera_speed          = 1.0f;
    editor_params.camera_move_lerp_t    = 0.8f;
    editor_params.camera_zoom_min       = 1.0f;
    editor_params.camera_zoom_max       = 1.0f;
    editor_params.camera_zoom_speed     = 1.0f;
    editor_params.camera_zoom_lerp_t    = 0.8f;
    
    vars_tree_add(TYPE_OF(editor_params), (u8 *)&editor_params, CSTR("editor_params"));


    // Setting non tweak vars.
    editor_camera_current_vel = VEC2F_ORIGIN;
    editor_camera_current_zoom = 0.0f;
    editor_camera_current_zoom_vel = 0.0f;
    world_mouse_position = VEC2F_ORIGIN;
    world_mouse_position_change = VEC2F_ORIGIN;

    // Setting pointers to global state.
    quad_drawer_ptr    = &state->quad_drawer;
    ui_quad_drawer_ptr = &state->ui_quad_drawer;
    line_drawer_ptr    = &state->line_drawer;
    window_ptr         = &state->window;
    mouse_input_ptr    = &state->events.mouse_input;
    time_ptr           = &state->t;



    quads_list = array_list_make(Editor_Quad, 8, &std_allocator);
    editor_selected = array_list_make(Editor_Selected, 8, &std_allocator);

    
    // @Copypasta: From console.c ... 
    // Get resources.

    // Load needed font... Hard coded...
    u8* font_data = read_file_into_buffer("res/font/Consolas-Regular.ttf", NULL, &std_allocator);

    font_small  = font_bake(font_data, 14.0f);
    font_medium = font_bake(font_data, 20.0f);

    free(font_data);
    

    // Copying main camera for editor.
    editor_camera = state->main_camera;


    // Make arena and allocate space for the buffers.
    arena = arena_make(ARENA_SIZE);

    info_buffer.data   = arena_alloc(&arena, 256);
    info_buffer.length = 256;
}







void editor_update_camera() {

    // Position control.
    Vec2f vel = VEC2F_ORIGIN;

    if (hold(SDLK_d)) {
        vel.x += 1.0f;
    } 
    if (hold(SDLK_a)) {
        vel.x -= 1.0f;
    }
    if (hold(SDLK_w)) {
        vel.y += 1.0f;
    }
    if (hold(SDLK_s)) {
        vel.y -= 1.0f;
    }
    
    if (!(fequal(vel.x, 0.0f) && fequal(vel.y, 0.0f))) {
        vel = vec2f_normalize(vel);
        vel = vec2f_multi_constant(vel, editor_params.camera_speed);
    }

    editor_camera_current_vel = vec2f_lerp(editor_camera_current_vel, vel, editor_params.camera_move_lerp_t);

    editor_camera.center = vec2f_sum(editor_camera.center, vec2f_multi_constant(editor_camera_current_vel, time_ptr->delta_time));



    // Zoom control.
    editor_camera_current_zoom_vel = lerp(editor_camera_current_zoom_vel, mouse_input_ptr->scrolled_y * editor_params.camera_zoom_speed, editor_params.camera_zoom_lerp_t);

    editor_camera_current_zoom += editor_camera_current_zoom_vel * time_ptr->delta_time;

    editor_camera_current_zoom = clamp(editor_camera_current_zoom, 0.0f, 1.0f);

    editor_camera.unit_scale = lerp(editor_params.camera_zoom_min, editor_params.camera_zoom_max, editor_camera_current_zoom);




}





bool editor_update() {
    
    editor_update_camera();





    world_mouse_position_change = world_mouse_position;
    world_mouse_position = screen_to_camera(mouse_input_ptr->position, &editor_camera, window_ptr->width, window_ptr->height);
    world_mouse_position_change = vec2f_difference(world_mouse_position, world_mouse_position_change);

    Vec2f mouse_left_click_origin = VEC2F_ORIGIN;
    
    if (mouse_input_ptr->left_pressed) {
        mouse_left_click_origin = world_mouse_position;

        // Basically if left shift is not holding user doesn't want to save what was previously selected.
        if (!hold(SDLK_LSHIFT)) {
            // Clear selected flags.
            for (u32 i = 0; i < array_list_length(&editor_selected); i++) {
                switch (editor_selected[i].type) {
                    case EDITOR_QUAD:
                        editor_selected[i].quad->flags &= ~(EDITOR_QUAD_BITMASK_ANY_POINT_SELECTED);
                        break;
                }
            }
            array_list_clear(&editor_selected);
        }


        for (u32 i = 0; i < array_list_length(&quads_list); i++) {
            // Selecting closes vertex if any.
            for (u32 j = 0; j < VERTICIES_PER_QUAD; j++) {
                if (vec2f_distance(quads_list[i].quad.verts[j], world_mouse_position) < editor_params.selection_radius) {
                    // If a quad with vertex that user selected is not selected, it gets selected. Yeah goodluck reading this comment in the future.
                    if (!(quads_list[i].flags & EDITOR_QUAD_BITMASK_ANY_POINT_SELECTED)) {
                        // quads_list[i].flags |= EDITOR_QUAD_BITMASK_ANY_POINT_SELECTED;
                        array_list_append(&editor_selected, ((Editor_Selected) {
                                    .type = EDITOR_QUAD,
                                    .quad = &quads_list[i],
                                    }));
                    }

                
                    // Selected vertex flags are in the order p0 -> p2 -> p3 -> p1
                    // Same order as verticies in quad.
                    // So this will select needed vertex.
                    quads_list[i].flags |= 1 << j;
                    

                    goto editor_left_mouse_select_end;
                }
            }
        }

        AABB approximation_aabb;
        // @Temporary: In the future loop over the verticies that are inside camera view boundaries.
        for (u32 i = 0; i < array_list_length(&quads_list); i++) {
            // Selecting quad if it's center is touched.
            if (vec2f_distance(quad_center(&quads_list[i].quad), world_mouse_position) < editor_params.selection_radius) {
                // @Redundant?
                // Quad is selected, add it only if it is not already selected.
                if (!(quads_list[i].flags & EDITOR_QUAD_BITMASK_ANY_POINT_SELECTED)) {
                    quads_list[i].flags |= EDITOR_QUAD_BITMASK_ANY_POINT_SELECTED;
                    array_list_append(&editor_selected, ((Editor_Selected) {
                                .type = EDITOR_QUAD,
                                .quad = &quads_list[i],
                                }));
                }

                goto editor_left_mouse_select_end;
                break;
            }
        }

editor_left_mouse_select_end:
    }



    // @Clean: Remove edito_selected.type != EDITOR_NONE?
    if (mouse_input_ptr->left_hold) {
        // One selected element.
        if (array_list_length(&editor_selected) > 0) {
            Vec2f selected_move_offset = vec2f_difference(world_mouse_position, mouse_left_click_origin);

            // For all selected elements.
            for (u32 i = 0; i < array_list_length(&editor_selected); i++) {
                switch(editor_selected[i].type) {
                    case EDITOR_QUAD:
                        for (u32 j = 0; j < VERTICIES_PER_QUAD; j++) {
                            if (editor_selected[i].quad->flags & (1 << j)) {
                                editor_selected[i].quad->quad.verts[j].x += world_mouse_position_change.x;
                                editor_selected[i].quad->quad.verts[j].y += world_mouse_position_change.y;
                            }
                        }
                        break;
                }
            }
            mouse_left_click_origin;
        } else {

        }

    }
    

    

    return false;
}

void editor_draw() {
    Matrix4f projection;

    projection = camera_calculate_projection(&editor_camera, window_ptr->width, window_ptr->height);
    
    

    // Drawing quads.
    shader_update_projection(quad_drawer_ptr->program, &projection);

    draw_begin(quad_drawer_ptr);

    // Draw editor quads.
    for (u32 i = 0; i < array_list_length(&quads_list); i++) {
        draw_quad(quads_list[i].quad.verts[0], quads_list[i].quad.verts[1], quads_list[i].quad.verts[2], quads_list[i].quad.verts[3], .color = quads_list[i].color);

        for (u32 j = 0; j < VERTICIES_PER_QUAD; j++) {
            if (quads_list[i].flags & (1 << j)) {
                draw_dot(quads_list[i].quad.verts[j], VEC4F_RED, &editor_camera, NULL);
            } else {
                draw_dot(quads_list[i].quad.verts[j], VEC4F_CYAN, &editor_camera, NULL);
            }
        }
    }

    draw_end();


    // Drawing quad outlines.
    shader_update_projection(line_drawer_ptr->program, &projection);

    line_draw_begin(line_drawer_ptr);

    // Draw editor quads.
    for (u32 i = 0; i < array_list_length(&quads_list); i++) {
        
        if (quads_list[i].flags & EDITOR_QUAD_BITMASK_ANY_POINT_SELECTED) {
            draw_quad_outline(quads_list[i].quad.verts[0], quads_list[i].quad.verts[1], quads_list[i].quad.verts[2], quads_list[i].quad.verts[3], VEC4F_RED, NULL);
            draw_cross(quad_center(&quads_list[i].quad), VEC4F_RED, &editor_camera, NULL);
        } else {
            draw_quad_outline(quads_list[i].quad.verts[0], quads_list[i].quad.verts[1], quads_list[i].quad.verts[2], quads_list[i].quad.verts[3], VEC4F_WHITE, NULL);
            draw_cross(quad_center(&quads_list[i].quad), VEC4F_WHITE, &editor_camera, NULL);
        }
    }

    line_draw_end();



    // Set ui to use specific font.
    ui_set_font(&font_small);



    projection = screen_calculate_projection(window_ptr->width, window_ptr->height);
    shader_update_projection(ui_quad_drawer_ptr->program, &projection);

    draw_begin(ui_quad_drawer_ptr);

    // Draw editor ui.
    UI_WINDOW(window_ptr->width, window_ptr->height, 
            
            ui_text(
                str_format(info_buffer, 
                    "Window size: %dx%d\n"
                    "Vert count: %u\n"
                    "World mouse position: (%2.2f, %2.2f)\n"
                    "Camera unit scale: %d\n"
                    , window_ptr->width, window_ptr->height, array_list_length(&quads_list) * 4, world_mouse_position.x, world_mouse_position.y, editor_camera.unit_scale)
            );
    );

    draw_end();
}

void editor_get_verticies(Vec2f **verticies, s64 *verticies_count) {
    TODO("Getting editor result.");
}

void editor_add_quad() {
    array_list_append(&quads_list, ((Editor_Quad) {
                .flags = 0,
                .quad = ((Quad) {{ { -1.0f, -1.0f }, { 1.0f, -1.0f, }, { -1.0f, 1.0f }, { 1.0f, 1.0f } }}),
                .color = { randf() * 0.6f + 0.2f, randf() * 0.6f + 0.2f, randf() * 0.6f + 0.2f, 1.0f },
                }));
}
