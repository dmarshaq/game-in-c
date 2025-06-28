#ifndef CORE_H
#define CORE_H

#include "core/type.h"

/**
 * Debug.
 */

#include <stdio.h>
#include <stdbool.h>

extern const char *debug_error_str;
extern const char *debug_warning_str;
extern const char *debug_ok_str;


#define printf_err(format, ...)                 (void)fprintf(stderr, "%s " format, debug_error_str, ##__VA_ARGS__);

#ifdef IGNORE_WARNING

#define printf_warning(format, ...)

#else

#define printf_warning(format, ...)             (void)fprintf(stderr, "%s " format, debug_warning_str, ##__VA_ARGS__);

#endif

#define printf_ok(format, ...)                  (void)fprintf(stderr, "%s " format, debug_ok_str, ##__VA_ARGS__);



/**
 * Allocators.
 * @Important: There are different ways to approach memory allocation situation, using malloc() calloc() and realloc() is all great, and if used correctly can benefit the program flow.
 * The problem that arises is when you want to have more flexibility over memory use.
 * For example - arena allocator uses pre-allocated memory to linearly distribute this "prepared" memory.
 * And ideally all allocations of such should be easily done, without any overhead.
 * So the use of dynamic memory becomes more convinient and logical when deciding what to use Stack or Heap.
 * It is important that the whole process doesn't become super simple, since it is not about making garbade collection system, but about making the whole process more organized and easy to follow.
 * This is why free(), malloc(), calloc(), realloc() and other std calls is fine to use. It is even convinient to provide an Allocator instance that directly calls this functions.
 * The idea is to have a workflow where if you allocate dynamically something in the function, and the function returns it to you, you pass your allocator of choice to do this job.
 *
 * Arena allocator.
 */


#include <stdlib.h>

typedef struct allocator_header {
    u64 capacity;
    u64 size_filled;
} Allocator_Header;

typedef void *(*Allocate)(Allocator_Header *header, u64 size);
typedef void *(*Zero_Allocate)(Allocator_Header *header, u64 size);
typedef void *(*Re_Allocate)(Allocator_Header *header, void *ptr, u64 size);
typedef void  (*Free)(Allocator_Header *header, void *ptr);

typedef struct allocator {
    Allocator_Header    *ptr;
    Allocate            alc_alloc;
    Zero_Allocate       alc_zero_alloc;
    Re_Allocate         alc_re_alloc;
    Free                alc_free;
} Allocator;

// Std.
extern Allocator std_allocator;

// Arena.
typedef Allocator Arena_Allocator;

Arena_Allocator arena_make(u64 capacity);
void arena_destroy(Arena_Allocator *arena);

// Allocator interface.
void *allocator_alloc(Allocator *allocator, u64 size);
void *allocator_zero_alloc(Allocator *allocator, u64 size);
void *allocator_re_alloc(Allocator *allocator, void *ptr, u64 size);
void  allocator_free(Allocator *allocator, void *ptr);



/**
 * Time.
 */
typedef struct time_data {
    u32 current_time;
    u32 delta_time_milliseconds;
    float delta_time;

    float delta_time_multi;
    u32 time_slow_factor;

    u32 last_update_time;
    u32 accumilated_time;
    u32 update_step_time;
} Time_Data;


typedef struct time_interpolator {
    float duration;
    float elapsed_t;
} T_Interpolator;

#define ti_make(duration)                   ((T_Interpolator) { duration, 0.0f })

void ti_update(T_Interpolator *interpolator, float delta_time);
float ti_elapsed_percent(T_Interpolator *interpolator);
bool ti_is_complete(T_Interpolator *interpolator);
void ti_reset(T_Interpolator *interpolator);



#endif
