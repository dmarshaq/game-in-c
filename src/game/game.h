#ifndef GAME_H
#define GAME_H

#include "core/core.h"
#include "core/mathf.h"

#include "game/graphics.h"
#include "game/input.h"
#include "game/event.h"
#include "game/physics.h"
#include "game/imui.h"
#include "game/vars.h"







typedef struct state {
    Window_Info window;
    Events_Info events;
    Time_Info t;

    Vars_Tree vars_tree;

    Camera main_camera;
    
    Shader     *shader_table;
    Font_Baked *font;

    Quad_Drawer quad_drawer;
} State;


/**
 * Inits all neccessary game structures and sets game ready to be updated.
 */
void game_init(State *state);

/**
 * Updates game, should be called one time a frame.
 * @Important: There is no divide between drawing and updating here, cause this function literally represents a whole frame in the game, no more, no less.
 */
void game_update();

/**
 * Simply frees all initted game memory.
 */
void game_free();

/**
 * This function will notify app to quit at the end of the next frame.
 */
           ;
                ;
void quit();

#endif
