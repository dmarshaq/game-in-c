#ifndef PLUG_H_
#define PLUG_H_

#include "core.h"

typedef struct {
    bool quit;
    Time_Data *t;
} Plug_State;

typedef void (*Plug_Init)(Plug_State *state);
typedef void (*Plug_Update)(Plug_State *state);

#endif
