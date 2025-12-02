#ifndef __LOGGER_H__
#define __LOGGER_H__

#include <time.h>
#include "selector.h"

#define LOGGER_TRACE 0
#define LOGGER_DEBUG 1
#define LOGGER_INFO 2
#define LOGGER_WARN 3
#define LOGGER_ERROR 4
#define LOGGER_FATAL 5

#define LOG_LEVEL LOGGER_DEBUG

void loggerInit();

int loggerRegisterSelector(fd_selector selector);

void loggerLogMessageDeferred(int level, const char* file, int line, time_t * now, const char* msg, ...);

void flushAllLogs();

void freeLogger();

#define __LOG_LEVEL(level, msg, ...) \
    loggerLogMessageDeferred(level, __FILE__, __LINE__, NULL, msg, ##__VA_ARGS__);

#define LOG_TRACE(...) __LOG_LEVEL(LOGGER_TRACE, __VA_ARGS__)
#define LOG_DEBUG(...) __LOG_LEVEL(LOGGER_DEBUG, __VA_ARGS__)
#define LOG_INFO(...)  __LOG_LEVEL(LOGGER_INFO,  __VA_ARGS__)
#define LOG_WARN(...)  __LOG_LEVEL(LOGGER_WARN,  __VA_ARGS__)
#define LOG_ERROR(...) __LOG_LEVEL(LOGGER_ERROR, __VA_ARGS__)
#define LOG_FATAL(...) __LOG_LEVEL(LOGGER_FATAL, __VA_ARGS__)

#endif