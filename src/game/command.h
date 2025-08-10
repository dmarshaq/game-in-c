#ifndef COMMAND_H
#define COMMAND_H

#include "core/str.h"
#include "core/typeinfo.h"

typedef struct command {
    Type_Info *type;
    void (*func)(Any *, u32);
} Command;

#define COMMAND_PREFIX(name)    _command_##name

void command_init();

/**
 * 'type' must be a type info of an actual function.
 * 'ptr' is a pointer to the wrapper of an actual function.
 */
void command_register(Type_Info *type, void (*ptr)(Any *, u32));

void command_run(String command);

void command_print(Command *command);







#endif
