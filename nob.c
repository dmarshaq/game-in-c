#define DEV


// Defining flags.
#ifdef DEV
#   define nob_cc_flags(cmd)    nob_cmd_append(cmd, "-std=gnu11", "-g", "-O0", "-DDEBUG")
#else
#   define nob_cc_flags(cmd)    nob_cmd_append(cmd, "-std=gnu11", "-O2", "-DNDEBUG")
#endif // DEV


// Defining libs.
#define nob_cc_libs(cmd)    nob_cmd_append(cmd, "-lmingw32", "-lSDL2main", "-lSDL2", "-lSDL2_mixer", "-lopengl32", "-lglew32")



// Defining project paths.
#define SRC_DIR "src"
#define OBJ_DIR "obj"
#define BIN_DIR "bin"

#define CORE_SRC_DIR SRC_DIR"/core"
#define META_SRC_DIR SRC_DIR"/meta"
#define GAME_SRC_DIR SRC_DIR"/game"
#define GAME_M_SRC_DIR SRC_DIR"/game_m"


// Defining includes path.
#define nob_cc_includes(cmd)    nob_cmd_append(cmd, "-I"SRC_DIR)




#define NOB_IMPLEMENTATION
#include "nob.h"





// Making a buffer to save important strings.
#define STRINGS_BUFFER_CAPACITY 2048
static char strings_buffer[STRINGS_BUFFER_CAPACITY];
static char *strings_buffer_write = strings_buffer;

char *save_string(char *str) {
    if (strings_buffer_write + strlen(str) - strings_buffer > STRINGS_BUFFER_CAPACITY) return NULL;

    char *result = memcpy(strings_buffer_write, str, strlen(str) + 1);
    strings_buffer_write = result + strlen(str) + 1;
    return result;
}

void reset_saved_strings() {
    strings_buffer_write = strings_buffer;
}






bool nob_cmd_append_all_in_dir(Nob_Cmd *cmd, const char *directory, const char *file_format) {
    // Finding specific paths variables.
    Nob_File_Paths paths = {0};
    char file_path_buffer[strlen(directory) + 1 + 128];

    strcpy(file_path_buffer, directory);
    strcat(file_path_buffer, "/");

    if (!nob_read_entire_dir(directory, &paths)) return false;

    for (size_t i = 0; i < paths.count; i++) {
        // Everytime I am doing null terminated strings I want to jump off a bridge... into the ocean water...
        strncat(file_path_buffer, paths.items[i], 128);

        // Taking only .c files.
        if (nob_get_file_type(file_path_buffer) == NOB_FILE_REGULAR && strcmp(strrchr(file_path_buffer, '.'), file_format) == 0) {
            
            char *saved = save_string(file_path_buffer);
            if (saved == NULL) {
                nob_log(NOB_ERROR, "Couldn't save string, not enough space.");
                return false;
            }

            nob_cmd_append(cmd, saved);
        }

        file_path_buffer[strlen(directory) + 1] = '\0';
    }

    return true;
}


/**
 * @Important: Assumes directory path starts with 'src'.
 */
bool nob_cc_compile_objs(Nob_Cmd *cmd, const char *directory) {
    // Finding specific paths variables.
    Nob_File_Paths paths = {0};
    char file_path_buffer[strlen(directory) + 1 + 128];

    strcpy(file_path_buffer, directory);
    strcat(file_path_buffer, "/");


    char obj_out_path_buffer[strlen(file_path_buffer) - 3 + strlen(OBJ_DIR) + 128];

    strcpy(obj_out_path_buffer, OBJ_DIR);
    strcat(obj_out_path_buffer, file_path_buffer + 3);


    if (!nob_read_entire_dir(directory, &paths)) return false;

    for (size_t i = 0; i < paths.count; i++) {
        // Everytime I am doing null terminated strings I want to jump off a bridge... into the ocean water...
        strncat(file_path_buffer,    paths.items[i], 128);

        // Taking only .c files.
        if (nob_get_file_type(file_path_buffer) == NOB_FILE_REGULAR && strcmp(strrchr(file_path_buffer, '.'), ".c") == 0) {
            
            strncat(obj_out_path_buffer, paths.items[i], 128);
            obj_out_path_buffer[strlen(obj_out_path_buffer) - 1] = 'o';

            nob_cc(cmd);
            nob_cc_flags(cmd);
            nob_cmd_append(cmd, "-c", file_path_buffer, "-o", obj_out_path_buffer);
            nob_cc_includes(cmd);

            if (!nob_cmd_run_sync_and_reset(cmd)) return 1;

            obj_out_path_buffer[strlen(directory) + 1 - 3 + strlen(OBJ_DIR)] = '\0';
        }

        file_path_buffer[strlen(directory) + 1] = '\0';
    }

    return true;
}




/**
 * @Important: Build nob one time and run it.
 *
 *      $ cc -o nob nob.c
 *      $ ./nob
 *
 * Then just run it like usually, thanks to NOB_GO_REBUILD_URSELF.
 *
 *      $ ./nob
 *
 */
int main(int argc, char **argv) {
    NOB_GO_REBUILD_URSELF(argc, argv);

    Nob_Cmd cmd = {0};


    // Cleaning.
    nob_cmd_append(&cmd, "rm", "-f", OBJ_DIR"/core/*.o");
    if (!nob_cmd_run_sync_and_reset(&cmd)) return 1;

    nob_cmd_append(&cmd, "rm", "-f", OBJ_DIR"/*.o");
    if (!nob_cmd_run_sync_and_reset(&cmd)) return 1;

    nob_cmd_append(&cmd, "rm", "-f", BIN_DIR"/*.a");
    if (!nob_cmd_run_sync_and_reset(&cmd)) return 1;

    nob_cmd_append(&cmd, "rm", "-f", BIN_DIR"/*.dll");
    if (!nob_cmd_run_sync_and_reset(&cmd)) return 1;

    nob_cmd_append(&cmd, "rm", "-f", BIN_DIR"/*.exe");
    if (!nob_cmd_run_sync_and_reset(&cmd)) return 1;

    // Compile all obj's.
    nob_cc_compile_objs(&cmd, CORE_SRC_DIR);

    // Link all obj's into static lib.
    nob_cmd_append(&cmd, "ar", "rcs", BIN_DIR"/libcore.a");
    nob_cmd_append_all_in_dir(&cmd, OBJ_DIR"/core", ".o");

    if (!nob_cmd_run_sync_and_reset(&cmd)) return 1;
    reset_saved_strings();


    // Building meta.exe
    nob_cc(&cmd);
    nob_cc_flags(&cmd);
    nob_cc_output(&cmd, BIN_DIR"/meta.exe");
    nob_cc_includes(&cmd);
    nob_cmd_append_all_in_dir(&cmd, META_SRC_DIR, ".c");
    nob_cmd_append(&cmd, "-L"BIN_DIR, "-lcore");

    if (!nob_cmd_run_sync_and_reset(&cmd)) return 1;
    reset_saved_strings();


    // Running meta.exe
    nob_cmd_append(&cmd, BIN_DIR"/meta.exe");
    nob_cmd_append_all_in_dir(&cmd, GAME_M_SRC_DIR, ".c");
    nob_cmd_append_all_in_dir(&cmd, GAME_M_SRC_DIR, ".h");

    if (!nob_cmd_run_sync_and_reset(&cmd)) return 1;
    reset_saved_strings();


    // Building main.exe
    nob_cc(&cmd);
    nob_cc_flags(&cmd);
    nob_cc_output(&cmd, BIN_DIR"/main.exe");
    nob_cc_includes(&cmd);
    nob_cmd_append(&cmd, SRC_DIR"/meta_generated.c");
    nob_cmd_append_all_in_dir(&cmd, GAME_SRC_DIR, ".c");
    nob_cmd_append(&cmd, "-L"BIN_DIR, "-lcore");
    nob_cc_libs(&cmd);

    if (!nob_cmd_run_sync_and_reset(&cmd)) return 1;
    reset_saved_strings();

    return 0;
}

