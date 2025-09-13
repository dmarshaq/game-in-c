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






typedef struct editor_quad {
    Vec2f verticies[VERTICIES_PER_QUAD];
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


@Introspect;
typedef struct editor_params {
    float selection_radius;
} Editor_Params;

static Editor_Params editor_params;



static Editor_Quad *quads_list;




static Vec2f world_mouse_position;

typedef enum editor_selected_type : u8 {
    EDITOR_NONE,
    EDITOR_VERTEX,
    EDITOR_QUAD,
} Editor_Selected_Type;

typedef struct editor_selected {
    Editor_Selected_Type type;

    union {
        Vec2f *vertex;
        Editor_Quad *quad;
    }
} Editor_Selected;

// @Temporary: Later it is going to an array of selected things in editor.
static Editor_Selected editor_selected;




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
    editor_params.selection_radius = 0.1f;
    
    vars_tree_add(TYPE_OF(editor_params), (u8 *)&editor_params, CSTR("editor_params"));


    // Setting pointers to global state.
    quad_drawer_ptr    = &state->quad_drawer;
    ui_quad_drawer_ptr = &state->ui_quad_drawer;
    line_drawer_ptr    = &state->line_drawer;
    window_ptr         = &state->window;
    mouse_input_ptr    = &state->events.mouse_input;
    time_ptr           = &state->t;




    quads_list = array_list_make(Editor_Quad, 8, &std_allocator);

    
    // @Copypasta: From console.c ... 
    // Get resources.

    // Load needed font... Hard coded...
    u8* font_data = read_file_into_buffer("res/font/Consolas-Regular.ttf", NULL, &std_allocator);

    font_small  = font_bake(font_data, 14.0f);;
    font_medium = font_bake(font_data, 20.0f);

    free(font_data);
    

    // Copying main camera for editor.
    editor_camera = state->main_camera;


    // Make arena and allocate space for the buffers.
    arena = arena_make(ARENA_SIZE);

    info_buffer.data   = arena_alloc(&arena, 256);
    info_buffer.length = 256;
}

bool editor_update() {
    world_mouse_position = screen_to_camera(mouse_input_ptr->position, &editor_camera, window_ptr->width, window_ptr->height);

    if (mouse_input_ptr->left_pressed) {
        console_log("Editor: Select at (%2.2f, %2.2f)\n", world_mouse_position.x, world_mouse_position.y);

        // @Temporary: In the future loop over the verticies that are inside camera view boundaries.
        for (u32 i = 0; i < array_list_length(&quads_list); i++) {
            for (u32 j = 0; j < VERTICIES_PER_QUAD; j++) {

                // Selecting vertex.
                if (vec2f_distance(quads_list[i].verticies[j], world_mouse_position) < editor_params.selection_radius) {
                    editor_selected = (Editor_Selected) {
                        .type = EDITOR_VERTEX,
                        .vertex = &quads_list[i].verticies[j],
                    };
                    
                    goto editor_left_mouse_press_end;
                }
            }
        }
        // If nothing was selected on mouse click. Select nothing.
        editor_selected.type = EDITOR_NONE;

editor_left_mouse_press_end:
    }

    // @Clean: Remove edito_selected.type != EDITOR_NONE?
    if (mouse_input_ptr->left_hold && editor_selected.type != EDITOR_NONE) {
        switch (editor_selected.type) {
            case EDITOR_VERTEX:
                *editor_selected.vertex = world_mouse_position;
                break;
            default:
                console_error("Editor: Unsupported selected type.\n");
                break;
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
                    "Selection radius: %2.2f\n"
                    , window_ptr->width, window_ptr->height, array_list_length(&quads_list) * 4, world_mouse_position.x, world_mouse_position.y, editor_params.selection_radius)
            );
    );

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
