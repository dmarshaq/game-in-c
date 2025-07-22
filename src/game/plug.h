#ifndef PLUG_H_
#define PLUG_H_

#include "core/core.h"
#include "core/mathf.h"

#include "game/graphics.h"
#include "game/input.h"
#include "game/event.h"
#include "game/physics.h"
#include "game/imui.h"


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

// For console test function must be available for other files scopes.
void spawn_box(Vec2f position, Vec4f color);
void quit();

typedef enum game_state {
    PLAY,
    MENU,
} Game_State;




/**
 * Definition of plug_state.
 *
 *
 *
 * @Todo: Divide global state on smalle sub states and pack them together so code pieces can choose what part of global state is specifically used at the moment rather than throwing all in one group and everytime time accessing all variables at the same time.
 * @Todo: Divide plug state even more, and make dynamic global vars loading from some kind of variables file or context. Can be used .json but it probably more reasonable to use straight up basic key value format.
 */
typedef struct plug_state {
    Window_Info window;
    Events_Info events;
    Time_Info t;


    UI_State ui;


    /**
     * Game Globals, @Important: Must be loaded or/and unloaded when hot reloading plug.
     */
    Vec4f clear_color; // @Refactor: Reduntant variable since there is gl function to set clear color.
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

#ifdef DEV
    bool should_hot_reload;
#endif

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
