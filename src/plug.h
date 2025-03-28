#ifndef PLUG_H_
#define PLUG_H_

#include "core.h"

// Game entities.
typedef struct player {
    Vec2f center;
    float width;
    float height;
    float speed;
    Vec2f velocity;
} Player;

/**
 * Definition of plug_state.
 * @Important: All global variables should be saved and organized in the struct.
 */
typedef struct plug_state {
    bool quit;
    Time_Data *t;
    u32 window_width;
    u32 window_height;

    /**
     * Globals, @Important: Must be loaded or/and unloaded when hot reloading plug.
     */
    Vec4f clear_color;
    Camera main_camera;
    
    Shader *shader_table;
    Font_Baked *font_table;

    Quad_Drawer drawer;
    Line_Drawer line_drawer;

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
