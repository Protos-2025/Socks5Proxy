#include "logger.h"
#include "queue.h"
#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "selector.h"

#define MAX_LOG_SIZE 1024

static const char* logLevelToString(int level);

static const char * logLevelToString(int level) {
    switch (level) {
        #ifndef NO_COLOR_LOGS
            case LOGGER_TRACE: return "\x1b[0;90mTRACE\x1b[0m";
            case LOGGER_DEBUG: return "\x1b[36mDEBUG\x1b[0m";
            case LOGGER_INFO:  return "\x1b[34mINFO\x1b[0m";
            case LOGGER_WARN:  return "\x1b[33mWARN\x1b[0m";
            case LOGGER_ERROR: return "\x1b[31mERROR\x1b[0m";
            case LOGGER_FATAL: return "\x1b[41;37mFATAL\x1b[0m";
            default:    return "UNKNOWN";
        #else
            case LOGGER_TRACE: return "TRACE";
            case LOGGER_DEBUG: return "DEBUG";
            case LOGGER_INFO:  return "INFO";
            case LOGGER_WARN:  return "WARN";
            case LOGGER_ERROR: return "ERROR";
            case LOGGER_FATAL: return "FATAL";
            default:    return "UNKNOWN";
        #endif
    }
}

static Queue logQueue = NULL;

void loggerInit() {
    logQueue = createQueue(NULL, sizeof(char *), 100);
}

int loggerRegisterSelector(fd_selector selector) {
    	if (!selector) return 0;

	selector_fd_set_nio(STDOUT_FILENO);

    static const struct fd_handler loggerHandlers = {
        .handle_read = NULL,
        .handle_write = flushAllLogs,
        .handle_close = freeLogger,
    };

	selector_status ss = SELECTOR_SUCCESS;
    ss = selector_register(selector, STDOUT_FILENO, &loggerHandlers, OP_WRITE, NULL);
    if (ss != SELECTOR_SUCCESS) {
        fprintf(stderr, "Failed to register logger flush handler: %s\n", selector_error(ss));
    }

    return ss == SELECTOR_SUCCESS ? 0 : -1;
}

static char * formatLogMessage(const char* levelStr, const char * file, int line, const time_t * now_ptr, const char* fmt, va_list args) {
    if (!levelStr) levelStr = "UNKNOWN";
    if (!fmt) fmt = "";

    char body[MAX_LOG_SIZE];

    int size = vsnprintf(body, (size_t)MAX_LOG_SIZE, fmt, args);

    char *out = (char *)malloc((size_t)MAX_LOG_SIZE);
	if (!out) return NULL;

	time_t now = now_ptr != NULL ? *now_ptr : time(NULL);
    struct tm *t = localtime(&now);

    int prefix_size = snprintf(out, (size_t)MAX_LOG_SIZE , "[%s] [%s:%d @ %04d-%02d-%02d %02d:%02d:%02d] ", levelStr, file, line, (t->tm_year + 1900), t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
	snprintf(out + prefix_size, MAX_LOG_SIZE - prefix_size, "%s", body);
	char* ellipsis = size >= (MAX_LOG_SIZE - strlen(out)) ? "..." : "\0";
	if (strlen(out) >= MAX_LOG_SIZE - 1) {
		strncpy(out + MAX_LOG_SIZE - 4, ellipsis, 4);
		out[MAX_LOG_SIZE - 1] = '\0';
	}
	return out;
}

void loggerLogMessageDeferred(int level, const char* file, int line, time_t * now, const char* msg, ...) {
    va_list args;
    va_start(args, msg);
    char *formattedMsg = formatLogMessage(logLevelToString(level), file, line, now, msg, args);
    va_end(args);

    if (formattedMsg) enqueue(logQueue, &formattedMsg);
}

void flushAllLogs() {
    if (!logQueue) return;
    while (queueSize(logQueue) > 0) {
        char * msg = NULL;
        dequeue(logQueue, &msg);
        if (msg) {
            fprintf(stdout, "%s\n", msg);
            free(msg);
        }
    }
}

void freeLogger() {
    if (!logQueue) return;
    while (queueSize(logQueue) > 0) {
        char * msg = NULL;
        dequeue(logQueue, &msg);
        if (msg) {
            free(msg);
        }
    }
    freeQueue(logQueue);
    logQueue = NULL;
}