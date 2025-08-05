#ifndef ASSET_H
#define ASSET_H


#include "core/str.h"


typedef struct asset_change {
    String file_name;
} Asset_Change;


/**
 * Specified directory path would be recursivly monitored for any changes made.
 * Returns 0 if initted correctly.
 */
int asset_observer_init(char *directory);


/**
 * Collects all of the changes, that can be viewed through view_asset_changes(...) function.
 * Ideally be called only once per frame.
 * Can be used in hotreloading or be used to dynamically poll changes every frame.
 * Returns 0 if successful.
 */
int asset_observer_poll_changes();


/**
 * Returns true if there are any assets changed, otherwise returns false.
 * Outputs count of the assets changed in the count variable, and outputs pointer to the changes buffer in changes variable.
 */
bool view_asset_changes(u32 *count, const Asset_Change **changes);







#endif
