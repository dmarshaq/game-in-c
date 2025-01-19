#ifndef CORE_H
#define CORE_H

/**
 * Debug.
 */

extern const char* debug_error_str;
extern const char* debug_warning_str;
extern const char* debug_ok_str;

/**
 * Integer typedefs.
 */

#include <stdint.h>

typedef int8_t      s8;
typedef uint8_t		u8;
typedef int16_t	    s16;
typedef uint16_t	u16;
typedef int32_t		s32;
typedef uint32_t	u32;
typedef int64_t     s64;
typedef uint64_t    u64;

#endif
