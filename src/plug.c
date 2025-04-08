#include "plug.h"
#include "SDL2/SDL_keycode.h"
#include "core.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Helper functions interface.
 */

// Drawing basics.
void draw_quad(Vec2f p0, Vec2f p1, Vec4f color, Texture *texture, Vec2f uv0, Vec2f uv1, Texture *mask, float offset_angle);
void draw_text(const char *text, Vec2f position, Vec4f color, Font_Baked *font, u32 unit_scale);
void draw_line(Vec2f p0, Vec2f p1, Vec4f color);
void draw_dot(Vec2f position, Vec4f color);

// Drawing frames.
void draw_quad_outline(Vec2f p0, Vec2f p1, Vec4f color, float offset_angle);
void draw_circle_outline(Vec2f position, float radius, u32 detail, Vec4f color);

// Drawing function.
void draw_function(float x0, float x1, Function y, u32 detail, Vec4f color);
void draw_polar(float t0, float t1, Function r, u32 detail, Vec4f color);
void draw_parametric(float t0, float t1, Function x, Function y, u32 detail, Vec4f color);

// Drawing area under the curve.
void draw_area_function(float x0, float x1, Function y, u32 rect_count, Vec4f color);
void draw_area_polar(float t0, float t1, Function r, u32 rect_count, Vec4f color);
void draw_area_parametric(float t0, float t1, Function x, Function y, u32 rect_count, Vec4f color);

// Viewport manipulation.
void draw_viewport(u32 x, u32 y, u32 width, u32 height, Vec4f color, Camera *camera);
void viewport_reset(Plug_State *state);




Plug_State *global_state;

const Vec2f gravity_acceleration = vec2f_make(0.0f, -9.81f);
const float jump_input_leeway = 0.4f;
const float air_control_percent = 0.3f;

u32 time_slow_factor = 1;




/**
 * Pop up message.
 */

char pop_up_buffer[256];
u32 pop_up_buffer_start = 0;
float pop_up_lerp_t = 0.0f;


void pop_up_log(char *format, ...) {
    va_list args;
    va_start(args, format);
    s32 bytes_written = vsnprintf(pop_up_buffer + pop_up_buffer_start, sizeof(pop_up_buffer) - pop_up_buffer_start, format, args);
    va_end(args);
    pop_up_lerp_t = 0.0f;

    if (bytes_written > 0)
        pop_up_buffer_start += bytes_written;
}


/**
 * Draws pop_up text that is stored in "pop_up_mesg", doesn't draw anything if it is NULL.
 * Use "pop_up_log(char *format, ...)" to log message;
 */
void draw_pop_up(Vec2f position, Vec4f color, Font_Baked *font, u32 unit_scale) {
    if (pop_up_buffer[0] == '\0')
        return;
    
    color.w = lerp(color.w, 0.0f, pop_up_lerp_t);
    position.y = lerp(position.y, position.y + 25.0f, pop_up_lerp_t);

    draw_text(pop_up_buffer, position, color, font, unit_scale);

    pop_up_lerp_t += 0.002f;

    if (pop_up_lerp_t >= 1.0f) {
        pop_up_buffer[0] = '\0';
        pop_up_lerp_t = 0.0f;
    }

    pop_up_buffer_start = 0;
}





/**
 * Physics.
 *
 * Standard units used:
 *      Mass -> kg
 *      Speed -> m/s
 *      Acceleration -> m/s^2
 *      Force -> m/s^2 * kg -> N
 *
 * Important formulas:
 *  
 *  Instanteneous acceleration and force.
 *      Vi + a = Vf
 *      Vi + (F / m) = Vf
 *  
 *  Acceleration and force over period of time / continuous.
 *      Vi + a * dt = Vf
 *      Vi + (F / m) * dt = Vf
 */

/**
 * Applies instanteneous force to rigid body.
 */
void phys_apply_force(Rigid_Body_2D *rb, Vec2f force) {
    rb->velocity = vec2f_sum(rb->velocity, vec2f_divide_constant(force, rb->mass));
}

/**
 * Applies instanteneous acceleration to rigid body.
 */
void phys_apply_acceleration(Rigid_Body_2D *rb, Vec2f acceleration) {
    rb->velocity = vec2f_sum(rb->velocity, acceleration);
}



#define MAX_IMPULSES        16
#define MAX_GENERATORS      16

void phys_init() {
    // Setting physics variables.
    global_state->impulses = array_list_make(Impulse, MAX_IMPULSES, &std_allocator); // @Leak
}

void phys_add_impulse(Vec2f force, u32 milliseconds, Rigid_Body_2D *rb) {
    Impulse impulse = (Impulse) { 
        .delta_force = vec2f_divide_constant(force, (float)milliseconds), 
        .milliseconds = milliseconds, 
        .target = rb 
    };
    array_list_append(&global_state->impulses, impulse);
}

void phys_apply_impulses() {
    for (u32 i = 0; i < array_list_length(&global_state->impulses); i++) {
        u32 force_time_multi = global_state->t->delta_time_milliseconds;

        if (global_state->impulses[i].milliseconds < global_state->t->delta_time_milliseconds) {
            force_time_multi = global_state->impulses[i].milliseconds;
            global_state->impulses[i].milliseconds = 0;
        }
        else {
            global_state->impulses[i].milliseconds -= global_state->t->delta_time_milliseconds;
        }

        global_state->impulses[i].target->velocity = vec2f_sum(global_state->impulses[i].target->velocity, vec2f_multi_constant(vec2f_divide_constant(global_state->impulses[i].delta_force, global_state->impulses[i].target->mass), (float)force_time_multi));

        if (global_state->impulses[i].milliseconds == 0) {
            array_list_unordered_remove(&global_state->impulses, i);
            i--;
        }
    }
}

/**
 * Internal function.
 * @Important: "axis1" should always correspond to the axis alligned with "obb1".
 * Returns true if "obb1" and "obb2" projected points are touching on "axis1".
 */
bool phys_sat_check_axis(OBB *obb1, Vec2f axis1, OBB *obb2) {
    float min = vec2f_dot(axis1, obb_p0(obb1));
    float max = vec2f_dot(axis1, obb_p1(obb1));

    if (min > max) {
        min = max;
        max = vec2f_dot(axis1, obb_p0(obb1));
    }

    return value_inside_domain(min, max, vec2f_dot(axis1, obb_p0(obb2))) ||
        value_inside_domain(min, max, vec2f_dot(axis1, obb_p1(obb2))) ||
        value_inside_domain(min, max, vec2f_dot(axis1, obb_p2(obb2))) ||
        value_inside_domain(min, max, vec2f_dot(axis1, obb_p3(obb2)));
}

/**
 * Returns true of "obb1" and "obb2" touch.
 */
bool phys_sat_collision_obb(OBB *obb1, OBB *obb2) {
    return phys_sat_check_axis(obb1, obb_right(obb1), obb2) &&
        phys_sat_check_axis(obb1, obb_up(obb1), obb2) &&
        phys_sat_check_axis(obb2, obb_right(obb2), obb1) &&
        phys_sat_check_axis(obb2, obb_up(obb2), obb1);
}




void plug_init(Plug_State *state) {
    global_state = state;

    /**
     * Setting hash tables for resources. 
     * Since they are dynamically allocated pointers will not change after hot reloading.
     */
    state->shader_table = hash_table_make(Shader, 8, &std_allocator);
    state->font_table = hash_table_make(Font_Baked, 8, &std_allocator);
    
    state->player = (Player) {
        .bound_box = aabb_make_dimensions(vec2f_make(0.0f, 0.0f), 1.0f, 1.8f),
        .body = rb_2d_make(vec2f_make(0.0f, 0.0f), 70.0f),
        .speed = 0.0f,
    };

    phys_init();
}


void plug_load(Plug_State *state) {
    state->clear_color = vec4f_make(0.1f, 0.1f, 0.4f, 1.0f);

    // Main camera init.
    state->main_camera = camera_make(VEC2F_ORIGIN, 48);

    // Shader loading.
    Shader quad_shader = shader_load("res/shader/quad.glsl");
    shader_init_uniforms(&quad_shader);
    quad_shader.vertex_stride = 11;
    hash_table_put(&state->shader_table, quad_shader, "quad", 4);
    
    Shader grid_shader = shader_load("res/shader/grid.glsl");
    shader_init_uniforms(&grid_shader);
    grid_shader.vertex_stride = 11;
    hash_table_put(&state->shader_table, grid_shader, "grid", 4);

    Shader line_shader = shader_load("res/shader/line.glsl");
    shader_init_uniforms(&line_shader);
    line_shader.vertex_stride = 7;
    hash_table_put(&state->shader_table, line_shader, "line", 4);

    // Drawer init.
    drawer_init(&state->drawer, hash_table_get(&state->shader_table, "quad", 4));

    // Drawer init.
    line_drawer_init(&state->line_drawer, hash_table_get(&state->shader_table, "line", 4));


    // Font loading.
    u8* font_data = read_file_into_buffer("res/font/font.ttf", NULL, &std_allocator);



    Font_Baked font_baked_medium = font_bake(font_data, 20.0f);
    hash_table_put(&state->font_table, font_baked_medium, "medium", 6);

    Font_Baked font_baked_small = font_bake(font_data, 16.0f);
    hash_table_put(&state->font_table, font_baked_small, "small", 5);

    free(font_data);

    // Player loading.
    state->player.speed = 10.0f;

    global_state = state;
}



/**
 * Interface for functions.
 */
void update_player(Player *p);

void draw_player(Player *p);

Vec2f test_rot = vec2f_make(2.0f, 0.0f);
float angle = 90.0f;


bool dot_collision(float *dotprod) {
    float min = dotprod[0], max = dotprod[0];
    
    for (u32 i = 1; i < 4; i++) {
        if (dotprod[i] > max)
            max = dotprod[i];

        if (dotprod[i] < min)
            min = dotprod[i];
    }

    for (u32 i = 0; i < 4; i++) {
        if (value_inside_domain(min, max, dotprod[i + 4]))
            return true;
    }
    return false;
}

/**
 * @Important: In game update loops the order of procedures is: Updating -> Drawing.
 * Where in updating all logic of the game loop is contained including inputs, sound and so on.
 * Drawing part is only responsible for putting pixels accrodingly with calculated data in "Updating" part.
 */
void plug_update(Plug_State *state) {


    /**
     * -----------------------------------
     *  Updating
     * -----------------------------------
     */

    // Random pop up.
    if (is_pressed_keycode(SDLK_e)) {
        for (s32 i = 0; i < 4; i++) {
            pop_up_log("Pop up: i = %d\n", i);
        }
    }
    
    // Time slow down input.
    if (is_pressed_keycode(SDLK_t)) {
        time_slow_factor++;
        state->t->delta_time_multi = 1.0f / (time_slow_factor % 5);
        
        pop_up_log("Delta Time Multiplier: %f\n", state->t->delta_time_multi);
    }

    // Player logic.
    update_player(&state->player);
    
    // Lerp camera.
    state->main_camera.center = vec2f_lerp(state->main_camera.center, state->player.body.center_mass, 0.025f);



    // Test rotation.
    angle += PI / 4 * state->t->delta_time;
    test_rot = vec2f_rotate(test_rot, PI/2 * state->t->delta_time);

    // Test obb.
    OBB box1 = obb_make(vec2f_make(4.0f, 3.0f), 2.0f, 1.5f, angle);
    OBB box2 = obb_make(vec2f_make(2.2f, 2.2f), 1.2f, 1.6f, PI / 6 + angle);

    OBB *obb1 = &box1;
    OBB *obb2 = &box2;

    Vec2f normals[4] = {
        obb_right(obb1),
        obb_up(obb1),
        obb_right(obb2),
        obb_up(obb2),
    };

    float dotprod[32];

    for (u32 i = 0; i < 4; i++) {
        dotprod[0 + i * 8] = vec2f_dot(normals[i], obb_p0(obb1));
        dotprod[1 + i * 8] = vec2f_dot(normals[i], obb_p1(obb1));
        dotprod[2 + i * 8] = vec2f_dot(normals[i], obb_p2(obb1));
        dotprod[3 + i * 8] = vec2f_dot(normals[i], obb_p3(obb1));
        dotprod[4 + i * 8] = vec2f_dot(normals[i], obb_p0(obb2));
        dotprod[5 + i * 8] = vec2f_dot(normals[i], obb_p1(obb2));
        dotprod[6 + i * 8] = vec2f_dot(normals[i], obb_p2(obb2));
        dotprod[7 + i * 8] = vec2f_dot(normals[i], obb_p3(obb2));
    }


    /**
     * -----------------------------------
     *  Drawing
     * -----------------------------------
     */

    // Get neccessary to draw resource: (Shaders, Fonts, Textures, etc.).
    Shader *quad_shader = hash_table_get(&state->shader_table, "quad", 4);
    Shader *grid_shader = hash_table_get(&state->shader_table, "grid", 4);
    Shader *line_shader = hash_table_get(&state->shader_table, "line", 4);

    Font_Baked *font_medium = hash_table_get(&state->font_table, "medium", 6);
    Font_Baked *font_small  = hash_table_get(&state->font_table, "small", 5);

    
    // Reset viewport.
    viewport_reset(state);


    // Clear the screen with clear_color.
    glClearColor(state->clear_color.x, state->clear_color.y, state->clear_color.z, state->clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);


    // Load camera's projection into all shaders to correctly draw elements according to camera view.
    Matrix4f projection;

    projection = camera_calculate_projection(&state->main_camera, (float)state->window_width, (float)state->window_height);

    shader_update_projection(quad_shader, &projection);
    shader_update_projection(grid_shader, &projection);
    shader_update_projection(line_shader, &projection);


    // Grid quad drawing.
    state->drawer.program = grid_shader;
    draw_begin(&state->drawer);

    Vec2f p0, p1;
    p0 = vec2f_make(-8.0f, -5.0f);
    p1 = vec2f_make(8.0f, 5.0f);
    draw_quad(p0, p1, vec4f_make(0.8f, 0.8f, 0.8f, 0.6f), NULL, p0, p1, NULL, 0.0f);

    draw_end();







    // Regular quad drawing.
    state->drawer.program = quad_shader;
    draw_begin(&state->drawer);

    draw_player(&state->player);

    Vec4f obb_color = VEC4F_GREEN;
    if (phys_sat_collision_obb(obb1, obb2)) {
        obb_color = VEC4F_RED;
    }
    
    draw_quad(obb_p0(obb1), obb_p1(obb1), obb_color, NULL, VEC2F_ORIGIN, VEC2F_UNIT, NULL, obb1->rot);
    draw_quad(obb_p0(obb2), obb_p1(obb2), obb_color, NULL, VEC2F_ORIGIN, VEC2F_UNIT, NULL, obb2->rot);
    

    for (u32 n = 0; n < 4; n++) {
        for (u32 i = 0; i < 8; i++) {
            draw_dot(vec2f_multi_constant(normals[n], dotprod[i + n * 8]), VEC4F_BLUE);
        }
    }



    draw_end();






    // Line drawing.
    line_draw_begin(&state->line_drawer);

    Vec4f color = VEC4F_GREY;
    if (dot_collision(dotprod))
        color = VEC4F_RED;
    draw_function(-12, 12, func_expr((normals[0].y / normals[0].x) * x), 1, color);

    color = VEC4F_GREY;
    if (dot_collision(dotprod + 8))
        color = VEC4F_RED;
    draw_function(-12, 12, func_expr((normals[1].y / normals[1].x) * x), 1, color);

    color = VEC4F_GREY;
    if (dot_collision(dotprod + 16))
        color = VEC4F_RED;
    draw_function(-12, 12, func_expr((normals[2].y / normals[2].x) * x), 1, color);

    color = VEC4F_GREY;
    if (dot_collision(dotprod + 24))
        color = VEC4F_RED;
    draw_function(-12, 12, func_expr((normals[3].y / normals[3].x) * x), 1, color);



    line_draw_end();










    // To screen, (ui) matrix.
    projection = screen_calculate_projection(state->window_width, state->window_height);

    shader_update_projection(quad_shader, &projection);


    state->drawer.program = quad_shader;
    draw_begin(&state->drawer);

    draw_pop_up(vec2f_make(10.0f, 80.0f), VEC4F_WHITE, font_medium, 1);

    draw_end();
    check_gl_error();
}


/**
 * @Todo: Refactor this approach.
 * Remove unnecessary if branching.
 */
void update_player(Player *p) {
    if (p->bound_box.p0.y > 0)
        p->in_air = true;
    else
        p->in_air = false;
    
    float vel_x = 0.0f;

    if (is_hold_keycode(SDLK_d)) {
        vel_x = 1.0f;
    }

    if (is_hold_keycode(SDLK_a)) {
        vel_x = -1.0f;
    }
    
    vel_x *= p->speed;
    p->body.velocity.x = lerp(p->body.velocity.x, vel_x, p->in_air ? 10.0f * air_control_percent * global_state->t->delta_time : 10.0f * global_state->t->delta_time );
 
    if (p->bound_box.p0.y <= jump_input_leeway && is_pressed_keycode(SDLK_SPACE)) {
        p->body.center_mass.y -= p->bound_box.p0.y;
        p->bound_box.p1.y -= p->bound_box.p0.y;
        p->bound_box.p0.y -= p->bound_box.p0.y;
        p->body.velocity.y = 0;
        
        // p->body.velocity = vec2f_sum(p->body.velocity, vec2f_divide_constant(vec2f_make(0.0f, sqrtf(0.75f) * 566.4f), p->body.mass));
        phys_apply_force(&p->body, vec2f_make(0.0f, 500.0f));

        // Update physics.
        phys_apply_impulses();
    }

    // Apply gravity.
    if (p->in_air) {
        // Vi + a * dt = Vf . 
        p->body.velocity = vec2f_sum(p->body.velocity, vec2f_multi_constant(gravity_acceleration, global_state->t->delta_time));
    }


    // Applying calculated velocities. For both AABB and Rigid Body 2D.
    aabb_move(&p->bound_box, vec2f_multi_constant(p->body.velocity, global_state->t->delta_time));
    p->body.center_mass = vec2f_sum(p->body.center_mass, vec2f_multi_constant(p->body.velocity, global_state->t->delta_time));
    
    // Ground collision resolution.
    if (p->bound_box.p0.y < 0) {
        p->body.center_mass.y -= p->bound_box.p0.y;
        p->bound_box.p1.y -= p->bound_box.p0.y;
        p->bound_box.p0.y -= p->bound_box.p0.y;
        p->body.velocity.y = 0;
        p->in_air = false;
    }
    else {
        p->in_air = true;
    }


}

void draw_player(Player *p) {
    // Drawing player as a quad.
   draw_quad(p->bound_box.p0, p->bound_box.p1, VEC4F_YELLOW, NULL, VEC2F_ORIGIN, VEC2F_UNIT, NULL, 0.0f);
}





// Plug unload.
void plug_unload(Plug_State *state) {
    shader_unload(hash_table_get(&state->shader_table, "quad", 4));
    shader_unload(hash_table_get(&state->shader_table, "grid", 4));
    shader_unload(hash_table_get(&state->shader_table, "line", 4));
    
    drawer_free(&state->drawer);
    line_drawer_free(&state->line_drawer);
    
    font_free(hash_table_get(&state->font_table, "medium", 6));
    font_free(hash_table_get(&state->font_table, "small", 5));
}


/**
 * Helper drawing functions.
 */
void draw_quad(Vec2f p0, Vec2f p1, Vec4f color, Texture *texture, Vec2f uv0, Vec2f uv1, Texture *mask, float offset_angle) {
    float texture_slot = -1.0f; // @Important: -1.0f slot signifies shader to use color, not texture.
    float mask_slot = -1.0f;

    if (texture != NULL)
        texture_slot = add_texture_to_slots(texture);              

    if (mask != NULL)
        mask_slot = add_texture_to_slots(mask);              
    
    Vec2f k = vec2f_make(cosf(offset_angle), sinf(offset_angle));
    k = vec2f_multi_constant(k, vec2f_dot(k, vec2f_difference(p1, p0)));
    
    Vec2f p2 = vec2f_sum(p0, k);
    Vec2f p3 = vec2f_difference(p1, k);
    
    float quad_data[44] = {
        p0.x, p0.y, 0.0f, color.x, color.y, color.z, color.w, uv0.x, uv0.y, texture_slot, mask_slot,
        p2.x, p2.y, 0.0f, color.x, color.y, color.z, color.w, uv1.x, uv0.y, texture_slot, mask_slot,
        p3.x, p3.y, 0.0f, color.x, color.y, color.z, color.w, uv0.x, uv1.y, texture_slot, mask_slot,
        p1.x, p1.y, 0.0f, color.x, color.y, color.z, color.w, uv1.x, uv1.y, texture_slot, mask_slot,
    };

    draw_quad_data(quad_data, 1);
}

void draw_text(const char *text, Vec2f current_point, Vec4f color, Font_Baked *font, u32 unit_scale) {
    u64 text_length = strlen(text);

    // Scale and adjust current_point.
    current_point = vec2f_multi_constant(current_point, (float)unit_scale);
    float origin_x = current_point.x;
    current_point.y += (float)font->baseline;

    // Text rendering variables.
    s32 font_char_index;
    u16 width, height;
    stbtt_bakedchar *c;

    for (u64 i = 0; i < text_length; i++) {
        // printf("%d\n", font_char_index);

        // @Incomplete: Handle special characters / symbols.
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

            draw_quad(vec2f_divide_constant(vec2f_make(current_point.x + c->xoff, current_point.y - c->yoff - height), (float)unit_scale), vec2f_divide_constant(vec2f_make(current_point.x + c->xoff + width, current_point.y - c->yoff), (float)unit_scale), color, NULL, vec2f_make(c->x0 / (float)font->bitmap.width, c->y1 / (float)font->bitmap.height), vec2f_make(c->x1 / (float)font->bitmap.width, c->y0 / (float)font->bitmap.height), &font->bitmap, 0.0f);

            current_point.x += font->chars[font_char_index].xadvance;
        }
    }
}

void draw_line(Vec2f p0, Vec2f p1, Vec4f color) {
    float line_data[14] = {
        p0.x, p0.y, 0.0f, color.x, color.y, color.z, color.w,
        p1.x, p1.y, 0.0f, color.x, color.y, color.z, color.w,
    };

    draw_line_data(line_data, 1);
}

#define DOT_SCALE 0.0015f

void draw_dot(Vec2f position, Vec4f color) {
    draw_quad(vec2f_make(position.x - (float)global_state->main_camera.unit_scale * DOT_SCALE, position.y), vec2f_make(position.x + (float)global_state->main_camera.unit_scale * DOT_SCALE, position.y), color, NULL, VEC2F_ORIGIN, VEC2F_UNIT, NULL, PI/4);
}

void draw_quad_outline(Vec2f p0, Vec2f p1, Vec4f color, float offset_angle) {
    
    Vec2f k = vec2f_make(cosf(offset_angle), sinf(offset_angle));
    k = vec2f_multi_constant(k, vec2f_dot(k, vec2f_difference(p1, p0)));
    
    Vec2f p2 = vec2f_sum(p0, k);
    Vec2f p3 = vec2f_difference(p1, k);
    
    float line_data[56] = {
        p0.x, p0.y, 0.0f, color.x, color.y, color.z, color.w,
        p2.x, p2.y, 0.0f, color.x, color.y, color.z, color.w,
        p2.x, p2.y, 0.0f, color.x, color.y, color.z, color.w,
        p1.x, p1.y, 0.0f, color.x, color.y, color.z, color.w,
        p1.x, p1.y, 0.0f, color.x, color.y, color.z, color.w,
        p3.x, p3.y, 0.0f, color.x, color.y, color.z, color.w,
        p3.x, p3.y, 0.0f, color.x, color.y, color.z, color.w,
        p0.x, p0.y, 0.0f, color.x, color.y, color.z, color.w,
    };

    draw_line_data(line_data, 4);
}

void draw_circle_outline(Vec2f position, float radius, u32 detail, Vec4f color) {

    float line_data[detail * 14];

    float step = 2*PI / (float)detail;
    for (u32 i = 0; i < detail; i++) {
        line_data[0  + i * 14] = radius * cosf(step * (float)i) + position.x;
        line_data[1  + i * 14] = radius * sinf(step * (float)i) + position.y;
        line_data[2  + i * 14] = 0.0f;
        line_data[3  + i * 14] = color.x;
        line_data[4  + i * 14] = color.y;
        line_data[5  + i * 14] = color.z;
        line_data[6  + i * 14] = color.w;
                     
        line_data[7  + i * 14] = radius * cosf(step * (float)(i + 1)) + position.x;
        line_data[8  + i * 14] = radius * sinf(step * (float)(i + 1)) + position.y;
        line_data[9  + i * 14] = 0.0f;
        line_data[10 + i * 14] = color.x;
        line_data[11 + i * 14] = color.y;
        line_data[12 + i * 14] = color.z;
        line_data[13 + i * 14] = color.w;
    }
    
   
    draw_line_data(line_data, detail);
}

void draw_function(float x0, float x1, Function y, u32 detail, Vec4f color) {
    float line_data[detail * 14];

    float step = (x1 - x0) / (float)detail;
    for (u32 i = 0; i < detail; i++) {
        line_data[0  + i * 14] = x0 + step * (float)i;
        line_data[1  + i * 14] = y(x0 + step * (float)i);
        line_data[2  + i * 14] = 0.0f;
        line_data[3  + i * 14] = color.x;
        line_data[4  + i * 14] = color.y;
        line_data[5  + i * 14] = color.z;
        line_data[6  + i * 14] = color.w;
                     
        line_data[7  + i * 14] = x0 + step * (float)(i + 1);
        line_data[8  + i * 14] = y(x0 + step * (float)(i + 1));
        line_data[9  + i * 14] = 0.0f;
        line_data[10 + i * 14] = color.x;
        line_data[11 + i * 14] = color.y;
        line_data[12 + i * 14] = color.z;
        line_data[13 + i * 14] = color.w;
    }
    
   
    draw_line_data(line_data, detail);
}

void draw_polar(float t0, float t1, Function r, u32 detail, Vec4f color) {
    float line_data[detail * 14];

    float step = (t1 - t0) / (float)detail;
    float radius;
    for (u32 i = 0; i < detail; i++) {
        radius = r(t0 + step * (float)i);
        line_data[0  + i * 14] = radius * cosf(t0 + step * (float)i);
        line_data[1  + i * 14] = radius * sinf(t0 + step * (float)i);
        line_data[2  + i * 14] = 0.0f;
        line_data[3  + i * 14] = color.x;
        line_data[4  + i * 14] = color.y;
        line_data[5  + i * 14] = color.z;
        line_data[6  + i * 14] = color.w;
                     
        radius = r(t0 + step * (float)(i + 1));
        line_data[7  + i * 14] = radius * cosf(t0 + step * (float)(i + 1));
        line_data[8  + i * 14] = radius * sinf(t0 + step * (float)(i + 1));
        line_data[9  + i * 14] = 0.0f;
        line_data[10 + i * 14] = color.x;
        line_data[11 + i * 14] = color.y;
        line_data[12 + i * 14] = color.z;
        line_data[13 + i * 14] = color.w;
    }
    
   
    draw_line_data(line_data, detail);
}

void draw_parametric(float t0, float t1, Function x, Function y, u32 detail, Vec4f color) {
    float line_data[detail * 14];

    float step = (t1 - t0) / (float)detail;
    for (u32 i = 0; i < detail; i++) {
        line_data[0  + i * 14] = x(t0 + step * (float)i);
        line_data[1  + i * 14] = y(t0 + step * (float)i);
        line_data[2  + i * 14] = 0.0f;
        line_data[3  + i * 14] = color.x;
        line_data[4  + i * 14] = color.y;
        line_data[5  + i * 14] = color.z;
        line_data[6  + i * 14] = color.w;

        line_data[7  + i * 14] = x(t0 + step * (float)(i + 1));
        line_data[8  + i * 14] = y(t0 + step * (float)(i + 1));
        line_data[9  + i * 14] = 0.0f;
        line_data[10 + i * 14] = color.x;
        line_data[11 + i * 14] = color.y;
        line_data[12 + i * 14] = color.z;
        line_data[13 + i * 14] = color.w;
    }
    
   
    draw_line_data(line_data, detail);
}

void draw_area_function(float x0, float x1, Function y, u32 rect_count, Vec4f color) {
    float step = (x1 - x0) / (float)rect_count;
    for (u32 i = 0; i < rect_count; i++) {
        draw_quad(vec2f_make(x0 + step * i, 0.0f), vec2f_make(x0 + step * (i + 1), y(x0 + step * (i + 1))), color, NULL, VEC2F_ORIGIN, VEC2F_UNIT, NULL, 0.0f);
    }
}

void draw_area_polar(float t0, float t1, Function r, u32 rect_count, Vec4f color) {
    float quad_data[rect_count * VERTICIES_PER_QUAD * 11];

    float step = (t1 - t0) / (float)rect_count;
    float radius;
    for (u32 i = 0; i < rect_count; i++) {
        quad_data[0  + i * VERTICIES_PER_QUAD * 11] = 0.0f;
        quad_data[1  + i * VERTICIES_PER_QUAD * 11] = 0.0f;
        quad_data[2  + i * VERTICIES_PER_QUAD * 11] = 0.0f;
        quad_data[3  + i * VERTICIES_PER_QUAD * 11] = color.x;
        quad_data[4  + i * VERTICIES_PER_QUAD * 11] = color.y;
        quad_data[5  + i * VERTICIES_PER_QUAD * 11] = color.z;
        quad_data[6  + i * VERTICIES_PER_QUAD * 11] = color.w;
        quad_data[7  + i * VERTICIES_PER_QUAD * 11] = 0.0f;
        quad_data[8  + i * VERTICIES_PER_QUAD * 11] = 0.0f;
        quad_data[9  + i * VERTICIES_PER_QUAD * 11] = -1.0f;
        quad_data[10 + i * VERTICIES_PER_QUAD * 11] = -1.0f;
                     
        quad_data[11 + i * VERTICIES_PER_QUAD * 11] = 0.0f;
        quad_data[12 + i * VERTICIES_PER_QUAD * 11] = 0.0f;
        quad_data[13 + i * VERTICIES_PER_QUAD * 11] = 0.0f;
        quad_data[14 + i * VERTICIES_PER_QUAD * 11] = color.x;
        quad_data[15 + i * VERTICIES_PER_QUAD * 11] = color.y;
        quad_data[16 + i * VERTICIES_PER_QUAD * 11] = color.z;
        quad_data[17 + i * VERTICIES_PER_QUAD * 11] = color.w;
        quad_data[18 + i * VERTICIES_PER_QUAD * 11] = 1.0f;
        quad_data[19 + i * VERTICIES_PER_QUAD * 11] = 0.0f;
        quad_data[20 + i * VERTICIES_PER_QUAD * 11] = -1.0f;
        quad_data[21 + i * VERTICIES_PER_QUAD * 11] = -1.0f;
                     
        radius = r(t0 + step * (float)i);
        quad_data[22 + i * VERTICIES_PER_QUAD * 11] = radius * cosf(t0 + step * (float)i);
        quad_data[23 + i * VERTICIES_PER_QUAD * 11] = radius * sinf(t0 + step * (float)i);
        quad_data[24 + i * VERTICIES_PER_QUAD * 11] = 0.0f;
        quad_data[25 + i * VERTICIES_PER_QUAD * 11] = color.x;
        quad_data[26 + i * VERTICIES_PER_QUAD * 11] = color.y;
        quad_data[27 + i * VERTICIES_PER_QUAD * 11] = color.z;
        quad_data[28 + i * VERTICIES_PER_QUAD * 11] = color.w;
        quad_data[29 + i * VERTICIES_PER_QUAD * 11] = 0.0f;
        quad_data[30 + i * VERTICIES_PER_QUAD * 11] = 1.0f;
        quad_data[31 + i * VERTICIES_PER_QUAD * 11] = -1.0f;
        quad_data[32 + i * VERTICIES_PER_QUAD * 11] = -1.0f;
                     
        radius = r(t0 + step * (float)(i + 1));
        quad_data[33 + i * VERTICIES_PER_QUAD * 11] = radius * cosf(t0 + step * (float)(i + 1));
        quad_data[34 + i * VERTICIES_PER_QUAD * 11] = radius * sinf(t0 + step * (float)(i + 1));
        quad_data[35 + i * VERTICIES_PER_QUAD * 11] = 0.0f;
        quad_data[36 + i * VERTICIES_PER_QUAD * 11] = color.x;
        quad_data[37 + i * VERTICIES_PER_QUAD * 11] = color.y;
        quad_data[38 + i * VERTICIES_PER_QUAD * 11] = color.z;
        quad_data[39 + i * VERTICIES_PER_QUAD * 11] = color.w;
        quad_data[40 + i * VERTICIES_PER_QUAD * 11] = 1.0f;
        quad_data[41 + i * VERTICIES_PER_QUAD * 11] = 1.0f;
        quad_data[42 + i * VERTICIES_PER_QUAD * 11] = -1.0f;
        quad_data[43 + i * VERTICIES_PER_QUAD * 11] = -1.0f;
    }
    
   
    draw_quad_data(quad_data, rect_count);
}

void draw_area_parametric(float t0, float t1, Function x, Function y, u32 rect_count, Vec4f color) {
    float step = (t1 - t0) / (float)rect_count;
    for (u32 i = 0; i < rect_count; i++) {
        draw_quad(vec2f_make(x(t0 + step * i), 0.0f), vec2f_make(x(t0 + step * (i + 1)), y(t0 + step * (i + 1))), color, NULL, VEC2F_ORIGIN, VEC2F_UNIT, NULL, 0.0f);
    }
}


void draw_viewport(u32 x, u32 y, u32 width, u32 height, Vec4f color, Camera *camera) {
    glViewport(x, y, width, height);
    
    Vec2f p1 = vec2f_make((float)width / 2 / (float)camera->unit_scale, (float)height / 2 / (float)camera->unit_scale);
    Vec2f p0 = vec2f_negate(p1);
    
    draw_quad(p0, p1, color, NULL, p0, p1, NULL, 0.0f);

}

void viewport_reset(Plug_State *state) {
    glViewport(0, 0, state->window_width, state->window_height);
}









