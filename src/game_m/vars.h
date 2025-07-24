#ifndef VARS_H
#define VARS_H

#include "core/type.h"

@Introspect;
typedef struct console {
    s64 speed;
    float open_percent;
    float full_open_percent;
    s64 text_pad;
} Console; 

/**
 * @Temporary: Perfoms a full load of a 'res/globals.vars' into an actual struct variables.
 */
void load_globals();

#endif
