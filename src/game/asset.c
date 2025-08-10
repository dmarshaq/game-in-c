#include "game/asset.h"

#include "core/arena.h"
#include "core/str.h"
#include "core/structs.h"
#include "core/type.h"

// Interfacing for internal function.
int asset_observer_start_watching(char *directory);


static Asset_Change *asset_changes_list;

int asset_observer_init(char *directory) {
    asset_changes_list = array_list_make(Asset_Change, 8, &std_allocator);

    // Is not inlined because it is more readable this way.
    if (asset_observer_start_watching(directory) != 0) {
        return 1;
    }

    return 0;
}



bool view_asset_changes(u32 *count, const Asset_Change **changes) {
    *count = array_list_length(&asset_changes_list);

    if (count > 0) {
        *changes = asset_changes_list;
        return true;
    }

    *changes = NULL;
    return false;
}




/**
 * WINDOWS specific code.
 */
#if OS == WINDOWS

#include <windows.h>


static Arena filenames_arena;


static HANDLE handle;
static OVERLAPPED overlapped;
static bool watching;
static String dir_path;


static u8 buffer[8*KB];




int asset_observer_start_watching(char *directory) {
    // Init arena for filenames retrived from windows NOTIFICATIONS.
    filenames_arena = arena_make(2*KB);

    // Init other static vars.
    handle = INVALID_HANDLE_VALUE;
    overlapped = (OVERLAPPED) {0};
    watching = false;
    dir_path = CSTR(directory);

    // Convert UTF-8 path to wide string.
    // @Safety: Maybe add additional check to see if length of directory is of right size.
    u16 path[KB];
    int written = MultiByteToWideChar(CP_UTF8, 0, directory, -1, path, KB);
    
    if (written <= 0) {
        printf_err("Couldn't multiply UTF8 directory path to fit UTF16 path format when starting asset watching.\n");
        return -1;
    }


    handle = CreateFileW(
            path,
            FILE_LIST_DIRECTORY,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            NULL,
            OPEN_EXISTING,
            FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
            NULL
            );

    if (handle == INVALID_HANDLE_VALUE) {
        printf_err("Failed to open directory. Error code: %lu\n", GetLastError());
        return -1;
    }

    overlapped.hEvent = CreateEventA(NULL, TRUE, FALSE, NULL);

    watching = ReadDirectoryChangesW(
            handle,
            buffer,
            sizeof(buffer),
            TRUE,
            FILE_NOTIFY_CHANGE_FILE_NAME |
            FILE_NOTIFY_CHANGE_DIR_NAME |
            FILE_NOTIFY_CHANGE_ATTRIBUTES |
            FILE_NOTIFY_CHANGE_SIZE |
            FILE_NOTIFY_CHANGE_LAST_WRITE |
            FILE_NOTIFY_CHANGE_CREATION,
            NULL,
            &overlapped,
            NULL
            );

    if (!watching && GetLastError() != ERROR_IO_PENDING) {
        printf_err("Initial ReadDirectoryChangesW when starting watching failed. Error: %lu\n", GetLastError());
        CloseHandle(handle);
        handle = INVALID_HANDLE_VALUE;
        return -1;
    }

    return 0;
}

// @Recursion.
int asset_observer_force_changes_directory(String directory) {

    // Making string be null terminated and have wildcard '*' at the end.
    char _buffer[directory.length + 3]; 
    str_copy_to(directory, _buffer);
    _buffer[directory.length - 2] = '/';
    _buffer[directory.length - 1] = '*';
    _buffer[directory.length]     = '\0';



    HANDLE found_file_handle;
    WIN32_FIND_DATA found_file_data;
    found_file_handle = FindFirstFile(_buffer, &found_file_data);

    if (found_file_handle == INVALID_HANDLE_VALUE) {
        printf_err("Couldn't get handle to the found file in the directory '%.*s'.\n", UNPACK(directory));
        return -1;
    }

    do {
        printf("Found file: '%s'\n", found_file_data.cFileName);
    } while (FindNextFile(found_file_handle, &found_file_data) != 0);

    return 0;
}

int asset_observer_force_changes() {
    array_list_clear(&asset_changes_list);
    arena_clear(&filenames_arena);

    return asset_observer_force_changes_directory(dir_path);
}

int asset_observer_poll_changes() {
    array_list_clear(&asset_changes_list);
    arena_clear(&filenames_arena);

    if (!watching) return 0;


    DWORD bytes_transferred = 0;
    bool ready = GetOverlappedResult(handle, &overlapped, &bytes_transferred, FALSE);

    if (!ready) {
        u32 err = GetLastError();
        if (err == ERROR_IO_INCOMPLETE) {
            return 0; // No changes yet.
        }
        printf_err("GetOverlappedResult failed while observing asset changes. Error code: %lu\n", err);
        watching = false;
        return -1;
    }

    // Parse notifications

    FILE_NOTIFY_INFORMATION* info = (FILE_NOTIFY_INFORMATION*)buffer;
    while (true) {
        s64 utf8_notification_file_name_length = info->FileNameLength / sizeof(u16);

        // Building full_path for Asset Change struct.
        String full_path = {
            .length = dir_path.length, // Length is increased later.
            .data = arena_alloc(&filenames_arena, dir_path.length),
        };
        
        // Copying dir_path.
        str_copy_to(dir_path, full_path.data);

        // Adding '/'.
        char *slash_char_p = arena_alloc(&filenames_arena, 1);
        *slash_char_p = '/';
        full_path.length += 1;


        // Copying and converting notification file name from UTF-16 to UTF-8.
        int written = WideCharToMultiByte(CP_UTF8, 0, info->FileName, utf8_notification_file_name_length, filenames_arena.ptr, filenames_arena.capacity - arena_size(&filenames_arena), NULL, NULL); // This allows to freerly use rest of the arena space to properly copy path.
        
        // Checking if WideCharToMultiByte is correctly executed.
        if (written <= 0) {
            printf_err("Couldn't write to notification file path to arena.\n");
            return -1;
        }

        arena_alloc(&filenames_arena, written);
        full_path.length += written;
        
        // Replacing all of the '\' with '/'.
        for (s64 i = 0; i < full_path.length; i++) {
            if (full_path.data[i] == '\\') {
                full_path.data[i] = '/';
            }
        }

        String file_name = str_substring(full_path, str_find_char_right(full_path, '/') + 1, full_path.length);

        String file_format = str_substring(full_path, str_find_char_right(full_path, '.') + 1, full_path.length);



        switch (info->Action) {
            case FILE_ACTION_ADDED:
                break;
            case FILE_ACTION_REMOVED:
                break;
            case FILE_ACTION_MODIFIED:
                array_list_append(&asset_changes_list, ((Asset_Change) { 
                    .full_path      = full_path,
                    .file_name      = file_name,
                    .file_format    = file_format,
                            }));
                break;
            case FILE_ACTION_RENAMED_OLD_NAME:
                break;
            case FILE_ACTION_RENAMED_NEW_NAME:
                break;
            default:
                printf_err("Asset unknown change. '%.*s', action change code: %d\n", UNPACK(full_path), info->Action); 
                break;
        }


        // Break if no entries left
        if (info->NextEntryOffset == 0) 
            break;

        // Get next entry.
        info = (FILE_NOTIFY_INFORMATION*)((u8*)info + info->NextEntryOffset);
    }

    // Restart watcher
    ResetEvent(overlapped.hEvent);
    watching = ReadDirectoryChangesW(
            handle,
            buffer,
            sizeof(buffer),
            TRUE,
            FILE_NOTIFY_CHANGE_FILE_NAME |
            FILE_NOTIFY_CHANGE_DIR_NAME |
            FILE_NOTIFY_CHANGE_ATTRIBUTES |
            FILE_NOTIFY_CHANGE_SIZE |
            FILE_NOTIFY_CHANGE_LAST_WRITE |
            FILE_NOTIFY_CHANGE_CREATION,
            NULL,
            &overlapped,
            NULL
            );

    if (!watching && GetLastError() != ERROR_IO_PENDING) {
        printf("ReadDirectoryChangesW restart failed when observing asset changes. Error: %lu\n", GetLastError());
        watching = false;
        return -1;
    } 

    return 0;
}


#endif
