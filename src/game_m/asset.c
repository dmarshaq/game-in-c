#include "game/asset.h"

#include "core/str.h"
#include "core/structs.h"
#include "core/type.h"

static Asset_Change *asset_changes_list;


int asset_observer_init(char *directory) {
    asset_changes_list = array_list_make(Asset_Change, 8, &std_allocator);

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



#if OS == WINDOWS

#include <windows.h>

int asset_observer_poll_changes() {
    TODO("Windows implementation of asset_observer_poll_changes()");
    return 0;
}


#endif
