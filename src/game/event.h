#ifndef EVENT_H
#define EVENT_H

#include "core/core.h"
#include "core/mathf.h"

#include "game/plug.h"

#include <stdbool.h>


void init_events_handler(Events_Info *events);

void handle_events(Events_Info *events, Window_Info *window, Time_Data *t);

#endif
