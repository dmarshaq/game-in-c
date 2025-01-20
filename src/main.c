#include "core.h"

int main() {
    
    Vec2f v1 = vec2f_make(2.0f, 1.5f);
    Vec2f v2 = vec2f_make(-1.5f, 1.0f);

    v1 = vec2f_difference_constant(v1, 2.0f);
    v1 = vec2f_multi_constant(v1, 2.0f);

    v2 = vec2f_sum(v1, v2);
    
    vec2f_print(v1);
    vec2f_print(v2);
    printf("dot = %2.2f\n", vec2f_dot(v1, v2));
    
    // Vec2f v3 = vec2f_difference(v2, VEC2F_RIGHT);
    printf("%2.2f\n", vec2f_distance(v1, v2));
    vec2f_print(vec2f_make(0.5f, vec2f_distance(v1, v2)));
    vec2f_print(vec2f_difference(vec2f_make(0.5f, vec2f_distance(v1, v2)), VEC2F_RIGHT));
    
    // v1 = vec2f_make(0.5f, vec2f_distance(v1, v2));
    return 0;
}
