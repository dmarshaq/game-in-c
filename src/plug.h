#ifndef PLUG_H_
#define PLUG_H_

#include "core.h"

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

/**
 * Definition of plug_state.
 * @Important: All global variables should be saved and organized in the struct.
 */
typedef struct plug_state {
    SDL_Window *window;
    u32 window_width;
    u32 window_height;
    SDL_Event event;
    Time_Data *t;
    bool quit;
    Mouse_Input mouse_input;

    /**
     * Globals, @Important: Must be loaded or/and unloaded when hot reloading plug.
     */
    Vec4f clear_color;
    Camera main_camera;
    
    Shader *shader_table;
    Font_Baked *font_table;

    Quad_Drawer drawer;
    Line_Drawer line_drawer;
    float *debug_vert_buffer;


    Impulse *impulses;
    Phys_Box **phys_boxes;

    Box *boxes;

    /**
     * Unsorted.
     */
    Player player;
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
