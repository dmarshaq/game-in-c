#include "game/vars.h"

#include "meta_generated.h"

#include "game/game.h"
#include "game/asset.h"

#include "core/core.h"
#include "core/str.h"
#include "core/file.h"
#include "core/structs.h"

#include <stddef.h>


typedef struct vars_tree_builder {
    Vars_Node **data; // Dynamic array that holds references to dynamic arrays, to easily build trees.
    u32 current_tree_level;
    Allocator *allocator;
} Vars_Tree_Builder;


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


void vars_tree_builder_add_field(Vars_Tree_Builder *builder, String name, Type_Info *type, void *field_ptr) {
    array_list_append(&(builder->data[builder->current_tree_level]), ((Vars_Node) {
                .name = name,
                .type = type,
                .data = field_ptr,
                .children_index = 0,
                .children_count = 0,
                }));


    if (builder->current_tree_level > 0) {
        Vars_Node *parent_struct_node = builder->data[builder->current_tree_level - 1] + array_list_length(&builder->data[builder->current_tree_level - 1]) - 1; // Getting last (parent) struct node.
        parent_struct_node->children_count++;
    }
}

void vars_tree_builder_struct_begin(Vars_Tree_Builder *builder, String name, Type_Info *type, void *struct_ptr) {
    vars_tree_builder_add_field(builder, name, type, struct_ptr);

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

/**
 * @Recursion: Adds all of the struct fields, including nested structs that where properly introspected.
 */
void vars_tree_builder_add_struct(Vars_Tree_Builder *builder, Type_Info *type, u8 *data, String var_name) {
    if (type->type != STRUCT) {
        printf_err("Can't add non STRUCT type to vars.\n");
        return;
    }


    vars_tree_builder_struct_begin(builder, var_name, type, (void *)data);

    for (u32 i = 0; i < type->t_struct.members_length; i++) {
        // Wrapping around get_base_of_typedef() because it properly sanitizes the member type in case if it is a typedef in other case it just returns the original member type.
        Type_Info *member_type = get_base_of_typedef(type->t_struct.members[i].type);

        // Handling nested struct case.
        if (member_type->type == STRUCT) {
            vars_tree_builder_add_struct(builder, member_type, data + type->t_struct.members[i].offset, type->t_struct.members[i].name);
            continue;
        }
        
        // If not struct we just assume it is a properly defined field.
        vars_tree_builder_add_field(builder, type->t_struct.members[i].name, member_type, data + type->t_struct.members[i].offset);
    }

    vars_tree_builder_struct_end(builder);
}























@Introspect;
typedef struct color {
    float red;
    float green;
    float blue;
    float alpha;
} Color;

@Introspect;
typedef struct console {
    s64 speed;
    float open_percent;
    Color bg;
    Color text_input;
    float full_open_percent;
    s64 text_pad;
} Console;

static Console console_data;






static Vars_Tree_Builder vt_builder;


void vars_tree_begin() {
    vt_builder = vars_tree_builder_make(8, &std_allocator);
}

void vars_tree_add(Type_Info *type, u8 *data, String var_name) {
    vars_tree_builder_add_struct(&vt_builder, type, data, var_name);
}

Vars_Tree vars_tree_build() {
    return vars_tree_builder_build(&vt_builder, &std_allocator);
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




void load_vars_file(char *file_name, Vars_Tree *tree) {
    String _content = read_file_into_str(file_name, &std_allocator);

    String content = _content;

    Vars_Node *current_node = tree->root;
    Vars_Node *current_key = NULL;
    
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
                printf_err("Couldn't parse '%s': Missing ']'.\n", file_name);
                return;
            }
            
            String path = str_substring(content, 1, end_of_path);
            printf("%-10s:    %.*s\n", "Path", UNPACK(path));

            content = str_eat_chars(content, path.length + 2);
            
            // Parsing path.
            current_node = tree->root;
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
                        printf_err("Couldn't parse '%s': No vars node named: '%.*s'.\n", file_name, UNPACK(node_name));
                        return;
                    }
                }
                
                path = str_eat_chars(path, node_name.length + 1);
            }



            continue;
        }
        

        // Default case.
        String literal = str_get_until_space(content);

        content = str_eat_chars(content, literal.length);

        // Check if literal is a key.
        if (current_key == NULL) {
            if (str_is_symbol(literal)) {

                // Searching node in the tree.
                Vars_Node *children = REL2ABS_32(current_node->children_rptr);
                for (u32 i = 0; i < current_node->children_count; i++) {
                    if (str_equals(children[i].name, literal)) {
                        current_key = children + i;
                        break;
                    }

                    if (i + 1 == current_node->children_count) {
                        printf_err("Couldn't parse '%s': No field node named: '%.*s'.\n", file_name, UNPACK(literal));
                        return;
                    }
                }

                printf("%-10s:    %.*s\n", "Key", UNPACK(literal));

                continue;
            }

            printf_err("Couldn't parse '%s': File has no key specified for the literal: '%.*s'.\n", file_name, UNPACK(literal));
            return;
        }


        // Parsing by type.
        Type_Info *type = current_key->type;
        switch (type->type) {
            case INTEGER:
                if (!str_is_int(literal)) {
                    printf_err("Couldn't parse '%s': Expected literal '%.*s' to be INTEGER.\n", file_name, UNPACK(literal));
                    return;
                }

                
                // Check if it is a valid integer type.
                if (type->t_integer.size_bits != 64 || !type->t_integer.is_signed) {
                    printf_err("Error '%s': Expected '%.*s' field node to be signed 64 bit integer.\n", file_name, UNPACK(literal));
                    return;
                }

                printf("%-10s:    %.*s\n", "Integer", UNPACK(literal));

                *(s64 *)current_key->data = str_parse_int(literal);

                current_key = NULL;
                break;
            case FLOAT:
                if (!str_is_float(literal)) {
                    printf_err("Couldn't parse '%s': Expected literal '%.*s' to be FLOAT.\n", file_name, UNPACK(literal));
                    return;
                }

                printf("%-10s:    %.*s\n", "Float", UNPACK(literal));
                
                *(float *)current_key->data = str_parse_float(literal);

                current_key = NULL;

                break;
            case BOOL:

                current_key = NULL;
                break;
        }
    }


    allocator_free(&std_allocator, _content.data);
}




void vars_listen_to_changes() {
    u32 count;
    const Asset_Change *changes;

    if (view_asset_changes(&count, &changes)) {
        for (u32 i = 0; i < count; i++) {
            printf("Vars listened to an Asset Change: '%.*s'\n", UNPACK(changes[i].file_name));
        }
    }








    // printf("console_data from code:\n");
    // printf("speed = %d\n", console_data.speed);
    // printf("open_percent = %f\n", console_data.open_percent);
    // printf("full_open_percent = %f\n", console_data.full_open_percent);
    // printf("text_pad = %d\n", console_data.text_pad);


    // printf("console_data.text_input from code:\n");
    // printf("red     = %f\n", console_data.text_input.red);
    // printf("green   = %f\n", console_data.text_input.green);
    // printf("blue    = %f\n", console_data.text_input.blue);
    // printf("alpha   = %f\n", console_data.text_input.alpha);


    // printf("console_data.bg from code:\n");
    // printf("red     = %f\n", console_data.bg.red);
    // printf("green   = %f\n", console_data.bg.green);
    // printf("blue    = %f\n", console_data.bg.blue);
    // printf("alpha   = %f\n", console_data.bg.alpha);
}
