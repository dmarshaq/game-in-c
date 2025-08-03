#ifndef TYPE_INFO_H
#define TYPE_INFO_H

/**
 * Type Info's.
 */

#include "core/type.h"
#include "core/str.h"

#include <stdbool.h>

typedef struct type_info Type_Info;

typedef enum : u8 {
    INTEGER,
    FLOAT,
    BOOL,
    POINTER,
    FUNCTION,
    VOID,
    STRUCT,
    ARRAY,
    ENUM,
    TYPEDEF,
    UNKNOWN,
} Type_Info_Kind;

typedef struct {
    u32 size_bits;
    bool is_signed;
} Type_Info_Integer;

typedef struct {
    u32 size_bits;
} Type_Info_Float;

typedef struct {
    Type_Info *ptr_to;
} Type_Info_Pointer;


typedef struct {
    Type_Info *type;
    String name;
} Type_Info_Function_Argument;

typedef struct {
    Type_Info *return_type;

    String name;

    u32 arguments_length;
    Type_Info_Function_Argument *arguments;
} Type_Info_Function;


typedef struct {
    Type_Info *type;

    String name;

    u32 offset;
} Type_Info_Struct_Member;

typedef struct {
    String name;

    u32 members_length;
    Type_Info_Struct_Member *members;

} Type_Info_Struct;

typedef struct {
    Type_Info *element_type;
    u64 length;
} Type_Info_Array;

// @Todo: enum.
typedef struct {

} Type_Info_Enum;

typedef struct {
    Type_Info *typedef_of;
} Type_Info_Typedef;

struct type_info {
    Type_Info_Kind type;
    u32 size;
    u32 align;
    
    union {
        Type_Info_Integer   t_integer;
        Type_Info_Float     t_float;
        Type_Info_Pointer   t_pointer;
        Type_Info_Function  t_function;
        Type_Info_Struct    t_struct;
        Type_Info_Array     t_array;
        Type_Info_Enum      t_enum;
        Type_Info_Typedef   t_typedef;
    };
};

static void print_type_info(Type_Info *type) {
    printf("size: %u, align: %u ,", type->size, type->align);
    switch (type->type) {
        case INTEGER:
            printf("INTEGER: size_bits: %d, %s\n", type->t_integer.size_bits, type->t_integer.is_signed ? "signed" : "unsigned");
            break;
        case FLOAT:
            printf("FLOAT: size_bits: %d\n", type->t_float.size_bits);
            break;
        case BOOL:
            printf("BOOL\n");
            break;
        case POINTER:
            printf("POINTER to ");
            print_type_info(type->t_pointer.ptr_to);
            break;
        case FUNCTION:
            printf("FUNCTION: '%.*s'\n", UNPACK(type->t_function.name));
            for (u32 i = 0; i < type->t_function.arguments_length; i++) {
                printf("    arg[%u] '%.*s': ", i, UNPACK(type->t_function.arguments[i].name));
                print_type_info(type->t_function.arguments[i].type);
            }
            printf("\n -> returns: ");
            print_type_info(type->t_function.return_type);
            break;
        case VOID:
            printf("VOID\n");
            break;
        case STRUCT:
            printf("STRUCT: '%.*s'\n", UNPACK(type->t_struct.name));
            for (u32 i = 0; i < type->t_struct.members_length; i++) {
                printf("    field[%u] '%.*s': ", i, UNPACK(type->t_struct.members[i].name));
                print_type_info(type->t_struct.members[i].type);
            }
            break;
        case ARRAY:
            break;
        case ENUM:
            break;
        case TYPEDEF:
            printf("TYPEDEF of ");
            print_type_info(type->t_typedef.typedef_of);
            break;
        case UNKNOWN:
            printf("UNKNOWN\n");
            break;
    }
}

static Type_Info *get_base_of_typedef(Type_Info *type) {
    while (type->type == TYPEDEF) {
        type = type->t_typedef.typedef_of;
    }

    return type;
}

#endif
