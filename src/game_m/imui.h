#ifndef IMUI_H
#define IMUI_H

#include "game/graphics.h"
#include "game/event.h"


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




void ui_draw_rect(Vec2f position, Vec2f size, Vec4f color);

void ui_draw_text(char *text, Vec2f position, Vec4f color);

void ui_draw_text_centered(char *text, Vec2f position, Vec2f size, Vec4f color);


void ui_init(UI_State *ui_state, Mouse_Input *mouse_input);

void ui_push_frame(float x, float y, float width, float height);

void ui_pop_frame();

void ui_cursor_reset();

void ui_cursor_advance(Vec2f size);

void ui_set_element_size(Vec2f size);

void ui_sameline();

void ui_set_prefix(s32 id);

void ui_activate_next();

#define UI_FRAME(width, height, code)\
    ui_frame_start(width, height);\
    code\
    ui_frame_end(width, height)\

void ui_frame_start(float width, float height);
void ui_frame_end(float width, float height);

#define UI_WINDOW(window_w, window_h, code)\
    ui_push_frame(0, 0, window_w, window_h);\
    ui_cursor_reset();\
    code\
    ui_pop_frame()\


#define UI_BUTTON(size, text) ui_button(size, text, __LINE__)
bool ui_button(Vec2f size, char *text, s32 id);

#define UI_SLIDER_INT(size, output, min, max) ui_slider_int(size, output, min, max, __LINE__)
bool ui_slider_int(Vec2f size, s32 *output, s32 min, s32 max, s32 id);

#define UI_SLIDER_FLOAT(size, output, min, max) ui_slider_float(size, output, min, max, __LINE__)
bool ui_slider_float(Vec2f size, float *output, float min, float max, s32 id);


void ui_text(char *text);

#endif
