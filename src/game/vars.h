#ifndef VARS_H
#define VARS_H

#include "core/type.h"
#include "core/typeinfo.h"
#include "core/str.h"



typedef struct vars_node {
    String name;

    Type_Info *type;
    void *data;

    union {
        s32 children_rptr; // Relative pointer. The actual pointer type is Node *. Used in actual tree.
        u32 children_index; // Children index is the index in the array list of children that specifies where node's children begin. Used only in tree builder.
    };
    u32 children_count;
} Vars_Node;

typedef struct vars_tree {
    u64 count;
    Vars_Node *root;
} Vars_Tree;

#define ABS2REL_32(rel_loc, abs_ptr)    rel_loc = ((s32)((u8 *)(abs_ptr) - (u8 *)&(rel_loc)))
#define REL2ABS_32(rel_loc)             ((void *)((u8 *)&(rel_loc) + rel_loc))



/**
 * This function begins building of the vars tree.
 * @Important: All 'vars_tree_add(...)' calls should happen between vars_tree_being and vars_tree_build.
 */
void vars_tree_begin();

void vars_tree_add(Type_Info *type, u8 *data, String var_name);

Vars_Tree vars_tree_build();

void vars_tree_print_node(Vars_Node *node, s64 depth);


void load_vars_file(char *file_name, Vars_Tree *tree);

/**
 * This function insures .vars files are listened to and properly loaded once any of those files are changed.
 */
void vars_listen_to_changes();

#endif
