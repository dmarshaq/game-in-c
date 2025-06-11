#include "plug.h"
#include "SDL2/SDL_keycode.h"
#include "core.h"
#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>











/**
 * Helper functions interface.
 */

// Drawing basics.
void draw_quad(Vec2f p0, Vec2f p1, Vec4f color, Texture *texture, Vec2f uv0, Vec2f uv1, Texture *mask, float offset_angle, Vertex_Buffer *buffer);
void draw_grid(Vec2f p0, Vec2f p1, Vec4f color, Vertex_Buffer *buffer);
void draw_text(const char *text, Vec2f position, Vec4f color, Font_Baked *font, u32 unit_scale, Vertex_Buffer *buffer);
Vec2f text_size(const char *text, Font_Baked *font);
void draw_line(Vec2f p0, Vec2f p1, Vec4f color, Vertex_Buffer *buffer);
void draw_dot(Vec2f position, Vec4f color, Vertex_Buffer *buffer);

// Drawing frames.
void draw_quad_outline(Vec2f p0, Vec2f p1, Vec4f color, float offset_angle, Vertex_Buffer *buffer);
void draw_circle_outline(Vec2f position, float radius, u32 detail, Vec4f color, Vertex_Buffer *buffer);

// Drawing function.
void draw_function(float x0, float x1, Function y, u32 detail, Vec4f color, Vertex_Buffer *buffer);
void draw_polar(float t0, float t1, Function r, u32 detail, Vec4f color, Vertex_Buffer *buffer);
void draw_parametric(float t0, float t1, Function x, Function y, u32 detail, Vec4f color, Vertex_Buffer *buffer);

// Drawing area under the curve.
void draw_area_function(float x0, float x1, Function y, u32 rect_count, Vec4f color, Vertex_Buffer *buffer);
void draw_area_polar(float t0, float t1, Function r, u32 rect_count, Vec4f color, Vertex_Buffer *buffer);
void draw_area_parametric(float t0, float t1, Function x, Function y, u32 rect_count, Vec4f color, Vertex_Buffer *buffer);

// Viewport manipulation.
void draw_viewport(u32 x, u32 y, u32 width, u32 height, Vec4f color, Camera *camera, Vertex_Buffer *buffer);
void viewport_reset();

// 2D screen point translation to 2D world.
Vec2f camera_screen_to_world(Vec2f point, Camera *camera);












/**
 * Constants.
 */
static const Vec2f GRAVITY_ACCELERATION = (Vec2f){ 0.0f, -9.81f };
static const u8 MAX_BOXES = 32;
static const u8 MAX_IMPULSES = 16;
static const u8 MAX_PHYS_BOXES = 16;
static const u8 PHYS_ITERATIONS = 16;
static const float PHYS_ITERATION_STEP_TIME = (1.0f / PHYS_ITERATIONS);
static const float MAX_PLAYER_SPEED = 6.0f;
static const float DOT_SCALE = 0.005f;



// Global state.
static Plug_State *state;






/**
 * User Interface.
 */

void ui_init() {
    state->ui.cursor = vec2f_make(20, 20);
    state->ui.origin = vec2f_make(20, 20);
    state->ui.gap = 8;

    state->ui.element_size = VEC2F_ORIGIN;
    state->ui.sameline = false;

    state->ui.theme = (UI_Theme) {
        .bg             = (Vec4f){ 0.12f, 0.12f, 0.14f, 1.0f },   // Dark slate background
        .light          = (Vec4f){ 0.85f, 0.85f, 0.88f, 1.0f },   // Soft light gray
        .btn_bg         = (Vec4f){ 0.12f, 0.16f, 0.28f, 1.0f },   // Deeper, richer base button
        .btn_bg_hover   = (Vec4f){ 0.20f, 0.26f, 0.40f, 1.0f },   // Gentle contrast on hover
        .btn_bg_press   = (Vec4f){ 0.08f, 0.10f, 0.22f, 1.0f },   // Subtle shadowy press state
        .text           = (Vec4f){ 0.97f, 0.96f, 0.92f, 1.0f },   // Warm white text
    };
}

bool ui_is_hover(Vec2f size) {
    return value_inside_domain(state->ui.cursor.x, state->ui.cursor.x + size.x, state->mouse_input.position.x) && value_inside_domain(state->ui.cursor.y, state->ui.cursor.y + size.y, state->mouse_input.position.y);
}

void ui_cursor_reset() {
    state->ui.cursor = state->ui.origin;
    state->ui.element_size = VEC2F_ORIGIN;
    state->ui.sameline = false;
}

void ui_cursor_set(Vec2f pos) {
    state->ui.cursor = pos;
    state->ui.element_size = VEC2F_ORIGIN;
    state->ui.sameline = false;
}

void ui_cursor_advance(Vec2f size) {
    if (state->ui.sameline) {
        state->ui.cursor.x += state->ui.element_size.x + state->ui.gap;
        state->ui.sameline = false;
    } else {
        state->ui.cursor.x = state->ui.origin.x;
        state->ui.cursor.y += state->ui.element_size.y + state->ui.gap;
    }
    state->ui.element_size = size;
}

void ui_sameline() {
    state->ui.sameline = true;
}

void ui_draw_text_centered(Vec2f size, const char *text) {
    if (text == NULL)
        return;

    Vec2f t_size = text_size(text, state->ui.font);

    // Following draw_text centered calculations are for the system where y axis points up, and x axis to the right.
    draw_text(text, vec2f_make(state->ui.cursor.x + (size.x - t_size.x) * 0.5f, state->ui.cursor.y + (size.y + t_size.y) * 0.5f), state->ui.theme.text, state->ui.font, 1.0f, NULL);
}

void ui_draw_box(Vec2f size, Vec4f color) {
    Vec2f p0 = state->ui.cursor;
    Vec2f p1 = vec2f_sum(state->ui.cursor, size);
    float aspect = size.x / size.y;

    float quad_data[44] = {
        p0.x, p0.y, 0.0f, color.x, color.y, color.z, color.w, 0.0f, 0.0f, aspect, -1.0f,
        p1.x, p0.y, 0.0f, color.x, color.y, color.z, color.w, 1.0f, 0.0f, aspect, -1.0f,
        p0.x, p1.y, 0.0f, color.x, color.y, color.z, color.w, 0.0f, 1.0f, aspect, -1.0f,
        p1.x, p1.y, 0.0f, color.x, color.y, color.z, color.w, 1.0f, 1.0f, aspect, -1.0f,
    };
    
    draw_quad_data(quad_data, 1);
}


bool ui_button(Vec2f size, const char *text) {
    ui_cursor_advance(size);

    if (ui_is_hover(size)) {
        if (state->mouse_input.left_hold) {
            ui_draw_box(size, state->ui.theme.btn_bg_press);
        } else {
            ui_draw_box(size, state->ui.theme.btn_bg_hover);
        }
        
        ui_draw_text_centered(size, text);
        return state->mouse_input.left_unpressed;
    }

    ui_draw_box(size, state->ui.theme.btn_bg);

    ui_draw_text_centered(size, text);
    return false;
}

void ui_text(const char *text) {
    Vec2f t_size = text_size(text, state->ui.font);
    ui_cursor_advance(t_size);
    draw_text(text, vec2f_make(state->ui.cursor.x, state->ui.cursor.y + t_size.y), state->ui.theme.text, state->ui.font, 1.0f, NULL);
}

void ui_frame(Vec2f size) {
    ui_cursor_advance(size);
    ui_draw_box(size, state->ui.theme.bg);
}
















/**
 * Pop up message.
 */

// static char pop_up_buffer[256];
// static u32 pop_up_buffer_start = 0;
// static float pop_up_lerp_t = 0.0f;
// 
// 
// void pop_up_log(char *format, ...) {
//     va_list args;
//     va_start(args, format);
//     s32 bytes_written = vsnprintf(pop_up_buffer + pop_up_buffer_start, sizeof(pop_up_buffer) - pop_up_buffer_start, format, args);
//     va_end(args);
//     pop_up_lerp_t = 0.0f;
// 
//     if (bytes_written > 0)
//         pop_up_buffer_start += bytes_written;
// }
// 
// 
// /**
//  * Draws pop_up text that is stored in "pop_up_mesg", doesn't draw anything if it is NULL.
//  * Use "pop_up_log(char *format, ...)" to log message;
//  */
// void draw_pop_up(Vec2f position, Vec4f color, Font_Baked *font, u32 unit_scale) {
//     if (pop_up_buffer[0] == '\0')
//         return;
//     
//     color.w = lerp(color.w, 0.0f, pop_up_lerp_t);
//     position.y = lerp(position.y, position.y + 25.0f, pop_up_lerp_t);
// 
//     draw_text(pop_up_buffer, position, color, font, unit_scale, NULL);
// 
//     pop_up_lerp_t += 0.002f;
// 
//     if (pop_up_lerp_t >= 1.0f) {
//         pop_up_buffer[0] = '\0';
//         pop_up_lerp_t = 0.0f;
//     }
// 
//     pop_up_buffer_start = 0;
// }
























/**
 * Physics.
 */

/**
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
void phys_apply_force(Body_2D *body, Vec2f force) {
    body->velocity = vec2f_sum(body->velocity, vec2f_multi_constant(force, body->inv_mass));
}

/**
 * Applies instanteneous acceleration to rigid body.
 */
void phys_apply_acceleration(Body_2D *body, Vec2f acceleration) {
    body->velocity = vec2f_sum(body->velocity, acceleration);
}

void phys_apply_angular_acceleration(Body_2D *body, float acceleration) {
    body->angular_velocity += acceleration;
}

void phys_init() {
    // Setting physics variables.
    state->impulses = array_list_make(Impulse, MAX_IMPULSES, &std_allocator); // @Leak.
    state->phys_boxes = array_list_make(Phys_Box *, MAX_PHYS_BOXES, &std_allocator); // @Leak.

}

void phys_add_impulse(Vec2f force, u32 milliseconds, Body_2D *body) {
    Impulse impulse = (Impulse) { 
        .delta_force = vec2f_divide_constant(force, (float)milliseconds), 
        .milliseconds = milliseconds, 
        .target = body 
    };
    array_list_append(&state->impulses, impulse);
}

void phys_apply_impulses() {
    for (u32 i = 0; i < array_list_length(&state->impulses); i++) {
        u32 force_time_multi = state->t->delta_time_milliseconds;

        if (state->impulses[i].milliseconds < state->t->delta_time_milliseconds) {
            force_time_multi = state->impulses[i].milliseconds;
            state->impulses[i].milliseconds = 0;
        }
        else {
            state->impulses[i].milliseconds -= state->t->delta_time_milliseconds;
        }

        state->impulses[i].target->velocity = vec2f_sum(state->impulses[i].target->velocity, vec2f_multi_constant(vec2f_divide_constant(state->impulses[i].delta_force, state->impulses[i].target->mass), (float)force_time_multi));

        if (state->impulses[i].milliseconds == 0) {
            array_list_unordered_remove(&state->impulses, i);
            i--;
        }
    }
}

/**
 * Internal function.
 * @Important: "axis1" should always correspond to the axis alligned with "obb1".
 * Returns min separation between "obb1" and "obb2" projected points on "axis1".
 */
float phys_sat_min_depth_on_normal(OBB *obb1, Vec2f axis1, OBB *obb2) {
    // Getting minA and maxA out of 2 points.
    float minA = vec2f_dot(axis1, obb_p0(obb1));
    float maxA = vec2f_dot(axis1, obb_p1(obb1));

    if (minA > maxA) {
        minA = maxA;
        maxA = vec2f_dot(axis1, obb_p0(obb1));
    }

    // Getting minB and maxB out of 4 points. @Optimization: Maybe find a way to calculate dot products only once and reuse them to find min, max.
    float minB = fminf(fminf(vec2f_dot(axis1, obb_p0(obb2)), vec2f_dot(axis1, obb_p1(obb2))), fminf(vec2f_dot(axis1, obb_p2(obb2)), vec2f_dot(axis1, obb_p3(obb2))));
    float maxB = fmaxf(fmaxf(vec2f_dot(axis1, obb_p0(obb2)), vec2f_dot(axis1, obb_p1(obb2))), fmaxf(vec2f_dot(axis1, obb_p2(obb2)), vec2f_dot(axis1, obb_p3(obb2))));
    
    float dist1 = maxB - minA; 
    float dist2 = maxA - minB; 

    return fabsf(fminf(dist1, dist2)) * sig(dist1 * dist2);
}

/**
 * Returns true of "obb1" and "obb2" touch.
 * Usefull for triggers.
 */
bool phys_sat_check_collision_obb(OBB *obb1, OBB *obb2) {
    return phys_sat_min_depth_on_normal(obb1, obb_right(obb1), obb2) > 0.0f &&
        phys_sat_min_depth_on_normal(obb1, obb_up(obb1), obb2) > 0.0f &&
        phys_sat_min_depth_on_normal(obb2, obb_right(obb2), obb1) > 0.0f &&
        phys_sat_min_depth_on_normal(obb2, obb_up(obb2), obb1) > 0.0f;
}

void phys_sat_find_min_depth_normal(OBB *obb1, OBB *obb2, float *depth, Vec2f *normal) {
    Vec2f normals[4] = {
        obb_right(obb1),
        obb_up(obb1),
        obb_right(obb2),
        obb_up(obb2)
    };
    float depths[4] = {
        phys_sat_min_depth_on_normal(obb1, normals[0], obb2),
        phys_sat_min_depth_on_normal(obb1, normals[1], obb2),
        phys_sat_min_depth_on_normal(obb2, normals[2], obb1),
        phys_sat_min_depth_on_normal(obb2, normals[3], obb1)
    };

    for (u32 i = 1; i < 4; i++) {
        if (depths[i] < depths[0]) {
            depths[0] = depths[i];
            normals[0] = normals[i];
        }
    }


    // Flip normal if it's not looking in direction of collosion.
    if (vec2f_dot(normals[0], vec2f_normalize(vec2f_difference(obb2->center, obb1->center))) < 0.0f) {
        normals[0] = vec2f_negate(normals[0]);
    }

    *depth = depths[0];
    *normal = normals[0];
}

/**
 * Sets both obbs apart based on depth and normal.
 * Usefull for two colliding dynamic objects.
 */
void phys_resolve_dynamic_obb_collision(OBB *obb1, OBB *obb2, float depth, Vec2f normal) {

    Vec2f displacement = vec2f_multi_constant(normal, depth / 2);

    obb1->center = vec2f_sum(obb1->center, vec2f_negate(displacement));
    obb2->center = vec2f_sum(obb2->center, displacement);
}

void phys_resolve_dynamic_body_collision_basic(Body_2D *body1, Body_2D *body2, Vec2f normal) {
    Vec2f relative_velocity = vec2f_difference(body2->velocity, body1->velocity);

    float e = fminf(body1->restitution, body2->restitution);

    float j = -(1.0f + e) * vec2f_dot(relative_velocity, normal);
    j /= (1.0f * body1->inv_mass + 1.0f * body2->inv_mass);

    phys_apply_force(body1, vec2f_multi_constant(normal, -j));
    phys_apply_force(body2, vec2f_multi_constant(normal, j));
}

void phys_resolve_dynamic_body_collision(Body_2D *body1, Body_2D *body2, Vec2f normal, Vec2f *contacts, u32 contacts_count) {
    // Calculated variables, used in both loops.
    Vec2f impulses[2] = { VEC2F_ORIGIN, VEC2F_ORIGIN };
    Vec2f r1_array[2] = { VEC2F_ORIGIN, VEC2F_ORIGIN };
    Vec2f r2_array[2] = { VEC2F_ORIGIN, VEC2F_ORIGIN };
    
    // Calculation variables.
    float e = fminf(body1->restitution, body2->restitution);

    Vec2f angular_lin_velocity1, angular_lin_velocity2, relative_velocity;
    float r1_perp_dot_n, r2_perp_dot_n, contact_velocity_mag, j;

    // Calculating impulses and vectors to contact points, for each contact point.
    for (u32 i = 0; i < contacts_count; i++) {
        r1_array[i] = vec2f_difference(contacts[i], body1->mass_center);
        r2_array[i] = vec2f_difference(contacts[i], body2->mass_center);

        angular_lin_velocity1 = vec2f_multi_constant(vec2f_make(-r1_array[i].y, r1_array[i].x), body1->angular_velocity);
        angular_lin_velocity2 = vec2f_multi_constant(vec2f_make(-r2_array[i].y, r2_array[i].x), body2->angular_velocity);

        relative_velocity = vec2f_difference(vec2f_sum(body2->velocity, angular_lin_velocity2), vec2f_sum(body1->velocity, angular_lin_velocity1));
        
        contact_velocity_mag = vec2f_dot(relative_velocity, normal);

        if (contact_velocity_mag > 0.0f) {
            continue;
        }

        r1_perp_dot_n = vec2f_cross(r1_array[i], normal);
        r2_perp_dot_n = vec2f_cross(r2_array[i], normal);

        j = -(1.0f + e) * contact_velocity_mag;
        j /= body1->inv_mass + body2->inv_mass + (r1_perp_dot_n * r1_perp_dot_n) * body1->inv_inertia + (r2_perp_dot_n * r2_perp_dot_n) * body2->inv_inertia;
        j /= (float)contacts_count;

        impulses[i] = vec2f_multi_constant(normal, j);
    }

    // For each contact point applying impulse and angular acceleration.
    for (u32 i = 0; i < contacts_count; i++) {
        phys_apply_force(body1, vec2f_negate(impulses[i]));
        phys_apply_angular_acceleration(body1, -vec2f_cross(r1_array[i], impulses[i]) * body1->inv_inertia);

        phys_apply_force(body2, impulses[i]);
        phys_apply_angular_acceleration(body2, vec2f_cross(r2_array[i], impulses[i]) * body2->inv_inertia);
    }
}

typedef struct phys_collision_calc_vars {
    // Calculated variables, used in both loops.
    Vec2f impulse;
    Vec2f r1;
    Vec2f r2;
    
    // Calculation variables.
    float e;
    float sf;
    float df;

    Vec2f angular_lin_velocity1;
    Vec2f angular_lin_velocity2;
    Vec2f relative_velocity;
    float r1_perp_dot_n; 
    float r2_perp_dot_n;
    float contact_velocity_mag;
    float j[2];
    float jt;

    // Result variables.
    Vec2f res_velocity1;
    Vec2f res_velocity2;
    float res_angular_acceleration1;
    float res_angular_acceleration2;

} Phys_Collision_Calc_Vars;

static Phys_Collision_Calc_Vars p_calc_vars;

void phys_resolve_phys_box_collision_with_rotation_friction(Phys_Box *box1, Phys_Box *box2, Vec2f normal, Vec2f *contacts, u32 contacts_count) {
    // Setting up variables that can be calculated without contact points.
    p_calc_vars.e  = fminf(box1->body.restitution, box2->body.restitution);
    p_calc_vars.sf = (box1->body.static_friction + box2->body.static_friction) / 2;
    p_calc_vars.df = (box1->body.dynamic_friction + box2->body.dynamic_friction) / 2;

    // Init resulat vars to default values.
    p_calc_vars.res_velocity1 = VEC2F_ORIGIN;
    p_calc_vars.res_velocity2 = VEC2F_ORIGIN;
    p_calc_vars.res_angular_acceleration1 = 0.0f;
    p_calc_vars.res_angular_acceleration2 = 0.0f;


    p_calc_vars.r1 = VEC2F_ORIGIN;
    p_calc_vars.r2 = VEC2F_ORIGIN;
    p_calc_vars.impulse = VEC2F_ORIGIN;

    p_calc_vars.j[0] = 0.0f;
    p_calc_vars.j[1] = 0.0f;

    // Calculating impulses and vectors to contact points, for each contact point.
    for (u32 i = 0; i < contacts_count; i++) {
        p_calc_vars.r1 = vec2f_difference(contacts[i], box1->body.mass_center);
        p_calc_vars.r2 = vec2f_difference(contacts[i], box2->body.mass_center);

        p_calc_vars.angular_lin_velocity1 = vec2f_multi_constant(vec2f_make(-p_calc_vars.r1.y, p_calc_vars.r1.x), box1->body.angular_velocity);
        p_calc_vars.angular_lin_velocity2 = vec2f_multi_constant(vec2f_make(-p_calc_vars.r2.y, p_calc_vars.r2.x), box2->body.angular_velocity);

        p_calc_vars.relative_velocity = vec2f_difference(vec2f_sum(box2->body.velocity, p_calc_vars.angular_lin_velocity2), vec2f_sum(box1->body.velocity, p_calc_vars.angular_lin_velocity1));

        p_calc_vars.contact_velocity_mag = vec2f_dot(p_calc_vars.relative_velocity, normal);


        if (p_calc_vars.contact_velocity_mag > 0.0f) {
            continue;
        }

        p_calc_vars.r1_perp_dot_n = vec2f_cross(p_calc_vars.r1, normal);
        p_calc_vars.r2_perp_dot_n = vec2f_cross(p_calc_vars.r2, normal);

        p_calc_vars.j[i] = -(1.0f + p_calc_vars.e) * p_calc_vars.contact_velocity_mag;
        p_calc_vars.j[i] /= box1->body.inv_mass + box2->body.inv_mass + (p_calc_vars.r1_perp_dot_n * p_calc_vars.r1_perp_dot_n) * box1->body.inv_inertia + (p_calc_vars.r2_perp_dot_n * p_calc_vars.r2_perp_dot_n) * box2->body.inv_inertia;
        p_calc_vars.j[i] /= (float)contacts_count;



        p_calc_vars.impulse = vec2f_multi_constant(normal, p_calc_vars.j[i]);

        p_calc_vars.res_velocity1 = vec2f_sum(p_calc_vars.res_velocity1, vec2f_multi_constant(vec2f_negate(p_calc_vars.impulse), box1->body.inv_mass));
        p_calc_vars.res_angular_acceleration1 += -vec2f_cross(p_calc_vars.r1, p_calc_vars.impulse) * box1->body.inv_inertia;

        p_calc_vars.res_velocity2 = vec2f_sum(p_calc_vars.res_velocity2, vec2f_multi_constant(p_calc_vars.impulse, box2->body.inv_mass));
        p_calc_vars.res_angular_acceleration2 += vec2f_cross(p_calc_vars.r2, p_calc_vars.impulse) * box2->body.inv_inertia;
    }

    box1->body.velocity = vec2f_sum(box1->body.velocity, p_calc_vars.res_velocity1);
    if (box1->rotatable)
        box1->body.angular_velocity += p_calc_vars.res_angular_acceleration1;

    box2->body.velocity = vec2f_sum(box2->body.velocity, p_calc_vars.res_velocity2);
    if (box2->rotatable)
        box2->body.angular_velocity += p_calc_vars.res_angular_acceleration2;



    p_calc_vars.res_velocity1 = VEC2F_ORIGIN;
    p_calc_vars.res_velocity2 = VEC2F_ORIGIN;
    p_calc_vars.res_angular_acceleration1 = 0.0f;
    p_calc_vars.res_angular_acceleration2 = 0.0f;


    p_calc_vars.r1 = VEC2F_ORIGIN;
    p_calc_vars.r2 = VEC2F_ORIGIN;
    p_calc_vars.impulse = VEC2F_ORIGIN;


    // Friction.
    for (u32 i = 0; i < contacts_count; i++) {
        p_calc_vars.r1 = vec2f_difference(contacts[i], box1->body.mass_center);
        p_calc_vars.r2 = vec2f_difference(contacts[i], box2->body.mass_center);

        p_calc_vars.angular_lin_velocity1 = vec2f_multi_constant(vec2f_make(-p_calc_vars.r1.y, p_calc_vars.r1.x), box1->body.angular_velocity);
        p_calc_vars.angular_lin_velocity2 = vec2f_multi_constant(vec2f_make(-p_calc_vars.r2.y, p_calc_vars.r2.x), box2->body.angular_velocity);

        p_calc_vars.relative_velocity = vec2f_difference(vec2f_sum(box2->body.velocity, p_calc_vars.angular_lin_velocity2), vec2f_sum(box1->body.velocity, p_calc_vars.angular_lin_velocity1));

        Vec2f tanget = vec2f_difference(p_calc_vars.relative_velocity, vec2f_multi_constant(normal, vec2f_dot(p_calc_vars.relative_velocity, normal)));

        
        if (fequal(tanget.x, 0.0f) && fequal(tanget.y, 0.0f)) {
            continue;
        }

        tanget = vec2f_normalize(tanget);

        p_calc_vars.r1_perp_dot_n = vec2f_cross(p_calc_vars.r1, tanget);
        p_calc_vars.r2_perp_dot_n = vec2f_cross(p_calc_vars.r2, tanget);

        p_calc_vars.jt = -vec2f_dot(p_calc_vars.relative_velocity, tanget);
        p_calc_vars.jt /= box1->body.inv_mass + box2->body.inv_mass + (p_calc_vars.r1_perp_dot_n * p_calc_vars.r1_perp_dot_n) * box1->body.inv_inertia + (p_calc_vars.r2_perp_dot_n * p_calc_vars.r2_perp_dot_n) * box2->body.inv_inertia;
        p_calc_vars.jt /= (float)contacts_count;

        // Collumbs law.
        if (fabsf(p_calc_vars.jt) <= p_calc_vars.j[i] * p_calc_vars.sf) {
            // friction_impulses[i] = vec2f_multi_constant(tanget, p_calc_vars.jt);
            p_calc_vars.impulse = vec2f_multi_constant(tanget, p_calc_vars.jt);
        }
        else {
            // friction_impulses[i] = vec2f_multi_constant(tanget, -j_array[i] * p_calc_vars.df);
            p_calc_vars.impulse = vec2f_multi_constant(tanget, -p_calc_vars.j[i] * p_calc_vars.df);
        }

        p_calc_vars.res_velocity1 = vec2f_sum(p_calc_vars.res_velocity1, vec2f_multi_constant(vec2f_negate(p_calc_vars.impulse), box1->body.inv_mass));
        p_calc_vars.res_angular_acceleration1 += -vec2f_cross(p_calc_vars.r1, p_calc_vars.impulse) * box1->body.inv_inertia;

        p_calc_vars.res_velocity2 = vec2f_sum(p_calc_vars.res_velocity2, vec2f_multi_constant(p_calc_vars.impulse, box2->body.inv_mass));
        p_calc_vars.res_angular_acceleration2 += vec2f_cross(p_calc_vars.r2, p_calc_vars.impulse) * box2->body.inv_inertia;
    }

    box1->body.velocity = vec2f_sum(box1->body.velocity, p_calc_vars.res_velocity1);
    if (box1->rotatable)
        box1->body.angular_velocity += p_calc_vars.res_angular_acceleration1;

    box2->body.velocity = vec2f_sum(box2->body.velocity, p_calc_vars.res_velocity2);
    if (box2->rotatable)
        box2->body.angular_velocity += p_calc_vars.res_angular_acceleration2;
}

void phys_resolve_dynamic_body_collision_with_friction(Body_2D *body1, Body_2D *body2, Vec2f normal, Vec2f *contacts, u32 contacts_count) {
    // Calculated variables, used in both loops.
    Vec2f impulses[2] = { VEC2F_ORIGIN, VEC2F_ORIGIN };
    Vec2f r1_array[2] = { VEC2F_ORIGIN, VEC2F_ORIGIN };
    Vec2f r2_array[2] = { VEC2F_ORIGIN, VEC2F_ORIGIN };
    float j_array[2] = { 0.0f, 0.0f };
    Vec2f friction_impulses[2] = { VEC2F_ORIGIN, VEC2F_ORIGIN };
    
    // Calculation variables.
    float e = fminf(body1->restitution, body2->restitution);
    float sf = (body1->static_friction + body2->static_friction) / 2;
    float df = (body1->dynamic_friction + body2->dynamic_friction) / 2;

    Vec2f angular_lin_velocity1, angular_lin_velocity2, relative_velocity;
    float r1_perp_dot_n, r2_perp_dot_n, contact_velocity_mag, j, jt;

    // Calculating impulses and vectors to contact points, for each contact point.
    for (u32 i = 0; i < contacts_count; i++) {
        r1_array[i] = vec2f_difference(contacts[i], body1->mass_center);
        r2_array[i] = vec2f_difference(contacts[i], body2->mass_center);

        angular_lin_velocity1 = vec2f_multi_constant(vec2f_make(-r1_array[i].y, r1_array[i].x), body1->angular_velocity);
        angular_lin_velocity2 = vec2f_multi_constant(vec2f_make(-r2_array[i].y, r2_array[i].x), body2->angular_velocity);

        relative_velocity = vec2f_difference(vec2f_sum(body2->velocity, angular_lin_velocity2), vec2f_sum(body1->velocity, angular_lin_velocity1));

        contact_velocity_mag = vec2f_dot(relative_velocity, normal);


        if (contact_velocity_mag > 0.0f) {
            continue;
        }

        r1_perp_dot_n = vec2f_cross(r1_array[i], normal);
        r2_perp_dot_n = vec2f_cross(r2_array[i], normal);

        j = -(1.0f + e) * contact_velocity_mag;
        j /= body1->inv_mass + body2->inv_mass + (r1_perp_dot_n * r1_perp_dot_n) * body1->inv_inertia + (r2_perp_dot_n * r2_perp_dot_n) * body2->inv_inertia;
        j /= (float)contacts_count;


        j_array[i] = j;

        impulses[i] = vec2f_multi_constant(normal, j);
    }

    // For each contact point applying impulse and angular acceleration.
    for (u32 i = 0; i < contacts_count; i++) {
        phys_apply_force(body1, vec2f_negate(impulses[i]));
        phys_apply_angular_acceleration(body1, -vec2f_cross(r1_array[i], impulses[i]) * body1->inv_inertia);

        phys_apply_force(body2, impulses[i]);
        phys_apply_angular_acceleration(body2, vec2f_cross(r2_array[i], impulses[i]) * body2->inv_inertia);
    }

    // Friction.
    for (u32 i = 0; i < contacts_count; i++) {
        r1_array[i] = vec2f_difference(contacts[i], body1->mass_center);
        r2_array[i] = vec2f_difference(contacts[i], body2->mass_center);

        angular_lin_velocity1 = vec2f_multi_constant(vec2f_make(-r1_array[i].y, r1_array[i].x), body1->angular_velocity);
        angular_lin_velocity2 = vec2f_multi_constant(vec2f_make(-r2_array[i].y, r2_array[i].x), body2->angular_velocity);

        relative_velocity = vec2f_difference(vec2f_sum(body2->velocity, angular_lin_velocity2), vec2f_sum(body1->velocity, angular_lin_velocity1));


        Vec2f tanget = vec2f_difference(relative_velocity, vec2f_multi_constant(normal, vec2f_dot(relative_velocity, normal)));

        
        if (fequal(tanget.x, 0.0f) && fequal(tanget.y, 0.0f)) {
            continue;
        }

        tanget = vec2f_normalize(tanget);

        r1_perp_dot_n = vec2f_cross(r1_array[i], tanget);
        r2_perp_dot_n = vec2f_cross(r2_array[i], tanget);

        jt = -vec2f_dot(relative_velocity, tanget);
        jt /= body1->inv_mass + body2->inv_mass + (r1_perp_dot_n * r1_perp_dot_n) * body1->inv_inertia + (r2_perp_dot_n * r2_perp_dot_n) * body2->inv_inertia;
        jt /= (float)contacts_count;
    

        // Collumbs law.
        if (fabsf(jt) <= j * sf) {
            friction_impulses[i] = vec2f_multi_constant(tanget, jt);
        }
        else {
            friction_impulses[i] = vec2f_multi_constant(tanget, -j_array[i] * df);
        }
    }

    // For each contact point applying friction impulse and angular acceleration.
    for (u32 i = 0; i < contacts_count; i++) {
        phys_apply_force(body1, vec2f_negate(friction_impulses[i]));
        phys_apply_angular_acceleration(body1, -vec2f_cross(r1_array[i], friction_impulses[i]) * body1->inv_inertia);

        phys_apply_force(body2, friction_impulses[i]);
        phys_apply_angular_acceleration(body2, vec2f_cross(r2_array[i], friction_impulses[i]) * body2->inv_inertia);
    }
}

/**
 * Sets dynamic obb apart based on depth and normal.
 * Usefull for colliding dynamic object with immovable or static object.
 */
void phys_resolve_static_obb_collision(OBB *obb, float depth, Vec2f normal) {
    obb->center = vec2f_sum(obb->center, vec2f_multi_constant(normal, depth));
}

/**
 * Finds all contact points, maximum of 2, and stores them in "points" array.
 * @Important: "points" should not be NULL and should be of size two.
 * Returns count of points found.
 */
u32 phys_find_contanct_points_obb(OBB* obb1, OBB* obb2, Vec2f *points) {
    Vec2f verticies[8] = {
        obb_p0(obb1),
        obb_p2(obb1),
        obb_p1(obb1),
        obb_p3(obb1),
        obb_p0(obb2),
        obb_p2(obb2),
        obb_p1(obb2),
        obb_p3(obb2),
    };
    
    Vec2f a, b, p;
    float min_dist1 = FLT_MAX;
    float dist = 0.0f;
    u32 count = 0;

    for (u32 o = 0; o < 2; o++) {
        for (u32 i = 0; i < 4; i++) {
            p = verticies[i + o * 4];
            for (u32 j = 0; j < 4; j++) {
                a = verticies[j + 4 - o * 4];
                b = verticies[((j + 1) % 4) + 4 - o * 4];

                dist = point_segment_min_distance(p, a, b);

                if (fequal(dist, min_dist1)) {
                    if (!(fequal(p.x, p.y) && fequal(points[0].x, points[0].y))) {
                        points[1] = p;
                        count = 2;
                    }
                }
                else if (dist < min_dist1) {
                    min_dist1 = dist;
                    points[0] = p;
                    count = 1;
                }
            }
        }
    }

    return count;
}

void phys_update() {
    u32 length = array_list_length(&state->phys_boxes);

    float depth;
    Vec2f normal;
    Phys_Box *box1;
    Phys_Box *box2;

    for (u32 it = 0; it < PHYS_ITERATIONS; it++) {

        for (u32 i = 0; i < length; i++) {
            box1 = state->phys_boxes[i];

            // @Important: This is a check if a pointer box1 is invalid pointer, it is done by specifically looking at x dimension, since that is part of data that is set to 0.0f when box1 is no longer used / nullified.
            if (fequal(box1->bound_box.dimensions.x, 0.0f)) { 
                array_list_unordered_remove(&state->phys_boxes, i);
                length = array_list_length(&state->phys_boxes);
                i--;
                continue;
            }

            if (!box1->dynamic) {
                continue;
            }

            box1->grounded = false;

            // Applying gravity.
            if (box1->gravitable) {
                box1->body.velocity = vec2f_sum(box1->body.velocity, vec2f_multi_constant(GRAVITY_ACCELERATION, state->t->delta_time * PHYS_ITERATION_STEP_TIME ));
            }

            // Applying velocities.
            box1->bound_box.center = vec2f_sum(box1->bound_box.center, vec2f_multi_constant(box1->body.velocity, state->t->delta_time * PHYS_ITERATION_STEP_TIME));
            box1->bound_box.rot += box1->body.angular_velocity * state->t->delta_time * PHYS_ITERATION_STEP_TIME;
            box1->body.mass_center = box1->bound_box.center;

            // Debug drawing.
            draw_line(box1->bound_box.center, vec2f_sum(box1->bound_box.center, vec2f_multi_constant(box1->body.velocity, state->t->delta_time * PHYS_ITERATION_STEP_TIME)), VEC4F_RED, &state->debug_vert_buffer);

            // Debug drawing.
            draw_quad_outline(obb_p0(&box1->bound_box), obb_p1(&box1->bound_box), VEC4F_RED, box1->bound_box.rot, &state->debug_vert_buffer);

        }

        Vec2f contacts[2];
        u32 contacts_count;
        // Collision.
        for (u32 i = 0; i < length; i++) {
            box1 = state->phys_boxes[i];
            for (u32 j = i + 1; j < length; j++) {
                box2 = state->phys_boxes[j];
                if (phys_sat_check_collision_obb(&box1->bound_box, &box2->bound_box)) { // @Speed: Need separate broad phase.
                    
                    // Fidning depth and normal of collision.
                    phys_sat_find_min_depth_normal(&box1->bound_box, &box2->bound_box, &depth, &normal);

                    // Calculating dot product to check if any objects are grounded.
                    float grounded_dot = vec2f_dot(vec2f_normalize(GRAVITY_ACCELERATION), normal);

                    if (box1->dynamic && !box2->dynamic) {
                        phys_resolve_static_obb_collision(&box1->bound_box, depth, vec2f_negate(normal));

                        if (grounded_dot > 0.7f)
                            box1->grounded = true;
                    }
                    else if (box2->dynamic && !box1->dynamic) {
                        phys_resolve_static_obb_collision(&box2->bound_box, depth, normal);

                        if (grounded_dot < -0.7f)
                            box2->grounded = true;
                    }
                    else {
                        phys_resolve_dynamic_obb_collision(&box1->bound_box, &box2->bound_box, depth, normal);

                        if (grounded_dot > 0.7f)
                            box1->grounded = true;
                        else if (grounded_dot < -0.7f)
                            box2->grounded = true;
                    }
                    box1->body.mass_center = box1->bound_box.center;
                    box2->body.mass_center = box2->bound_box.center;


                    /**
                     * @Todo: Remove physics resolution abstraction, and generalize for different types of phys boxes collisions.
                     */
                    contacts_count = phys_find_contanct_points_obb(&box1->bound_box, &box2->bound_box, contacts);
                    phys_resolve_phys_box_collision_with_rotation_friction(box1, box2, normal, contacts, contacts_count);
                }
            }
        }
    }
}












/**
 * Box functions.
 */

void spawn_box(Vec2f position, Vec4f color) {
    u32 free_index = 0;

    for (; free_index < MAX_BOXES; free_index++) {
        if (state->boxes[free_index].destroyed) {
            break;
        }   
    }

    if (free_index == MAX_BOXES) {
        printf_err("Max boxes limit have been reached, cannot spawn more boxes.\n");
        return;
    }

    float width = 0.6f * randf() + 0.8f;
    float height = 0.6f * randf() + 0.8f;
    

    state->boxes[free_index] = (Box) {
        .p_box = (Phys_Box) {
                .bound_box = obb_make(position, width, height, 0.0f),
                .body = body_obb_make(30.0f, position, width, height, 0.4f, 0.45f, 0.3f),

                .dynamic = true,
                .rotatable = true,
                .destructible = false,
                .gravitable = true,
                .grounded = false,
            },
        .color = color,
        .destroyed = false,
    };

    array_list_append(&state->phys_boxes, &state->boxes[free_index].p_box);
}

void spawn_obstacle(Vec2f c, float w, float h, Vec4f color, float angle) {
    u32 free_index = 0;

    for (; free_index < MAX_BOXES; free_index++) {
        if (state->boxes[free_index].destroyed) {
            break;
        }   
    }

    if (free_index == MAX_BOXES) {
        printf_err("Max boxes limit have been reached, cannot spawn more boxes.\n");
        return;
    }

    state->boxes[free_index] = (Box) {
        .p_box = (Phys_Box) {
                .bound_box = obb_make(c, w, h, angle),
                .body = body_obb_make(0.0f, c, w, h, 0.4f, 0.45f, 0.3f),

                .dynamic = false,
                .destructible = false,
                .rotatable = true,
                .gravitable = false,
                .grounded = false,
            },
        .color = color,
        .destroyed = false,
    };

    array_list_append(&state->phys_boxes, &state->boxes[free_index].p_box);
}

void spawn_rect(Vec2f p0, Vec2f p1, Vec4f color) {
    u32 free_index = 0;

    for (; free_index < MAX_BOXES; free_index++) {
        if (state->boxes[free_index].destroyed) {
            break;
        }   
    }

    if (free_index == MAX_BOXES) {
        printf_err("Max boxes limit have been reached, cannot spawn more boxes.\n");
        return;
    }

    state->boxes[free_index] = (Box) {
        .p_box = (Phys_Box) {
                .bound_box = obb_make(vec2f_sum(p0, vec2f_divide_constant(vec2f_difference(p1, p0), 2)), p1.x - p0.x, p1.y - p0.y, 0.0f),
                .body = body_obb_make(0.0f, vec2f_sum(p0, vec2f_divide_constant(vec2f_difference(p1, p0), 2)), p1.x - p0.x, p1.y - p0.y, 0.4f, 0.45f, 0.3f),

                .dynamic = false,
                .destructible = false,
                .rotatable = true,
                .gravitable = false,
                .grounded = false,
            },
        .color = color,
        .destroyed = false,
    };

    array_list_append(&state->phys_boxes, &state->boxes[free_index].p_box);
}

void update_boxes() {
    for (u32 i = 0; i < MAX_BOXES; i++) {
        if (state->boxes[i].destroyed) {
            continue;
        }
        if (state->boxes[i].p_box.bound_box.center.y < -15.0f) {
            state->boxes[i].p_box.bound_box.dimensions.x = 0.0f;
            state->boxes[i].destroyed = true;
        }
    }
}

void draw_boxes() {
    for (u32 i = 0; i < MAX_BOXES; i++) {
        if (!state->boxes[i].destroyed)
            draw_quad(obb_p0(&state->boxes[i].p_box.bound_box), obb_p1(&state->boxes[i].p_box.bound_box), state->boxes[i].color, NULL, VEC2F_ORIGIN, VEC2F_UNIT, NULL, state->boxes[i].p_box.bound_box.rot, NULL);
    }
}

void draw_boxes_outline() {
    for (u32 i = 0; i < MAX_BOXES; i++) {
        if (!state->boxes[i].destroyed)
            draw_quad_outline(obb_p0(&state->boxes[i].p_box.bound_box), obb_p1(&state->boxes[i].p_box.bound_box), vec4f_make(1.0f, 1.0f, 1.0f, 0.6f), state->boxes[i].p_box.bound_box.rot, NULL);
    }
}











/**
 * Player functions.
 */

void spawn_player(Vec2f position, Vec4f color) {
    state->player = (Player) {
        .p_box = (Phys_Box) {
                .bound_box = obb_make(position, 1.0f, 1.8f, 0.0f),
                .body = body_obb_make(30.0f, position, 1.0f, 1.8f, 0.0f, 0.45f, 0.3f),
                .dynamic = true,
                .rotatable = false,
                .destructible = false,
                .gravitable = true,
                .grounded = false,
                },
        .color = color,
        .speed = 0.0f,


        .sword = (Sword) {
            .origin = transform_make_trs_2d(position, 0.0f, vec2f_make(1.0f, 1.0f)),
            .handle = transform_make_trs_2d(position, 0.0f, vec2f_make(2.8f, 1.0f)),

            .angle_a = -30.0f,
            .angle_b = 60.0f,
            .animator = ti_make(0.2f),
        }   
    };

    array_list_append(&state->phys_boxes, &state->player.p_box);
}

void update_player(Player *p) {
    float x_vel = 0.0f;
    if (is_hold_keycode(SDLK_d)) {
        x_vel = 1.0f;

        transform_set_flip_x(&p->sword.handle, 1.0f);
        transform_set_flip_x(&p->sword.origin, 1.0f);
    } else if (is_hold_keycode(SDLK_a)) {
        x_vel = -1.0f;

        transform_set_flip_x(&p->sword.handle, -1.0f);
        transform_set_flip_x(&p->sword.origin, -1.0f);
    }


    p->p_box.body.velocity.x = lerp(p->p_box.body.velocity.x, x_vel * p->speed, 0.5f);

    if (is_pressed_keycode(SDLK_SPACE) && p->p_box.grounded) {
        phys_apply_force(&p->p_box.body, vec2f_make(0.0f, 140.0f));
    }


    // Applying calculated velocities. For both AABB and Rigid Body 2D.
    p->p_box.bound_box.center = vec2f_sum(p->p_box.bound_box.center, vec2f_multi_constant(p->p_box.body.velocity, state->t->delta_time));
    p->p_box.body.mass_center = p->p_box.bound_box.center;

    // Updating sword.
    transform_set_rotation_2d(&p->sword.origin, deg2rad(lerp(p->sword.angle_a, p->sword.angle_b, ease_in_back(ti_elapsed_percent(&p->sword.animator))) ));
    transform_set_rotation_2d(&p->sword.handle, deg2rad(lerp(p->sword.angle_a, p->sword.angle_b, ease_in_back(ti_elapsed_percent(&p->sword.animator))) ));

    transform_set_translation_2d(&p->sword.origin, p->p_box.bound_box.center);
    transform_set_translation_2d(&p->sword.handle, matrix4f_mul_vec2f(p->sword.origin, VEC2F_RIGHT));

    ti_update(&p->sword.animator, state->t->delta_time);


    if (is_pressed_keycode(SDLK_q)) {
        float temp = p->sword.angle_a;
        p->sword.angle_a = p->sword.angle_b;
        p->sword.angle_b = temp;

        ti_reset(&p->sword.animator);
    }
}

void draw_player(Player *p) {
    // Drawing player as a quad.
    draw_quad(obb_p0(&p->p_box.bound_box), obb_p1(&p->p_box.bound_box), p->color, NULL, VEC2F_ORIGIN, VEC2F_UNIT, NULL, p->p_box.bound_box.rot, NULL);
}











/**
 * Sword functions.
 */

void draw_sword_trail(Sword *s) {

}

void draw_sword_line(Sword *s) {
    // Drawing sword as a stick.
    Vec2f right = matrix4f_mul_vec2f(s->handle, VEC2F_RIGHT);
    Vec2f up = matrix4f_mul_vec2f(s->handle, VEC2F_UP);
    Vec2f origin = matrix4f_mul_vec2f(s->handle, VEC2F_ORIGIN);

    draw_line(origin, right, VEC4F_RED, NULL);
    draw_line(origin, up, VEC4F_GREEN, NULL);
}















void game_update() {

    /**
     * -----------------------------------
     *  Updating
     * -----------------------------------
     */


    // Spawning box.
    if (state->mouse_input.right_pressed) {
        Vec2f pos = camera_screen_to_world(state->mouse_input.position, &state->main_camera);
        spawn_box(pos, vec4f_make(randf(), randf(), randf(), 0.4f));
    } 



    
    // Time slow down input.
    if (is_pressed_keycode(SDLK_t)) {
        state->t->time_slow_factor++;
        state->t->delta_time_multi = 1.0f / (state->t->time_slow_factor % 5);
        
        printf("Delta Time Multiplier: %f\n", state->t->delta_time_multi);
    }

    // Player logic.
    update_player(&state->player);
    
    // Lerp camera.
    state->main_camera.center = vec2f_lerp(state->main_camera.center, state->player.p_box.bound_box.center, 0.025f);

    // Boxes logic.
    update_boxes();

    // Physics update loop.
    phys_update();


    // Switching game state to menu.
    if (is_pressed_keycode(SDLK_m)) {
        state->gs = MENU;
    }

}


void game_draw() {

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


    // Load camera's projection into all shaders to correctly draw elements according to camera view.
    Matrix4f projection;

    projection = camera_calculate_projection(&state->main_camera, (float)state->window_width, (float)state->window_height);

    shader_update_projection(quad_shader, &projection);
    shader_update_projection(grid_shader, &projection);
    shader_update_projection(line_shader, &projection);


    // Reset viewport.
    viewport_reset();

    // Clear the screen with clear_color.
    glClearColor(state->clear_color.x, state->clear_color.y, state->clear_color.z, state->clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);


    // Grid drawing.
    draw_begin(&state->grid_drawer);

    Vec2f p0, p1;
    p0 = vec2f_make(-8.0f, -5.0f);
    p1 = vec2f_make(8.0f, 5.0f);
    // draw_quad(p0, p1, vec4f_make(0.8f, 0.8f, 0.8f, 0.6f), NULL, p0, p1, NULL, 0.0f, NULL);
    draw_grid(p0, p1, vec4f_make(0.8f, 0.8f, 0.8f, 0.6f), NULL);

    draw_end();



    // Regular quad drawing.
    state->quad_drawer.program = quad_shader;
    draw_begin(&state->quad_drawer);

    draw_player(&state->player);
    draw_boxes();

    draw_end();




    // Line drawing.
    glLineWidth(2.0f);

    line_draw_begin(&state->line_drawer);

    draw_boxes_outline();

    draw_sword_line(&state->player.sword);


    // Test interpolation of angle matrix.
    Transform mat = transform_make_trs_2d(VEC2F_ORIGIN, deg2rad(30.0f), vec2f_make(1.0f, 2.0f));
    Transform reflection = transform_make_scale_2d(vec2f_make(-1.0f, 1.0f));
    mat = matrix4f_multiplication(&reflection, &mat);


    Vec2f right = matrix4f_mul_vec2f(mat, VEC2F_RIGHT);
    Vec2f up = matrix4f_mul_vec2f(mat, VEC2F_UP);
    Vec2f origin = matrix4f_mul_vec2f(mat, VEC2F_ORIGIN);

    draw_line(origin, right, VEC4F_RED, NULL);
    draw_line(origin, up, VEC4F_GREEN, NULL);




    line_draw_end();

    glLineWidth(1.0f);



    // Before drawing UI making final call to draw debug lines that are globally added to "debug_vert_buffer" throughout the update logic.
    vertex_buffer_draw_lines(&state->debug_vert_buffer, &state->line_drawer);
    vertex_buffer_clear(&state->debug_vert_buffer);
    


    // Get neccessary to draw resource: (Shaders, Fonts, Textures, etc.).
    state->ui.font = font_medium;

    Shader *ui_quad_shader = hash_table_get(&state->shader_table, "ui_quad", 7);

    projection = screen_calculate_projection(state->window_width, state->window_height);
    shader_update_projection(ui_quad_shader, &projection);

    state->quad_drawer.program = ui_quad_shader;


    // Resetting UI cursor.
    ui_cursor_reset();


    draw_begin(&state->quad_drawer);


    // Drawing button.
    if (ui_button(vec2f_make(100, 50), "Menu")) {
        state->gs = MENU;
    }

    ui_sameline();

    if (ui_button(vec2f_make(120, 80), "Spawn box")) {
        spawn_box(VEC2F_ORIGIN, vec4f_make(randf(), randf(), randf(), 0.4f));
    }

    ui_cursor_set(vec2f_make(20, state->window_height - 40));
    ui_text("Game");

    draw_end();

}








void menu_update() {
    // Switching game state back to play.
    if (is_pressed_keycode(SDLK_m)) {
        state->gs = PLAY;
    }
}


bool toggle;

void menu_draw() {
    // Reset viewport.
    viewport_reset();

    // Clear screen.
    glClearColor(0.1f, 0.0f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Get neccessary to draw resource: (Shaders, Fonts, Textures, etc.).
    Font_Baked *font_medium = hash_table_get(&state->font_table, "medium", 6);
    state->ui.font = font_medium;

    Shader *ui_quad_shader = hash_table_get(&state->shader_table, "ui_quad", 7);
    Shader *line_shader = hash_table_get(&state->shader_table, "line", 4);

    Matrix4f projection = screen_calculate_projection(state->window_width, state->window_height);
    shader_update_projection(ui_quad_shader, &projection);
    shader_update_projection(line_shader, &projection);

    state->quad_drawer.program = ui_quad_shader;
    state->line_drawer.program = line_shader;


    // Resetting UI cursor.
    ui_cursor_reset();

    draw_begin(&state->quad_drawer);


    // Buttons.



    if (ui_button(vec2f_make(100, 50), "Click me")) {
        toggle = !toggle;
    }

    if (toggle) {
        ui_button(vec2f_make(200, 100), "UI Buttons");
        if (ui_button(vec2f_make(200, 100), "GO BACK")) {
            state->gs = PLAY;
        }
        ui_button(vec2f_make(200, 100), "Centered text");
    }

    ui_cursor_set(vec2f_make(20, state->window_height - 40));
    ui_text("Menu");

    draw_end();

    vertex_buffer_draw_lines(&state->debug_vert_buffer, &state->line_drawer);
    vertex_buffer_clear(&state->debug_vert_buffer);

}








void plug_init(Plug_State *s) {
    state = s;

    // Setting random seed.
    srand((u32)time(NULL));


    // Debug verticies buffer allocation.
    state->debug_vert_buffer = vertex_buffer_make(); // @Leak



    /**
     * Setting hash tables for resources. 
     * Since they are dynamically allocated pointers will not change after hot reloading.
     */
    state->shader_table = hash_table_make(Shader, 8, &std_allocator);
    state->font_table = hash_table_make(Font_Baked, 8, &std_allocator);
    


    ui_init();
    phys_init();



    state->boxes = malloc(MAX_BOXES * sizeof(Box)); // @Leak.
    if (state->boxes == NULL)
        printf_err("Couldn't allocate boxes array.\n");
    
    // Small init of boxes for other algorithms to work properly.
    for (u32 i = 0; i < MAX_BOXES; i++) {
        state->boxes[i].destroyed = true;
    }

    // Set game state.
    s->gs = PLAY;



    spawn_rect(vec2f_make(-8.0f, -5.0f), vec2f_make(8.0f, -6.0f), vec4f_make(randf(), randf(), randf(), 0.4f));
    spawn_obstacle(vec2f_make(4.0f, -1.0f), 6.0f, 1.0f, vec4f_make(randf(), randf(), randf(), 0.4f), PI / 6);
    spawn_obstacle(vec2f_make(-4.0f, 1.5f), 6.0f, 1.0f, vec4f_make(randf(), randf(), randf(), 0.4f), -PI / 6);

    spawn_player(VEC2F_ORIGIN, vec4f_make(0.0f, 1.0f, 0.0f, 0.2f));
}

void plug_load(Plug_State *s) {
    state = s;

    state->clear_color = vec4f_make(0.1f, 0.1f, 0.4f, 1.0f);

    // Main camera init.
    state->main_camera = camera_make(VEC2F_ORIGIN, 48);

    // Shader loading.
    Shader grid_shader = shader_load("res/shader/grid.glsl");
    shader_init_uniforms(&grid_shader);
    printf("stride: %d\n", grid_shader.vertex_stride);
    hash_table_put(&state->shader_table, grid_shader, "grid", 4);

    Shader quad_shader = shader_load("res/shader/quad.glsl");
    shader_init_uniforms(&quad_shader);
    printf("stride: %d\n", quad_shader.vertex_stride);
    hash_table_put(&state->shader_table, quad_shader, "quad", 4);
    

    Shader line_shader = shader_load("res/shader/line.glsl");
    shader_init_uniforms(&line_shader);
    line_shader.vertex_stride = 7;
    hash_table_put(&state->shader_table, line_shader, "line", 4);

    Shader ui_quad_shader = shader_load("res/shader/ui_quad.glsl");
    shader_init_uniforms(&ui_quad_shader);
    ui_quad_shader.vertex_stride = 11;
    hash_table_put(&state->shader_table, ui_quad_shader, "ui_quad", 7);

    // Drawers init.
    drawer_init(&state->quad_drawer, hash_table_get(&state->shader_table, "quad", 4));
    drawer_init(&state->grid_drawer, hash_table_get(&state->shader_table, "grid", 4));
    line_drawer_init(&state->line_drawer, hash_table_get(&state->shader_table, "line", 4));



    // Font loading.
    u8* font_data = read_file_into_buffer("res/font/font.ttf", NULL, &std_allocator);



    Font_Baked font_baked_medium = font_bake(font_data, 20.0f);
    hash_table_put(&state->font_table, font_baked_medium, "medium", 6);

    Font_Baked font_baked_small = font_bake(font_data, 16.0f);
    hash_table_put(&state->font_table, font_baked_small, "small", 5);

    free(font_data);

    // Player loading.
    state->player.speed = 5.0f;
}


static s32 load_counter = 5;

/**
 * @Important: In game update loops the order of procedures is: Updating -> Drawing.
 * Where in updating all logic of the game loop is contained including inputs, sound and so on.
 * Drawing part is only responsible for putting pixels accrodingly with calculated data in "Updating" part.
 */
void plug_update(Plug_State *s) {

    // A simple and easy way to prevent delta_time jumps when loading.
    if (load_counter == 0) {
        state->t->delta_time_multi = 1.0f / (state->t->time_slow_factor % 5);
    }
    if (load_counter > -1) {
        load_counter--;
    }


    // Switch to handle different game states.
    switch (state->gs) {
        case PLAY:
            game_update();
            game_draw();
            break;
        case MENU:
            menu_update();
            menu_draw();
            break;
        default:
            printf_err("Couldn't handle current game state value.\n");
    }
   


    // Checking for gl error.
    check_gl_error();


    // Swap buffers to display the rendered image.
    SDL_GL_SwapWindow(state->window);
}

void plug_unload(Plug_State *s) {
    state->t->delta_time_multi = 0.0f;

    shader_unload(hash_table_get(&state->shader_table, "quad", 4));
    shader_unload(hash_table_get(&state->shader_table, "grid", 4));
    shader_unload(hash_table_get(&state->shader_table, "line", 4));
    
    drawer_free(&state->quad_drawer);
    drawer_free(&state->grid_drawer);
    line_drawer_free(&state->line_drawer);
    
    font_free(hash_table_get(&state->font_table, "medium", 6));
    font_free(hash_table_get(&state->font_table, "small", 5));
}































/**
 * Helper functions defenitions.
 */
void draw_quad(Vec2f p0, Vec2f p1, Vec4f color, Texture *texture, Vec2f uv0, Vec2f uv1, Texture *mask, float offset_angle, Vertex_Buffer *buffer) {
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
    
    if (buffer == NULL)
        draw_quad_data(quad_data, 1);
    else
        vertex_buffer_append_data(buffer, quad_data, VERTICIES_PER_QUAD * 11);
}

void draw_grid(Vec2f p0, Vec2f p1, Vec4f color, Vertex_Buffer *buffer) {
    Vec2f p2 = vec2f_make(p1.x, p0.y);
    Vec2f p3 = vec2f_make(p0.x, p1.y);
    
    float quad_data[36] = {
        p0.x, p0.y, 0.0f, color.x, color.y, color.z, color.w, p0.x, p0.y,
        p2.x, p2.y, 0.0f, color.x, color.y, color.z, color.w, p2.x, p2.y,
        p3.x, p3.y, 0.0f, color.x, color.y, color.z, color.w, p3.x, p3.y,
        p1.x, p1.y, 0.0f, color.x, color.y, color.z, color.w, p1.x, p1.y,
    };
    
    if (buffer == NULL)
        draw_quad_data(quad_data, 1);
    else
        vertex_buffer_append_data(buffer, quad_data, VERTICIES_PER_QUAD * 9);
}

void draw_text(const char *text, Vec2f current_point, Vec4f color, Font_Baked *font, u32 unit_scale, Vertex_Buffer *buffer) {
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

            draw_quad(vec2f_divide_constant(vec2f_make(current_point.x + c->xoff, current_point.y - c->yoff - height), (float)unit_scale), vec2f_divide_constant(vec2f_make(current_point.x + c->xoff + width, current_point.y - c->yoff), (float)unit_scale), color, NULL, vec2f_make(c->x0 / (float)font->bitmap.width, c->y1 / (float)font->bitmap.height), vec2f_make(c->x1 / (float)font->bitmap.width, c->y0 / (float)font->bitmap.height), &font->bitmap, 0.0f, buffer);

            current_point.x += font->chars[font_char_index].xadvance;
        }
    }
}

Vec2f text_size(const char *text, Font_Baked *font) {
    u64 text_length = strlen(text);

    // Result.
    Vec2f result = VEC2F_ORIGIN;

    // Scale and adjust current_point.
    Vec2f current_point = VEC2F_ORIGIN;
    float origin_x = current_point.x;
    current_point.y += (float)font->baseline;

    // Text rendering variables.
    s32 font_char_index;

    for (u64 i = 0; i < text_length; i++) {
        // Handle special characters / symbols.
        if (text[i] == '\n') {
            result.y += (float)font->line_height;
            if (result.x < current_point.x)
                result.x = current_point.x;

            current_point.x = origin_x;
            current_point.y -= (float)font->line_height;
            continue;
        }

        // Character iterating.
        font_char_index = (s32)text[i] - font->first_char_code;
        if (font_char_index < font->chars_count) {

            current_point.x += font->chars[font_char_index].xadvance;
        }
    }
    
    // Since last line dimensions is not handled in the loop, it can be done here.
    result.y += (float)font->line_height;
    if (result.x < current_point.x)
        result.x = current_point.x;

    return result;
}



void draw_line(Vec2f p0, Vec2f p1, Vec4f color, Vertex_Buffer *buffer) {
    float line_data[14] = {
        p0.x, p0.y, 0.0f, color.x, color.y, color.z, color.w,
        p1.x, p1.y, 0.0f, color.x, color.y, color.z, color.w,
    };

    if (buffer == NULL)
        draw_line_data(line_data, 1);
    else
        vertex_buffer_append_data(buffer, line_data, VERTICIES_PER_LINE * 7);
}


void draw_dot(Vec2f position, Vec4f color, Vertex_Buffer *buffer) {
    draw_quad(vec2f_make(position.x - (float)state->main_camera.unit_scale * DOT_SCALE, position.y), vec2f_make(position.x + (float)state->main_camera.unit_scale * DOT_SCALE, position.y), color, NULL, VEC2F_ORIGIN, VEC2F_UNIT, NULL, PI/4, buffer);
}

void draw_quad_outline(Vec2f p0, Vec2f p1, Vec4f color, float offset_angle, Vertex_Buffer *buffer) {
    
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

    if (buffer == NULL)
        draw_line_data(line_data, 4);
    else
        vertex_buffer_append_data(buffer, line_data, 4 * VERTICIES_PER_LINE * 7);
}

void draw_circle_outline(Vec2f position, float radius, u32 detail, Vec4f color, Vertex_Buffer *buffer) {

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
    
   
    if (buffer == NULL)
        draw_line_data(line_data, detail);
    else
        vertex_buffer_append_data(buffer, line_data, detail * VERTICIES_PER_LINE * 7);
}

void draw_function(float x0, float x1, Function y, u32 detail, Vec4f color, Vertex_Buffer *buffer) {
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
    
   
    if (buffer == NULL)
        draw_line_data(line_data, detail);
    else
        vertex_buffer_append_data(buffer, line_data, detail * VERTICIES_PER_LINE * 7);
}

void draw_polar(float t0, float t1, Function r, u32 detail, Vec4f color, Vertex_Buffer *buffer) {
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
    
   
    if (buffer == NULL)
        draw_line_data(line_data, detail);
    else
        vertex_buffer_append_data(buffer, line_data, detail * VERTICIES_PER_LINE * 7);
}

void draw_parametric(float t0, float t1, Function x, Function y, u32 detail, Vec4f color, Vertex_Buffer *buffer) {
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
    
   
    if (buffer == NULL)
        draw_line_data(line_data, detail);
    else
        vertex_buffer_append_data(buffer, line_data, detail * VERTICIES_PER_LINE * 7);
}

void draw_area_function(float x0, float x1, Function y, u32 rect_count, Vec4f color, Vertex_Buffer *buffer) {
    float step = (x1 - x0) / (float)rect_count;
    for (u32 i = 0; i < rect_count; i++) {
        draw_quad(vec2f_make(x0 + step * i, 0.0f), vec2f_make(x0 + step * (i + 1), y(x0 + step * (i + 1))), color, NULL, VEC2F_ORIGIN, VEC2F_UNIT, NULL, 0.0f, buffer);
    }
}

void draw_area_polar(float t0, float t1, Function r, u32 rect_count, Vec4f color, Vertex_Buffer *buffer) {
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
    
    if (buffer == NULL)
        draw_quad_data(quad_data, rect_count);
    else
        vertex_buffer_append_data(buffer, quad_data, rect_count * VERTICIES_PER_QUAD * 11);

}

void draw_area_parametric(float t0, float t1, Function x, Function y, u32 rect_count, Vec4f color, Vertex_Buffer *buffer) {
    float step = (t1 - t0) / (float)rect_count;
    for (u32 i = 0; i < rect_count; i++) {
        draw_quad(vec2f_make(x(t0 + step * i), 0.0f), vec2f_make(x(t0 + step * (i + 1)), y(t0 + step * (i + 1))), color, NULL, VEC2F_ORIGIN, VEC2F_UNIT, NULL, 0.0f, buffer);
    }
}


void draw_viewport(u32 x, u32 y, u32 width, u32 height, Vec4f color, Camera *camera, Vertex_Buffer *buffer) {
    glViewport(x, y, width, height);
    
    Vec2f p1 = vec2f_make((float)width / 2 / (float)camera->unit_scale, (float)height / 2 / (float)camera->unit_scale);
    Vec2f p0 = vec2f_negate(p1);
    
    draw_quad(p0, p1, color, NULL, p0, p1, NULL, 0.0f, buffer);

}

void viewport_reset() {
    glViewport(0, 0, state->window_width, state->window_height);
}

Vec2f camera_screen_to_world(Vec2f point, Camera *camera) {
    return vec2f_make(camera->center.x + (point.x - (float)state->window_width / 2) / (float)camera->unit_scale, camera->center.y + (point.y - (float)state->window_height / 2) / (float)state->main_camera.unit_scale);
}








