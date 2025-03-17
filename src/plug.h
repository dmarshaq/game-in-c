#ifndef PLUG_H_
#define PLUG_H_

#include "core.h"

typedef struct {
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
    float angle;
} Plug_State;

typedef void (*Plug_Init)(Plug_State *state);
typedef void (*Plug_Update)(Plug_State *state);

typedef void (*Plug_Load)(Plug_State *state);
typedef void (*Plug_Unload)(Plug_State *state);

#endif
