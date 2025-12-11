#ifndef __LOGGER_H__
#define __LOGGER_H__

#include <time.h>
#include "selector.h"
#include "defines.h"

void logger_init();

int logger_register_selector(FdSelector selector);

void logger_log_message_deferred(int level, const char* file, int line, time_t * now, const char* msg, ...);

void flush_all_logs();

int logger_get_min_level();

void logger_set_min_level(int level);

void free_logger();

#define __LOG_LEVEL(level, msg, ...) \
    logger_log_message_deferred(level, __FILE__, __LINE__, NULL, msg, ##__VA_ARGS__);

#define LOG_TRACE(...) __LOG_LEVEL(LOGGER_TRACE, __VA_ARGS__)
#define LOG_DEBUG(...) __LOG_LEVEL(LOGGER_DEBUG, __VA_ARGS__)
#define LOG_INFO(...)  __LOG_LEVEL(LOGGER_INFO,  __VA_ARGS__)
#define LOG_WARN(...)  __LOG_LEVEL(LOGGER_WARN,  __VA_ARGS__)
#define LOG_ERROR(...) __LOG_LEVEL(LOGGER_ERROR, __VA_ARGS__)
#define LOG_FATAL(...) __LOG_LEVEL(LOGGER_FATAL, __VA_ARGS__)
#define ACCESS_LOG(...) __LOG_LEVEL(LOGGER_ACCESS_LOG, __VA_ARGS__)

#endif