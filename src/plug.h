#ifndef PLUG_H_
#define PLUG_H_

#include "core.h"

typedef struct {
    bool quit;
    Time_Data *t;

    /**
     * Globals, @Important: Must be loaded or/and unloaded when hot reloading plug.
     */
    Vec4f clear_color;
    Camera main_camera;
    float angle;
    
    Shader quad_shader;
    Shader grid_shader;
    Quad_Drawer drawer;
    Font_Baked font_baked_medium;
    Font_Baked font_baked_small;
    Shader line_shader;
    Line_Drawer line_drawer;
} Plug_State;

typedef void (*Plug_Init)(Plug_State *state);
typedef void (*Plug_Update)(Plug_State *state);

typedef void (*Plug_Load)(Plug_State *state);
typedef void (*Plug_Unload)(Plug_State *state);

#endif
