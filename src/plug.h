#ifndef PLUG_H_
#define PLUG_H_

#include "core.h"

typedef struct impulse {
    Vec2f delta_force;
    u32 milliseconds;
    Body_2D *target;
} Impulse;

// Game entities.
typedef struct player {
    OBB bound_box;
    Body_2D body;
    float speed;
} Player;

typedef struct phys_box {
    OBB bound_box;
    Body_2D body;
    Vec4f color;
    bool is_static;
} Phys_Box;

/**
 * Definition of plug_state.
 * @Important: All global variables should be saved and organized in the struct.
 */
typedef struct plug_state {
    SDL_Event event;
    Time_Data *t;
    bool quit;
    u32 window_width;
    u32 window_height;
    Vec2f mouse_position;

    /**
     * Globals, @Important: Must be loaded or/and unloaded when hot reloading plug.
     */
    Vec4f clear_color;
    Camera main_camera;
    
    Shader *shader_table;
    Font_Baked *font_table;

    Quad_Drawer drawer;
    Line_Drawer line_drawer;

    Impulse *impulses;
    Phys_Box *phys_boxes;

    /**
     * Unsorted.
     */
    Player player;
} Plug_State;

typedef void (*Plug_Init)(Plug_State *state);
typedef void (*Plug_Update)(Plug_State *state);

typedef void (*Plug_Load)(Plug_State *state);
typedef void (*Plug_Unload)(Plug_State *state);

#endif
