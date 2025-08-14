#ifndef COMMAND_H
#define COMMAND_H

#include "core/str.h"
#include "core/typeinfo.h"

typedef struct command {
    Type_Info *type;
    void (*func)(Any *, u32);
} Command;

#define COMMAND_PREFIX(name)    _command_##name

/**
 * Inits command handler, should be called once before any commands are registered.
 */
void command_init();

/**
 *  Registers command, that then can be called in the console.
 * 'type' must be a type info of an actual function.
 * 'ptr' is a pointer to the wrapper of an actual function.
 */
void command_register(Type_Info *type, void (*ptr)(Any *, u32));

/**
 * Parses 'command' String specified into an actual command call and then runs it.
 */
void command_run(String command);

/**
 * Prints information about the specific command.
 */
void command_print(Command *command);







#endif
