#include "game/imui.h"

#include "core/graphics.h"
#include "core/structs.h"

#include "game/draw.h"
#include "game/event.h"

/**
 * User Interface.
 */


static UI_State *ui;
static Mouse_Input *mouse_i;


// UI Origin logic.
Vec2f ui_current_frame_origin() {
    if (array_list_length(&ui->frame_stack) < 1) {
        printf_err("UI Current frame doesn't exist.\n");
        return VEC2F_ORIGIN;
    }
    return ui->frame_stack[array_list_length(&ui->frame_stack) - 1].origin;
}

Vec2f ui_current_frame_size() {
    if (array_list_length(&ui->frame_stack) < 1) {
        printf_err("UI Current frame doesn't exist.\n");
        return VEC2F_ORIGIN;
    }
    return ui->frame_stack[array_list_length(&ui->frame_stack) - 1].size;
}

void ui_push_frame(float x, float y, float width, float height) {
    array_list_append(&ui->frame_stack, ((UI_Frame){
            .origin = vec2f_make(x, y),
            .size = vec2f_make(width, height),
            }) );
}

void ui_pop_frame() {
    array_list_pop(&ui->frame_stack);
}



// UI Init
void ui_init(UI_State *ui_state, Mouse_Input *mouse_input) {
    ui = ui_state;
    mouse_i = mouse_input;

    ui->cursor = vec2f_make(0, 0);
    ui->frame_stack = array_list_make(UI_Frame, 8, &std_allocator); // Maybe replace with the custom defined arena allocator later. @Leak.

    ui->x_axis = UI_ALIGN_OPPOSITE;
    ui->y_axis = UI_ALIGN_OPPOSITE;

    ui->element_size = VEC2F_ORIGIN;
    ui->sameline = false;

    ui->active_line_id = -1;
    ui->active_prefix_id = -1;
    ui->set_prefix_id = -1;

    ui->theme = (UI_Theme) {
        .bg             = (Vec4f){ 0.12f, 0.12f, 0.14f, 1.0f },   // Dark slate background
        .light          = (Vec4f){ 0.85f, 0.85f, 0.88f, 1.0f },   // Soft light gray
        .btn_bg         = (Vec4f){ 0.12f, 0.16f, 0.28f, 1.0f },   // Deeper, richer base button
        .btn_bg_hover   = (Vec4f){ 0.20f, 0.26f, 0.40f, 1.0f },   // Gentle contrast on hover
        .btn_bg_press   = (Vec4f){ 0.08f, 0.10f, 0.22f, 1.0f },   // Subtle shadowy press state
        .text           = (Vec4f){ 0.97f, 0.96f, 0.92f, 1.0f },   // Warm white text
    };
}

// UI prefix id.
void ui_set_prefix(s32 id) {
    ui->set_prefix_id = id;
}

void ui_activate_next() {
    ui->activate_next = true;
}

void ui_end_element() {
    ui->set_prefix_id = -1;
    ui->activate_next = false;
}

// UI Alignment.
Vec2f ui_current_aligned_origin() {
    Vec2f origin = ui_current_frame_origin();
    Vec2f size = ui_current_frame_size();
    if (ui->x_axis > 0)
        origin.x += size.x;
    if (ui->y_axis > 0)
        origin.y += size.y;
    return origin;
}


// UI Cursor related logic.
void ui_cursor_reset() {
    ui->cursor = ui_current_aligned_origin();
    ui->element_size = VEC2F_ORIGIN;
    ui->sameline = false;
}

/**
 * Advances cursor based on "sameline", "element_size", and next element "size" values, according to alignment.
 */
void ui_cursor_advance(Vec2f size) {
    if (ui->sameline) {
        ui->cursor.y += ui->y_axis * (ui->line_height - size.y);
        ui->line_height = size.y > ui->line_height ? size.y : ui->line_height;
        ui->cursor.x += !ui->x_axis * ui->element_size.x - ui->x_axis * size.x;
        ui->sameline = false;
    } else {
        ui->line_height = size.y;
        ui->cursor.x = ui_current_aligned_origin().x - ui->x_axis * size.x;
        ui->cursor.y += !ui->y_axis * ui->element_size.y - ui->y_axis * size.y;
    }
}

void ui_set_element_size(Vec2f size) {
    ui->element_size = size;
}

void ui_sameline() {
    ui->sameline = true;
}









// UI Frame.
void ui_frame_start(float width, float height) {
    ui_cursor_advance(vec2f_make(width, height));
    ui_draw_rect(ui->cursor, vec2f_make(width, height), ui->theme.bg);
    ui_push_frame(ui->cursor.x, ui->cursor.y, width, height);
    ui_cursor_reset();
}

void ui_frame_end(float width, float height) {
    ui_cursor_reset();
    ui_pop_frame();
    ui_cursor_advance(vec2f_make(width, height));
    ui_set_element_size(vec2f_make(width, height));
}



void ui_draw_rect(Vec2f position, Vec2f size, Vec4f color) {
    Vec2f p0 = position;
    Vec2f p1 = vec2f_sum(position, size);

    float quad_data[48] = {
        p0.x, p0.y, 0.0f, color.x, color.y, color.z, color.w, 0.0f, 0.0f, size.x, size.y, -1.0f,
        p1.x, p0.y, 0.0f, color.x, color.y, color.z, color.w, 1.0f, 0.0f, size.x, size.y, -1.0f,
        p0.x, p1.y, 0.0f, color.x, color.y, color.z, color.w, 0.0f, 1.0f, size.x, size.y, -1.0f,
        p1.x, p1.y, 0.0f, color.x, color.y, color.z, color.w, 1.0f, 1.0f, size.x, size.y, -1.0f,
    };
    
    draw_quad_data(quad_data, 1);
}



    







bool ui_is_hover(Vec2f size) {
    return value_inside_domain(ui->cursor.x, ui->cursor.x + size.x, mouse_i->position.x) && value_inside_domain(ui->cursor.y, ui->cursor.y + size.y, mouse_i->position.y);
}

void ui_draw_text(char *text, Vec2f position, Vec4f color) {
    u64 text_length = strlen(text);

    // Scale and adjust current_point.
    Vec2f current_point = position;
    Font_Baked *font = ui->font;
    float origin_x = current_point.x;
    current_point.y += (float)font->baseline;

    // Text rendering variables.
    s32 font_char_index;
    u16 width, height;
    stbtt_bakedchar *c;

    for (u64 i = 0; i < text_length; i++) {
        if (text[i] == '\n') {
            current_point.x = origin_x;
            current_point.y -= (float)font->line_height;
            continue;
        }

        // Character drawing.
        font_char_index = (s32)text[i] - font->first_char_code;
        if (font_char_index < font->chars_count) {
            c = &font->chars[font_char_index];
            width  = font->chars[font_char_index].x1 - font->chars[font_char_index].x0;
            height = font->chars[font_char_index].y1 - font->chars[font_char_index].y0;


            Vec2f p0 = vec2f_make(current_point.x + c->xoff, current_point.y - c->yoff - height);
            Vec2f p1 = vec2f_make(current_point.x + c->xoff + width, current_point.y - c->yoff);

            Vec2f uv0 = vec2f_make(c->x0 / (float)font->bitmap.width, c->y1 / (float)font->bitmap.height);
            Vec2f uv1 = vec2f_make(c->x1 / (float)font->bitmap.width, c->y0 / (float)font->bitmap.height);

            float mask_slot = add_texture_to_slots(&font->bitmap);              

            float quad_data[48] = {
                p0.x, p0.y, 0.0f, color.x, color.y, color.z, color.w, uv0.x, uv0.y, 1.0f, 1.0f, mask_slot,
                p1.x, p0.y, 0.0f, color.x, color.y, color.z, color.w, uv1.x, uv0.y, 1.0f, 1.0f, mask_slot,
                p0.x, p1.y, 0.0f, color.x, color.y, color.z, color.w, uv0.x, uv1.y, 1.0f, 1.0f, mask_slot,
                p1.x, p1.y, 0.0f, color.x, color.y, color.z, color.w, uv1.x, uv1.y, 1.0f, 1.0f, mask_slot,
            };

            draw_quad_data(quad_data, 1);


            current_point.x += font->chars[font_char_index].xadvance;
        }

    }
}

void ui_draw_text_centered(char *text, Vec2f position, Vec2f size, Vec4f color) {
    if (text == NULL)
        return;

    Vec2f t_size = text_size(CSTR(text), ui->font);

    // Following ui_draw_text centered calculations are for the system where y axis points up, and x axis to the right.
    ui_draw_text(text, vec2f_make(position.x + (size.x - t_size.x) * 0.5f, position.y + (size.y + t_size.y) * 0.5f), color);
}


// UI Button

bool ui_button(Vec2f size, char *text, s32 id) {
    ui_cursor_advance(size);
    ui_set_element_size(size);

    s32 prefix_id = ui->set_prefix_id;

    Vec4f res_color = ui->theme.btn_bg;
    bool  res_ret   = false;
    

    if (ui_is_hover(size) || ui->activate_next) {
        if (ui->active_line_id == id && ui->active_prefix_id == prefix_id) {
            if (mouse_i->left_unpressed) {
                ui->active_line_id = -1;
                ui->active_prefix_id = -1;

                res_color = ui->theme.btn_bg_hover;
                res_ret   = true;
                goto leave_draw;
            }

            res_color = ui->theme.btn_bg_press;
            goto leave_draw;
        }

        if (mouse_i->left_pressed || ui->activate_next) {
            ui->active_line_id = id;
            ui->active_prefix_id = prefix_id;
        }

        res_color = ui->theme.btn_bg_hover;

    } else if (ui->active_line_id == id && ui->active_prefix_id == prefix_id) {
        ui->active_line_id = -1;
        ui->active_prefix_id = -1;
    } 

leave_draw:
    
    ui_draw_rect(ui->cursor, size, res_color);
    ui_draw_text_centered(text, ui->cursor, size, ui->theme.text);

    ui_end_element();

    return res_ret;
}


// UI Slider



bool ui_slider_int(Vec2f size, s32 *output, s32 min, s32 max, s32 id) {
    ui_cursor_advance(size);
    ui_set_element_size(size);

    s32 prefix_id = ui->set_prefix_id;

    float scale = (max - min) / size.x;

    Vec4f res_color = ui->theme.btn_bg;
    Vec4f res_knob_color = ui->theme.btn_bg_hover;
    bool  res_ret   = false;
    
    if (ui->active_line_id == id && ui->active_prefix_id == prefix_id) {
        if (mouse_i->left_hold) {
            // If was pressed, and holding.
            float a = mouse_i->position.x * scale - (ui->cursor.x * scale - min);
            *output = (s32)clamp(a, min, max);

            res_color = ui->theme.btn_bg_press;
            res_knob_color = ui->theme.text;
            res_ret   = true;
            goto leave_draw;
        } else {
            // If was pressed, but not holding anymore.
            ui->active_line_id = -1;
            ui->active_prefix_id = -1;
            goto leave_draw;
        }
    }
    
    if (ui_is_hover(size)) {
        // If just pressed.
        if (mouse_i->left_pressed) {
            ui->active_line_id = id;
            ui->active_prefix_id = prefix_id;
        }

        res_color = ui->theme.btn_bg_hover;
    } 


leave_draw:

    ui_draw_rect(vec2f_make(ui->cursor.x, ui->cursor.y + size.y * 0.5f - 5), vec2f_make(size.x, 10), res_color); // Slider line.
    ui_draw_rect(vec2f_make(ui->cursor.x + (*output - min) / scale - 8, ui->cursor.y + size.y * 0.5f - 8), vec2f_make(16, 16), res_knob_color); // Slider knob.

    ui_end_element();

    return res_ret;
}


bool ui_slider_float(Vec2f size, float *output, float min, float max, s32 id) {
    ui_cursor_advance(size);
    ui_set_element_size(size);

    s32 prefix_id = ui->set_prefix_id;

    float scale = (max - min) / size.x;

    Vec4f res_color = ui->theme.btn_bg;
    Vec4f res_knob_color = ui->theme.btn_bg_hover;
    bool  res_ret   = false;
    
    if (ui->active_line_id == id && ui->active_prefix_id == prefix_id) {
        if (mouse_i->left_hold) {
            // If was pressed, and holding.
            float a = mouse_i->position.x * scale - (ui->cursor.x * scale - min);
            *output = clamp(a, min, max);

            res_color = ui->theme.btn_bg_press;
            res_knob_color = ui->theme.text;
            res_ret   = true;
            goto leave_draw;
        } else {
            // If was pressed, but not holding anymore.
            ui->active_line_id = -1;
            ui->active_prefix_id = -1;
            goto leave_draw;
        }
    }
    
    if (ui_is_hover(size)) {
        // If just pressed.
        if (mouse_i->left_pressed) {
            ui->active_line_id = id;
            ui->active_prefix_id = prefix_id;
        }

        res_color = ui->theme.btn_bg_hover;
    } 


leave_draw:

    ui_draw_rect(vec2f_make(ui->cursor.x, ui->cursor.y + size.y * 0.5f - 5), vec2f_make(size.x, 10), res_color); // Slider line.
    ui_draw_rect(vec2f_make(ui->cursor.x + (*output - min) / scale - 8, ui->cursor.y + size.y * 0.5f - 8), vec2f_make(16, 16), res_knob_color); // Slider knob.
    // ui_draw_rect(vec2f_make(ui->cursor.x + (*output - min) / scale - 5, ui->cursor.y), vec2f_make(10, size.y), res_knob_color); // Slider knob bar.

    ui_end_element();

    return res_ret;
}


// UI Input field

#define UI_INPUT_FIELD(size, buffer, buffer_size) ui_input_field(size, buffer, buffer_size, __LINE__)

bool ui_input_field(Vec2f size, char *buffer, s32 buffer_size, s32 id) {
    // Mouse_Input *mouse_i = &mouse_i->
    // Text_Input *text_i = &state->events.text_input;

    // ui_cursor_advance(size);
    // ui_set_element_size(size);

    // s32 prefix_id = ui->set_prefix_id;

    // Vec4f res_color = ui->theme.btn_bg;
    // Vec4f res_bar_color = ui->theme.btn_bg_hover;
    // bool  res_flushed = false;

    // if (ui->active_line_id == id && ui->active_prefix_id == prefix_id) {
    //     if (mouse_i->left_pressed || pressed(SDLK_ESCAPE)) {
    //         // If was pressed but was pressed again outside (deselected)
    //         ui->active_line_id = -1;
    //         ui->active_prefix_id = -1;

    //         SDL_StopTextInput();

    //         text_i->buffer = NULL;
    //         text_i->capacity = 0;
    //         text_i->length = 0;
    //         text_i->write_index = 0;


    //         goto leave_draw;
    //     }

    //     res_color = ui->theme.btn_bg_press;

    //     if (pressed(SDLK_RETURN)) {
    //         res_flushed = true;
    //         goto leave_draw;
    //     } 


    //     // If is still active.
    //     


    // }



    // if (ui_is_hover(size) || ui->activate_next) {
    //     // If just pressed.
    //     if (mouse_i->left_pressed || ui->activate_next) {
    //         ui->active_line_id = id;
    //         ui->active_prefix_id = prefix_id;
    //         
    //         SDL_StartTextInput();

    //         // Setting text input for appending.
    //         text_i->buffer = buffer;
    //         text_i->capacity = buffer_size;
    //         text_i->length = strlen(buffer);
    //         text_i->write_index = text_i->length;
    //         
    //     }

    //     res_color = ui->theme.btn_bg_hover;

    //     goto leave_draw;
    // }



//leav// e_draw:
    //  Vec2f t_size = text_size(CSTR(buffer), ui->font);
    //  ui_draw_rect(ui->cursor, size, res_color); // Input field.
    // // Please rework
    //  // ui_draw_rect(vec2f_make(ui->cursor.x + text_size(CSTR(buffer), text_i->write_index, ui->font).x, ui->cursor.y), vec2f_make(10, size.y), res_bar_color); // Input bar.
    //  ui_draw_text(buffer, vec2f_make(ui->cursor.x, ui->cursor.y + (size.y + t_size.y) * 0.5f), ui->theme.text);

    //  ui_end_element();

    //  return res_flushed;
    return false;
}




void ui_text(char *text) {
    Vec2f t_size = text_size(CSTR(text), ui->font);
    ui_cursor_advance(t_size);
    ui_set_element_size(t_size);
    ui_draw_text(text, vec2f_make(ui->cursor.x, ui->cursor.y + t_size.y), ui->theme.text);
}


