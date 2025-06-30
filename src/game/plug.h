#ifndef PLUG_H_
#define PLUG_H_

#include "core/core.h"
#include "core/graphics.h"
#include "core/mathf.h"
#include "core/input.h"

#define calculate_obb_inertia(mass, width, height)                                          ((1.0f / 12.0f) * mass * (height * height + width * width))

typedef struct body_2d {
    Vec2f velocity;
    float angular_velocity;

    float mass;
    float inv_mass;
    float inertia;
    float inv_inertia;
    Vec2f mass_center;
    float restitution;
    float static_friction;
    float dynamic_friction;
} Body_2D;

#define body_obb_make(mass, center, width, height, restitution, static_friction, dynamic_friction)                             ((Body_2D) { VEC2F_ORIGIN, 0.0f, mass, (mass == 0.0f ? 0.0f : 1.0f / mass), calculate_obb_inertia(mass, width, height), (mass == 0.0f ? 0.0f : 1.0f / calculate_obb_inertia(mass, width, height)), center, restitution, static_friction, dynamic_friction })


typedef struct impulse {
    Vec2f delta_force;
    u32 milliseconds;
    Body_2D *target;
} Impulse;




/**
 * Some of these flags in theory can be moved to rigid body 2d to abstact shape from body when resolving collisions.
 */
typedef struct phys_box {
    OBB bound_box;
    Body_2D body;
    
    bool dynamic;
    bool rotatable;
    bool destructible;
    bool gravitable;
    bool grounded;

} Phys_Box;




typedef enum game_state {
    PLAY,
    MENU,
} Game_State;


typedef struct ui_theme {
    Vec4f bg;
    Vec4f light;
    Vec4f btn_bg;
    Vec4f btn_bg_hover;
    Vec4f btn_bg_press;
    Vec4f text;
} UI_Theme;


typedef struct ui_frame {
    Vec2f origin;
    Vec2f size;
} UI_Frame;

typedef enum ui_alignment : s8 {
    UI_ALIGN_DEFAULT  = 0,
    UI_ALIGN_OPPOSITE = 1,
} UI_Alignment;



typedef struct ui_state {
    // Cursor
    Vec2f cursor;
    UI_Frame *frame_stack;

    // Alignment
    UI_Alignment x_axis;
    UI_Alignment y_axis;
    
    // Advancing
    float line_height;
    Vec2f element_size;
    bool sameline;

    // Element activation
    s32 active_line_id;
    s32 active_prefix_id;
    s32 set_prefix_id;
    bool activate_next;

    // Customization
    Font_Baked *font;
    UI_Theme theme;
} UI_State;



/**
 * Game entities.
 * 
 * @Important: Comment below is still valid to some extent, but the solution is not ideal, it strictly depended on how data is used, and updated, so using arrays of pointers as data might not be ideal solution due to potential cache issues when updating large data sets.
 *
 * In this game it seems unnecessary to introduce complex ECS systems, so this game will follow more basic approach where each "game entity" is predifined as its own struct containing all necessary data.
 * The problem arises when some organized generic algorithm like physics resolution tries to use these predefined entities.
 * Generally the pointer to the Phys_Box value can just be passed to some physics arraylist for it to resolve the collision and other physics related calculations.
 * But if original entity is destroyed the ptr inside arraylist will no longer point to a valid memory, and additionally original memory of the entity cannot be moved anywhere cause ptr will not "track" it and as result will point to a wrong location.
 * Yet such pointers are VERY usefull in game logics, so, to resolve such issue it is necessary to:
 *      1) Gurantee that ANY entities memory stays in place once created.
 *      2) Once entity is "unalived" make sure to "nullify" specific parts of the memory so ptrs know that this memory is no longer good.
 *      3) Make sure to check each and every ptr to entity memory to see if it is "nullified".
 *      4) Comment such checks so it is easier to understand what is done.
 */

typedef struct sword {
    Transform origin;
    Transform handle;

    float angle_a;
    float angle_b;
    T_Interpolator animator;
} Sword;


typedef struct player {
    Phys_Box p_box;
    Vec4f color;
    float speed;
    
    Sword sword;
} Player;


typedef struct box {
    Phys_Box p_box;
    Vec4f color;

    bool destroyed;
} Box;




















typedef struct mouse_input {
    Vec2f position;
    bool left_hold;
    bool left_pressed;
    bool left_unpressed;
    bool right_hold;
    bool right_pressed;
    bool right_unpressed;
} Mouse_Input;

typedef struct text_input {
    char *buffer;
    s64 capacity;
    s64 length;
    s64 write_index;
} Text_Input;

typedef struct events_info {
    bool should_quit;
    Mouse_Input mouse_input;
    Text_Input text_input;
} Events_Info;



typedef struct window_info {
    SDL_Window *ptr;
    u32 width;
    u32 height;
} Window_Info;


/**
 * Definition of plug_state.
 *
 * @Todo: Divide global state on smalle sub states and pack them together so code pieces can choose what part of global state is specifically used at the moment rather than throwing all in one group and everytime time accessing all variables at the same time.
 */
typedef struct plug_state {
    Window_Info window;
    Events_Info events;
    Time_Data *t;


    UI_State ui;


    /**
     * Game Globals, @Important: Must be loaded or/and unloaded when hot reloading plug.
     */
    Vec4f clear_color;
    Camera main_camera;
    
    Shader *shader_table;
    Font_Baked *font_table;

    Quad_Drawer quad_drawer;
    Quad_Drawer grid_drawer;
    Quad_Drawer ui_drawer;
    Line_Drawer line_drawer;
    float *debug_vert_buffer;


    Impulse *impulses;
    Phys_Box **phys_boxes;

    Box *boxes;

    /**
     * Unsorted.
     */
    Player player;
    Game_State gs;


} Plug_State;






#ifdef DEV
typedef void (*Plug_Init)(Plug_State *state);
typedef void (*Plug_Update)(Plug_State *state);
typedef void (*Plug_Load)(Plug_State *state);
typedef void (*Plug_Unload)(Plug_State *state);
#else
void plug_init(Plug_State *state);
void plug_update(Plug_State *state);
void plug_load(Plug_State *state);
void plug_unload(Plug_State *state);
#endif

#endif
