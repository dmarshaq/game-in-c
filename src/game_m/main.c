#include "core/core.h"
#include "core/structs.h"

#include "game/game.h"
#include "game/graphics.h"
#include "game/input.h"





int main(int argc, char **argv) {
    printf("Hello world!\n");

    return 0;
}



static State *state;

int main2(int argc, char **argv) {
    // Allocate global state.
    state = allocator_alloc(&std_allocator, sizeof(State));
    
    state->t.delta_time_multi = 1.0f;
    state->t.time_slow_factor = 1;
    state->t.last_update_time = 0; 
    state->t.update_step_time = 10; // Milliseconds per one update time

    init(state);
    load(state);

    while (!state->events.should_quit) {
        // Time management.
        state->t.current_time = SDL_GetTicks();
        state->t.accumilated_time += state->t.current_time - state->t.last_update_time;
        state->t.last_update_time = state->t.current_time;
        
        if (state->t.accumilated_time < state->t.update_step_time) {
            continue;
        }
        
        state->t.delta_time_milliseconds = (u32)((float)state->t.accumilated_time * state->t.delta_time_multi);
        state->t.delta_time = (float)(state->t.delta_time_milliseconds) / 1000.0f;
        state->t.accumilated_time = state->t.accumilated_time % state->t.update_step_time;

        
        update(state);

    }

    unload(state);

    return 0;
}
