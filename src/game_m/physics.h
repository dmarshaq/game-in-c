#ifndef PHYSICS_H
#define PHYSICS_H

#include "core/mathf.h"
#include "core/core.h"

#define calculate_obb_inertia(mass, width, height)                                          ((1.0f / 12.0f) * mass * (height * height + width * width))

typedef struct body_2d {
    Vec2f velocity;
    float angular_velocity;

    float mass;
    float inv_mass;
    float inertia;
    float inv_inertia;
    Vec2f mass_center;
    float restitution;
    float static_friction;
    float dynamic_friction;
} Body_2D;

#define body_obb_make(mass, center, width, height, restitution, static_friction, dynamic_friction)                             ((Body_2D) { VEC2F_ORIGIN, 0.0f, mass, (mass == 0.0f ? 0.0f : 1.0f / mass), calculate_obb_inertia(mass, width, height), (mass == 0.0f ? 0.0f : 1.0f / calculate_obb_inertia(mass, width, height)), center, restitution, static_friction, dynamic_friction })


typedef struct impulse {
    Vec2f delta_force;
    u32 milliseconds;
    Body_2D *target;
} Impulse;

/**
 * Some of these flags in theory can be moved to rigid body 2d to abstact shape from body when resolving collisions.
 */
typedef struct phys_box {
    OBB bound_box;
    Body_2D body;
    
    bool dynamic;
    bool rotatable;
    bool destructible;
    bool gravitable;
    bool grounded;
} Phys_Box;


/**
 * Applies instanteneous force to rigid body.
 */
void phys_apply_force(Body_2D *body, Vec2f force);

/**
 * Applies instanteneous acceleration to rigid body.
 */
void phys_apply_acceleration(Body_2D *body, Vec2f acceleration);

void phys_apply_angular_acceleration(Body_2D *body, float acceleration);

void phys_update(Phys_Box **phys_boxes, s64 length, Time_Info *t);


#endif
