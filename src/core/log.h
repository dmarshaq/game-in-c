#ifndef LOG_H
#define LOG_H

#include <stdio.h>

#include "core/type.h"

typedef enum log_level : u8 {
    LOG_LEVEL_INFO    = 0,
    LOG_LEVEL_WARNING = 1,
    LOG_LEVEL_ERROR   = 2,
} Log_Level;

/**
 * This list of defines dictates what information included in the logging of the program.
 * It is not runtime flags, because logs should run fast.
 * If you don't want some information displayed, just comment out define macros.
 */
#define LOG_INFO_COLOR
#define LOG_INFO_TIME
#define LOG_INFO_THREAD
#define LOG_INFO_FILE
#define LOG_INFO_LINE
#define LOG_INFO_FUNC


#define LOG_INFO(format, ...)       log_print(LOG_LEVEL_INFO, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)
#define LOG_WARNING(format, ...)    log_print(LOG_LEVEL_WARNING, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)
#define LOG_ERROR(format, ...)      log_print(LOG_LEVEL_ERROR, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)


/**
 * Sets minimum log level, if log level is below minimum it is not displayed.
 */
void log_set_minimum_level(Log_Level level);

/**
 * Sets log output. If output is not set, all logs default to stderr.
 */
void log_set_output(FILE *stream);

/**
 * Prints and formats log.
 */
void log_print(Log_Level level, const char *file_name, s64 line, const char *function_name, char *format, ...);

#endif
