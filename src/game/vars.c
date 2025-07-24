#include "game/vars.h"

#include "game/game.h"

#include "core/core.h"
#include "core/str.h"
#include "core/file.h"
#include "core/structs.h"

#include <stddef.h>

// Since C doesn't have built in metaprogramming capabilities that would allow automatica tree generation from the in project structures it has to be done manually :( .


// This is incomplete and should be refactored cause tree building will be a little more complicated than that.
typedef struct vars_node {
    String name;
    void *data;
    union {
        s32 children_rptr; // Relative pointer. The actual pointer type is Node *. Used in actual tree.
        u32 children_index; // Children index is the index in the array list of children that specifies where node's children begin. Used only in tree builder.
    };
    u32 children_count;
} Vars_Node;

typedef struct vars_tree {
    u64 count; // Count of nodes, including the root.
    Vars_Node *root;
} Vars_Tree;

typedef struct vars_tree_builder {
    Vars_Node **data; // Dynamic array that holds references to dynamic arrays, to easily build trees.
    u32 current_tree_level;
    Allocator *allocator;
} Vars_Tree_Builder;


#define ABS2REL_32(rel_loc, abs_ptr)    rel_loc = ((s32)((u8 *)(abs_ptr) - (u8 *)&(rel_loc)))
#define REL2ABS_32(rel_loc)             ((void *)((u8 *)&(rel_loc) + rel_loc))

Vars_Tree_Builder vars_tree_builder_make(u64 initial_capacity, Allocator *allocator) {
    Vars_Tree_Builder builder = {
        .data = array_list_make(Vars_Node *, initial_capacity, allocator),
        .current_tree_level = 0,
        .allocator = allocator,
    };

    
    Vars_Node *level = array_list_make(Vars_Node, 4, builder.allocator);
    array_list_append(&builder.data, level);


    return builder;
}


void vars_tree_builder_add_field(Vars_Tree_Builder *builder, String name, void *field_ptr) {
    array_list_append(&(builder->data[builder->current_tree_level]), ((Vars_Node) {
                .name = name,
                .data = field_ptr,
                .children_index = 0,
                .children_count = 0,
                }));


    if (builder->current_tree_level > 0) {
        Vars_Node *parent_struct_node = builder->data[builder->current_tree_level - 1] + array_list_length(&builder->data[builder->current_tree_level - 1]) - 1; // Getting last (parent) struct node.
        parent_struct_node->children_count++;
    }
}

void vars_tree_builder_struct_begin(Vars_Tree_Builder *builder, String name, void *struct_ptr) {
    vars_tree_builder_add_field(builder, name, struct_ptr);

    Vars_Node *struct_node = builder->data[builder->current_tree_level] + array_list_length(&builder->data[builder->current_tree_level]) - 1; // Getting struct node that was just created.

    builder->current_tree_level++;

    // Creating a new level if it doesn't exist.
    if (array_list_length(&builder->data) == builder->current_tree_level) {
        Vars_Node *level = array_list_make(Vars_Node, 4, builder->allocator);
        array_list_append(&builder->data, level);
    }

    struct_node->children_index = array_list_length(&builder->data[builder->current_tree_level]);
}

void vars_tree_builder_struct_end(Vars_Tree_Builder *builder) {
    builder->current_tree_level--;
}


Vars_Tree vars_tree_builder_build(Vars_Tree_Builder *builder, Allocator *allocator) {
    u64 count = 1; // Starting at one because first node is always the root.
    u64 string_data_buffer_length = 0; // This value holds length of data needed to store all of the names info of vars nodes.
    for (u32 i = 0; i < array_list_length(&builder->data); i++) {
        count += array_list_length(&builder->data[i]);
        for (u32 j = 0; j < array_list_length(&builder->data[i]); j++) {
            string_data_buffer_length += builder->data[i][j].name.length;
        }
    }

    printf("BUILDER: Counted nodes: %lld, %lld.\n", count, string_data_buffer_length);

    Vars_Tree tree = {
        .count = count,
        .root = allocator_alloc(allocator, count * sizeof(Vars_Node) + string_data_buffer_length),
    };

    printf("BUILDER: Allocated tree.\n");

    // Pointers to the begginnings of the data segments.
    Vars_Node *tree_nodes = tree.root + 1; // + 1 because compiler knows tha tree.root is of underlying type Vars_Node, so it adds sizeof(Vars_Node) automatically.

    char *names_data = (char *)(tree.root + tree.count);


    printf("BUILDER: Got data pointers.\n");

    // Making tree root node here.
    tree.root[0].name = (String){0};
    tree.root[0].data = NULL;
    ABS2REL_32(tree.root[0].children_rptr, tree_nodes); // Saving abosulute pointer as an realtive pointer inside the array.

    // Checking if there are any nodes besides root node.
    if (tree.count > 1)
        tree.root[0].children_count = array_list_length(&builder->data[0]); // Array list at index 0 must exist if there are any nodes besides root.
    else
        tree.root[0].children_count = 0;

    printf("BUILDER: Made root.\n");




    u64 names_data_write_index = 0;
    count = 0; // Repurposing count variable to travers tree_nodes array.
    for (u32 i = 0; i < array_list_length(&builder->data); i++) {
        for (u32 j = 0; j < array_list_length(&builder->data[i]); j++) {
            // u32 counter_x = builder->data[i][j].children_count;
            // tree_nodes[count].name = builder->data[i][j].name;
            tree_nodes[count] = builder->data[i][j];
            printf("BUILDER: Copied node from array list.\n");


            
            if (tree_nodes[count].children_count > 0) {
                printf("BUILDER: Detected children nodes...");
// This comment is hard to read...
                // '(count + (array_list_length(&builder->data[i]) - j))' this expression should calculate where next layer ("children tree layer") should start, and from that we can just add
                // 'builder->data[i][j].children_index' to find the first node of the children array assigned to the current node copied.
                Vars_Node *children_abs_ptr = tree_nodes + (count + (array_list_length(&builder->data[i]) - j)) + builder->data[i][j].children_index;

                // Encoding this absolute pointer to relative pointer.
                ABS2REL_32(tree_nodes[count].children_rptr, children_abs_ptr);

                printf(" OK.\n");
            }

            printf("BUILDER: Copying string name...");
            // Copying names data.
            memcpy(names_data + names_data_write_index, builder->data[i][j].name.data, builder->data[i][j].name.length);
            tree_nodes[count].name.data = names_data + names_data_write_index;
            names_data_write_index += builder->data[i][j].name.length;
            printf(" OK.\n");

            count++;
        }

        array_list_free(&(builder->data[i]));
        printf("BUILDER: Freed arraylist layer.\n");
    }
    printf("BUILDER: Exited copy node loop.\n");
    array_list_free(&builder->data);
    printf("BUILDER: Freed arraylist for tree.\n");


    printf("BUILDER: Finished copying nodes.\n");

    return tree;
}

void vars_tree_free(Vars_Tree *tree, Allocator *allocator) {
    allocator_free(allocator, tree->root);
}


void vars_tree_print_node(Vars_Node *node, s64 depth) {
    for (s64 i = 0; i < depth; i++) {
        printf("  "); // Indent 2 spaces per level
    }

    if (node->name.length > 0) {
        printf("| %.*s\n", UNPACK(node->name));
    } else {
        printf("| (null)\n");
    }

    for (s64 i = 0; i < node->children_count; i++) {
        vars_tree_print_node((Vars_Node *)REL2ABS_32(node->children_rptr) + i, depth + 1);
    }
}













static Console console_data;



#define VT_BUILDER_STRUCT(builder_ptr, str, struct_ptr, code)\
    vars_tree_builder_struct_begin(builder_ptr, str, struct_ptr);\
    code\
    vars_tree_builder_struct_end(builder_ptr)

#define VT_BUILDER_FIELD(builder_ptr, str, field_ptr)   vars_tree_builder_add_field(builder_ptr, str, field_ptr)


Vars_Tree vars_tree_make() {
    Vars_Tree_Builder builder = vars_tree_builder_make(8, &std_allocator);

    // Build vars tree here.

    VT_BUILDER_STRUCT(&builder, CSTR("console_data"), &console_data,

            VT_BUILDER_FIELD(&builder, CSTR("speed"),               &console_data.speed);
            VT_BUILDER_FIELD(&builder, CSTR("open_percent"),        &console_data.open_percent);
            VT_BUILDER_FIELD(&builder, CSTR("full_open_percent"),   &console_data.full_open_percent);
            VT_BUILDER_FIELD(&builder, CSTR("text_pad"),            &console_data.text_pad);

            );



    return vars_tree_builder_build(&builder, &std_allocator);
}




void load_globals() {
    Vars_Tree tree = vars_tree_make();

    // Displaying tree.
    printf("Tree nodes count: %d\n", tree.count);
    vars_tree_print_node(tree.root, 0);




    String _content = read_file_into_str("res/globals.vars", &std_allocator);


    String content = _content;

    Vars_Node *current_node = tree.root;
    
    String found_field = {0};

    while (true) {
        // Eat until literal. Not including newline symbols.
        content = str_eat_spaces(content);
        if (content.length <= 0) break;

        // Check if comment.
        if (content.data[0] == '#') {
            s64 newline = str_find_char_left(content, '\n');
            
            String comment = str_substring(content, 0, newline);
            printf("%-10s:    %.*s\n", "Comment", UNPACK(comment));

            content = str_eat_chars(content, comment.length);
            continue;
        }

        // Check if vars tree node path
        if (content.data[0] == '[') {


            s64 end_of_path = str_find_char_left(content, ']');
            if (end_of_path == -1) {
                printf_err("Couldn't parse '%s': Missing ']'.\n", "res/globals.vars");
                exit(1);
            }
            
            String path = str_substring(content, 1, end_of_path);
            printf("%-10s:    %.*s\n", "Path", UNPACK(path));

            content = str_eat_chars(content, path.length + 2);
            
            // Parsing path.
            current_node = tree.root;
            while (true) {
                if (path.length <= 0) break;

                s64 end_of_node_name = str_find_char_left(path, '.');
                if (end_of_node_name == -1)
                    end_of_node_name = path.length;

                String node_name = str_substring(path, 0, end_of_node_name);
                printf("    %-10s:    %.*s\n", "Node name", UNPACK(node_name));

                // Searching node in the tree.
                Vars_Node *children = REL2ABS_32(current_node->children_rptr);
                for (u32 i = 0; i < current_node->children_count; i++) {
                    if (str_equals(children[i].name, node_name)) {
                        current_node = children + i;
                        break;
                    }

                    if (i + 1 == current_node->children_count) {
                        printf_err("Couldn't parse '%s': No vars node named: '%.*s'.\n", "res/globals.vars", UNPACK(node_name));
                        exit(1);
                    }
                }
                
                path = str_eat_chars(path, node_name.length + 1);
            }



            continue;
        }
        

        // Default case.
        String literal = str_get_until_space(content);

        content = str_eat_chars(content, literal.length);

        if (found_field.length == 0 ) {
            if (!str_is_int(literal) && !str_is_float(literal)) {
                found_field = literal; // Basically any new literal which is not a number could be counted as a new field if other field was not found before.
                printf("%-10s:    %.*s\n", "Key", UNPACK(literal));
            }

        }
        else {
            if (str_is_int(literal)) {
                
                // Searching node in the tree.
                Vars_Node *children = REL2ABS_32(current_node->children_rptr);
                for (u32 i = 0; i < current_node->children_count; i++) {
                    if (str_equals(children[i].name, found_field)) {
                        *(s64 *)children[i].data = str_parse_int(literal);
                        break;
                    }

                    if (i + 1 == current_node->children_count) {
                        printf_err("Couldn't parse '%s': No field node named: '%.*s'.\n", "res/globals.vars", UNPACK(found_field));
                        exit(1);
                    }
                }

                found_field = (String){0};

                printf("%-10s:    %.*s\n", "Integer", UNPACK(literal));
                continue;
            }

            if (str_is_float(literal)) {

                Vars_Node *children = REL2ABS_32(current_node->children_rptr);
                for (u32 i = 0; i < current_node->children_count; i++) {
                    if (str_equals(children[i].name, found_field)) {
                        *(float *)children[i].data = str_parse_float(literal);
                        break;
                    }

                    if (i + 1 == current_node->children_count) {
                        printf_err("Couldn't parse '%s': No field node named: '%.*s'.\n", "res/globals.vars", UNPACK(found_field));
                        exit(1);
                    }
                }

                found_field = (String){0};

                printf("%-10s:    %.*s\n", "Float", UNPACK(literal));
                continue;
            }
            
            printf_err("Couldn't parse '%s': '%.*s' is not a valid field value.\n", "res/globals.vars", UNPACK(literal));
            exit(1);
        }
    }


    allocator_free(&std_allocator, _content.data);



    printf("console_data from code:\n");
    printf("speed = %d\n", console_data.speed);
    printf("open_percent = %f\n", console_data.open_percent);
    printf("full_open_percent = %f\n", console_data.full_open_percent);
    printf("text_pad = %d\n", console_data.text_pad);
}
