#include "plug.h"
#include "SDL2/SDL_keycode.h"
#include "core.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

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
void phys_apply_force(Body_2D *body, Vec2f force) {
    body->velocity = vec2f_sum(body->velocity, vec2f_multi_constant(force, body->inv_mass));
}

/**
 * Applies instanteneous acceleration to rigid body.
 */
void phys_apply_acceleration(Body_2D *body, Vec2f acceleration) {
    body->velocity = vec2f_sum(body->velocity, acceleration);
}

void phys_apply_angular_force(Body_2D *body, float force) {
    body->angular_velocity += force * body->inv_mass;
}

void phys_apply_angular_acceleration(Body_2D *body, float acceleration) {
    body->angular_velocity += acceleration;
}



#define MAX_IMPULSES        16

#define MAX_PHYS_BOXES      16

void phys_init() {
    // Setting physics variables.
    global_state->impulses = array_list_make(Impulse, MAX_IMPULSES, &std_allocator); // @Leak.
    global_state->phys_boxes = array_list_make(Phys_Box, MAX_PHYS_BOXES, &std_allocator); // @Leak.

}

void phys_add_impulse(Vec2f force, u32 milliseconds, Body_2D *body) {
    Impulse impulse = (Impulse) { 
        .delta_force = vec2f_divide_constant(force, (float)milliseconds), 
        .milliseconds = milliseconds, 
        .target = body 
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
        if (depths[0] > depths[i]) {
            depths[0] = depths[i];
            normals[0] = normals[i];
        }
    }

    // Flip normal if it's not looking in direction of collosion.
    if (vec2f_dot(normals[0], vec2f_normalize(vec2f_difference(obb1->center, obb2->center))) < 0.0f) {
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

    obb1->center = vec2f_sum(obb1->center, displacement);
    obb2->center = vec2f_difference(obb2->center, displacement);
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
    
    Quad_Drawer *quad_drawer = &global_state->drawer;
    draw_begin(quad_drawer);
    
    for (u32 i = 0; i < contacts_count; i++) {
        draw_dot(contacts[i], VEC4F_RED);
    }

    draw_end();

    check_gl_error();
    float e = fminf(body1->restitution, body2->restitution);
    float impulses[2];
    Vec2f r1_array[2];
    Vec2f r2_array[2];

    for (u32 i = 0; i < contacts_count; i++) {

        Vec2f r1 = vec2f_difference(contacts[i], body1->mass_center);
        Vec2f r2 = vec2f_difference(contacts[i], body2->mass_center);

        r1_array[i] = r1;
        r2_array[i] = r2;

        Vec2f r1_perp = vec2f_make(-r1.y, r1.x);
        Vec2f r2_perp = vec2f_make(-r2.y, r2.x);

        Vec2f angular_lin_velocity1 = vec2f_multi_constant(r1_perp, body1->angular_velocity);
        Vec2f angular_lin_velocity2 = vec2f_multi_constant(r2_perp, body2->angular_velocity);

        Vec2f relative_velocity = vec2f_difference(
                vec2f_sum(body2->velocity, angular_lin_velocity2), 
                vec2f_sum(body1->velocity, angular_lin_velocity1));

        float contact_velocity_mag = vec2f_dot(relative_velocity, normal);

        if (contact_velocity_mag > 0.0f) {
            continue;
        }

        float r1_perp_dot_n = vec2f_dot(r1_perp, normal);
        float r2_perp_dot_n = vec2f_dot(r2_perp, normal);

        float j = -(1.0f + e) * contact_velocity_mag;
        j /= (1.0f * body1->inv_mass + 1.0f * body2->inv_mass) + (r1_perp_dot_n * r1_perp_dot_n) * body1->inv_inertia + (r2_perp_dot_n * r2_perp_dot_n) * body2->inv_inertia;
        j /= (float)contacts_count;

        impulses[i] = j;
    }

    Vec2f impulse;
    for (u32 i = 0; i < contacts_count; i++) {
        impulse = vec2f_multi_constant(normal, -impulses[i]);
        phys_apply_force(body1, impulse);
        phys_apply_angular_acceleration(body1, vec2f_cross(r1_array[i], impulse) * body1->inv_inertia);

        impulse = vec2f_multi_constant(normal, impulses[i]);
        phys_apply_force(body2, impulse);
        phys_apply_angular_acceleration(body2, vec2f_cross(r2_array[i], impulse) * body2->inv_inertia);
    }
}

/**
 * Sets dynamic obb apart based on depth and normal.
 * Usefull for colliding dynamic object with immovable or static object.
 */
void phys_resolve_static_obb_collision(OBB *obb, float depth, Vec2f normal) {
    obb->center = vec2f_sum(obb->center, vec2f_multi_constant(normal, depth));
}

// void phys_resolve_static_body_collision_basic(Body_2D *body1, Vec2f normal) {
//     
//     Vec2f relative_velocity = body1->velocity;
// 
//     float e = body1->restitution;
// 
//     float j = -(1.0f + e) * vec2f_dot(relative_velocity, normal);
//     j /= (1.0f / body1->mass);
// 
//     phys_apply_force(body1, vec2f_multi_constant(normal, j));
// }
// 
// void phys_resolve_static_body_collision(Body_2D *body1, Vec2f normal, Vec2f *contacts, u32 contacts_count) {
// 
//     float e = fminf(body1->restitution, body2->restitution);
//     float impulses[2];
//     Vec2f r1_array[2];
//     Vec2f r2_array[2];
// 
//     for (u32 i = 0; i < contacts_count; i++) {
// 
//         Vec2f r1 = vec2f_difference(contacts[i], body1->mass_center);
//         Vec2f r2 = vec2f_difference(contacts[i], body2->mass_center);
// 
//         r1_array[i] = r1;
//         r2_array[i] = r2;
// 
//         Vec2f r1_perp = vec2f_make(-r1.y, r1.x);
//         Vec2f r2_perp = vec2f_make(-r2.y, r2.x);
// 
//         Vec2f angular_lin_velocity1 = vec2f_multi_constant(r1_perp, body1->angular_velocity);
//         Vec2f angular_lin_velocity2 = vec2f_multi_constant(r2_perp, body2->angular_velocity);
// 
//         Vec2f relative_velocity = vec2f_difference(
//                 vec2f_sum(body2->velocity, angular_lin_velocity2), 
//                 vec2f_sum(body1->velocity, angular_lin_velocity1));
// 
//         float contact_velocity_mag = vec2f_dot(relative_velocity, normal);
// 
//         if (contact_velocity_mag > 0.0f) {
//             // printf("Contact vel mag: %2.2f\n", contact_velocity_mag);
//             // vec2f_print(relative_velocity);
//             continue;
//         }
// 
//         float r1_perp_dot_n = vec2f_dot(r1_perp, normal);
//         float r2_perp_dot_n = vec2f_dot(r2_perp, normal);
// 
//         float j = -(1.0f + e) * contact_velocity_mag;
//         j /= (1.0f / body1->mass + 1.0f / body2->mass) + (r1_perp_dot_n * r1_perp_dot_n) / body1->inertia + (r2_perp_dot_n * r2_perp_dot_n) / body2->inertia;
//         j /= (float)contacts_count;
// 
//         impulses[i] = j;
//     }
// 
//     Vec2f impulse;
//     for (u32 i = 0; i < contacts_count; i++) {
//         impulse = vec2f_multi_constant(normal, -impulses[i]);
//         phys_apply_force(body1, impulse);
//         phys_apply_angular_acceleration(body1, vec2f_cross(r1_array[i], impulse) / body1->inertia);
// 
//         impulse = vec2f_multi_constant(normal, impulses[i]);
//         phys_apply_force(body2, impulse);
//         phys_apply_angular_acceleration(body2, vec2f_cross(r2_array[i], impulse) / body2->inertia);
//     }
// }

#define ONE_POINT_OPTIMIZATION_THRESHOLD 0.00001f

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


                // printf("p: %u, a: %u, b: %u, o: %u\n", i + o * 4, j + 4 - o * 4, ((j + 1) % 4) + 4 - o * 4, o);
                
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

    // if (min_dist1 - min_dist2 != 0.0f) {
    //     printf_ok("min distance not close, take the smallest\n");
    //     return 1;
    // }
    // if (fabsf(points[0].x - points[1].x) < ONE_POINT_OPTIMIZATION_THRESHOLD && fabsf(points[0].y - points[1].y) < ONE_POINT_OPTIMIZATION_THRESHOLD) {
    //     printf_ok("one point\n");
    //     return 1;
    // }

    // return 2;
}



void spawn_box(Vec2f position, Vec4f color) {
    array_list_append(&global_state->phys_boxes, ((Phys_Box) {
                .bound_box = obb_make(position, 1.0f, 1.0f, 0.0f),
                .body = body_obb_make(20.0f, position, 1.0f, 1.0f, 0.2f),
                .color = color,
                .is_static = false
            }));
}

void spawn_rect(Vec2f p0, Vec2f p1, Vec4f color) {
    array_list_append(&global_state->phys_boxes, ((Phys_Box) {
                .bound_box = obb_make(vec2f_sum(p0, vec2f_divide_constant(vec2f_difference(p1, p0), 2)), p1.x - p0.x, p1.y - p0.y, 0.0f),
                .body = body_obb_make(0.0f, vec2f_sum(p0, vec2f_divide_constant(vec2f_difference(p1, p0), 2)), p1.x - p0.x, p1.y - p0.y, 0.2f),
                .color = color,
                .is_static = true,
            }));
}

void update_boxes() {
    u32 length = array_list_length(&global_state->phys_boxes);

    float depth;
    Vec2f normal;
    Phys_Box *box1;
    Phys_Box *box2;

    
    for (u32 i = 0; i < length; i++) {
        box1 = &global_state->phys_boxes[i];
        
        // Applying gravity.
        if (!box1->is_static) {
            box1->body.velocity = vec2f_sum(box1->body.velocity, vec2f_multi_constant(gravity_acceleration, global_state->t->delta_time));
        }

        // Applying velocities.
        box1->bound_box.center = vec2f_sum(box1->bound_box.center, vec2f_multi_constant(box1->body.velocity, global_state->t->delta_time));
        box1->body.mass_center = box1->bound_box.center;
        box1->bound_box.rot += box1->body.angular_velocity * global_state->t->delta_time;
    }


    Vec2f contacts[2];
    u32 contacts_count;
    // Collision.
    for (u32 i = 0; i < length; i++) {
        box1 = &global_state->phys_boxes[i];
        for (u32 j = i + 1; j < length; j++) {
            box2 = &global_state->phys_boxes[j];
            if (phys_sat_check_collision_obb(&box1->bound_box, &box2->bound_box)) {
                phys_sat_find_min_depth_normal(&box1->bound_box, &box2->bound_box, &depth, &normal);
                if (global_state->phys_boxes[i].is_static || global_state->phys_boxes[j].is_static) {
                    phys_resolve_static_obb_collision(&box1->bound_box, depth, normal);
                }
                else {
                    phys_resolve_dynamic_obb_collision(&box1->bound_box, &box2->bound_box, depth, normal);
                }
                contacts_count = phys_find_contanct_points_obb(&box1->bound_box, &box2->bound_box, contacts);
                phys_resolve_dynamic_body_collision(&box1->body, &box2->body, normal, contacts, contacts_count);
            }
        }
    }
}

void draw_boxes() {
    u32 length = array_list_length(&global_state->phys_boxes);
    for (u32 i = 0; i < length; i++) {
        draw_quad(obb_p0(&global_state->phys_boxes[i].bound_box), obb_p1(&global_state->phys_boxes[i].bound_box), global_state->phys_boxes[i].color, NULL, VEC2F_ORIGIN, VEC2F_UNIT, NULL, global_state->phys_boxes[i].bound_box.rot);
    }
}


void plug_init(Plug_State *state) {
    srand((u32)time(NULL));
    global_state = state;

    /**
     * Setting hash tables for resources. 
     * Since they are dynamically allocated pointers will not change after hot reloading.
     */
    state->shader_table = hash_table_make(Shader, 8, &std_allocator);
    state->font_table = hash_table_make(Font_Baked, 8, &std_allocator);
    
    phys_init();
    
    state->player = (Player) {
        .bound_box = obb_make(VEC2F_ORIGIN, 1.0f, 1.0f, 0.0f),
        .body = body_obb_make(10.0f, VEC2F_ORIGIN, 1.0f, 1.0f, 0.2f),
        .speed = 0.0f,
    };

    for (u32 i = 0; i < 10; i++) {
        spawn_box(vec2f_make(randf() * 10.0f - 5.0f, randf() * 10.0f - 5.0f), vec4f_make(randf(), randf(), randf(), 1.0f));
    }
    
    spawn_rect(vec2f_make(-8.0f, 5.0f), vec2f_make(8.0f, 6.0f), vec4f_make(randf(), randf(), randf(), 1.0f));
    spawn_rect(vec2f_make(-8.0f, -5.0f), vec2f_make(8.0f, -6.0f), vec4f_make(randf(), randf(), randf(), 1.0f));
    spawn_rect(vec2f_make(-9.0f, -6.0f), vec2f_make(-8.0f, 6.0f), vec4f_make(randf(), randf(), randf(), 1.0f));
    spawn_rect(vec2f_make(8.0f, -6.0f), vec2f_make(9.0f, 6.0f), vec4f_make(randf(), randf(), randf(), 1.0f));
}


void plug_load(Plug_State *state) {
    // Test
    float dist = point_segment_min_distance(vec2f_make(1.0f, 2.0f), vec2f_make(-1.0f, 1.0f), vec2f_make(0.0f, 1.0f));
    printf("Dist: %2.2f\n", dist);




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

    // box1 = obb_make(vec2f_make(0.0f, 3.5f), 2.0f, 1.5f, 0.0f);
    // box2 = obb_make(vec2f_make(0.0f, 0.0f), 1.2f, 1.6f, 0.0f);

    global_state = state;


}



/**
 * Interface for functions.
 */
void update_player(Player *p);

void draw_player(Player *p);

Vec2f test_rot = vec2f_make(2.0f, 0.0f);
float angle = 90.0f;

s32 load_counter = 5;

/**
 * @Important: In game update loops the order of procedures is: Updating -> Drawing.
 * Where in updating all logic of the game loop is contained including inputs, sound and so on.
 * Drawing part is only responsible for putting pixels accrodingly with calculated data in "Updating" part.
 */
void plug_update(Plug_State *state) {
    
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









    // A simple and easy way to prevent delta_time jumps when loading.
    if (load_counter == 0) {
        state->t->delta_time_multi = 1.0f / (state->t->time_slow_factor % 5);
    }
    if (load_counter > -1) {
        load_counter--;
    }

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
        state->t->time_slow_factor++;
        state->t->delta_time_multi = 1.0f / (state->t->time_slow_factor % 5);
        
        pop_up_log("Delta Time Multiplier: %f\n", state->t->delta_time_multi);
    }

    // Player logic.
    update_player(&state->player);
    
    // Lerp camera.
    state->main_camera.center = vec2f_lerp(state->main_camera.center, state->player.bound_box.center, 0.025f);



    // Test rotation.
    angle += PI / 4 * state->t->delta_time;
    test_rot = vec2f_rotate(test_rot, PI/2 * state->t->delta_time);

    // Test obb.
    update_boxes();
    // box2.center.y += state->t->delta_time;

    // OBB *obb1 = &box1;
    // OBB *obb2 = &box2;

    // Vec4f obb_color = VEC4F_GREEN;
    // if (phys_sat_check_collision_obb(obb1, obb2)) {
    //     obb_color = VEC4F_RED;
    // }
    // phys_sat_resolve_if_collision_obb(obb1, obb2);

    /**
     * -----------------------------------
     *  Drawing
     * -----------------------------------
     */



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

    draw_boxes();

    // draw_quad(obb_p0(obb1), obb_p1(obb1), obb_color, NULL, VEC2F_ORIGIN, VEC2F_UNIT, NULL, obb1->rot);
    // draw_quad(obb_p0(obb2), obb_p1(obb2), obb_color, NULL, VEC2F_ORIGIN, VEC2F_UNIT, NULL, obb2->rot);
    
    draw_end();






    // Line drawing.
    line_draw_begin(&state->line_drawer);


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
    
    Vec2f vel = VEC2F_ORIGIN;
    if (is_hold_keycode(SDLK_d)) {
        vel.x = 1.0f;
    }
    if (is_hold_keycode(SDLK_a)) {
        vel.x = -1.0f;
    }
    if (is_hold_keycode(SDLK_w)) {
        vel.y = 1.0f;
    }
    if (is_hold_keycode(SDLK_s)) {
        vel.y = -1.0f;
    }
    
    vel = vec2f_normalize(vel);
    vel.x *= p->speed;
    vel.y *= p->speed;

    p->body.velocity = vec2f_lerp(p->body.velocity, vel, 50.0f * global_state->t->delta_time);
 

    // Applying calculated velocities. For both AABB and Rigid Body 2D.
    p->bound_box.center = vec2f_sum(p->bound_box.center, vec2f_multi_constant(p->body.velocity, global_state->t->delta_time));
    p->body.mass_center = p->bound_box.center;

    u32 length = array_list_length(&global_state->phys_boxes);

    float depth;
    Vec2f normal;

    Vec2f contacts[2];
    u32 contacts_count;
    for (u32 i = 0; i < length; i++) {
        if (phys_sat_check_collision_obb(&p->bound_box, &global_state->phys_boxes[i].bound_box)) {
            phys_sat_find_min_depth_normal(&p->bound_box, &global_state->phys_boxes[i].bound_box, &depth, &normal);
            if (global_state->phys_boxes[i].is_static) {
                phys_resolve_static_obb_collision(&p->bound_box, depth, normal);
            }
            else {
                phys_resolve_dynamic_obb_collision(&p->bound_box, &global_state->phys_boxes[i].bound_box, depth, normal);
            }
            contacts_count = phys_find_contanct_points_obb(&p->bound_box, &global_state->phys_boxes[i].bound_box, contacts);
            phys_resolve_dynamic_body_collision(&p->body, &global_state->phys_boxes[i].body, normal, contacts, contacts_count);
        }
    }
    
    
}

void draw_player(Player *p) {
    // Drawing player as a quad.
   draw_quad(obb_p0(&p->bound_box), obb_p1(&p->bound_box), VEC4F_GREEN, NULL, VEC2F_ORIGIN, VEC2F_UNIT, NULL, 0.0f);
}





// Plug unload.
void plug_unload(Plug_State *state) {
    state->t->delta_time_multi = 0.0f;

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
            // continue;
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

#define DOT_SCALE 0.005f

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









