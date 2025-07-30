#include "core/arena.h"

#include "core/type.h"
#include "core/core.h"

#include <stdlib.h>


Arena arena_make(u64 capacity) {
    void *mem = malloc(capacity);

    if (mem == NULL) {
        printf_err("Couldn't malloc %lld bytes of memory for the arena.\n", capacity);
        return (Arena) {0};
    }

    return (Arena) {
        .capacity = capacity,
        .allocation = mem,
        .ptr = mem,
    };
}

void *arena_alloc(Arena *arena, u64 size) {
    arena->ptr += size;

    if (arena->ptr > arena->allocation + arena->capacity) {
        printf_err("Couldn't allocate %lld bytes of memory from the arena, this allocation exceeded arena's capacity.\n", size);
        return NULL;
    }

    return arena->ptr - size;
}

u64 arena_size(Arena *arena) {
    return arena->ptr - arena->allocation;
}

void arena_clear(Arena *arena) {
    arena->ptr = arena->allocation;
}

void arena_free(Arena *arena) {
    free(arena->allocation);

    *arena = (Arena) {0};
}

