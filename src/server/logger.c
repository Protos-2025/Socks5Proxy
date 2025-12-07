#include "logger.h"
#include "buffer.h"
#include "queue.h"
#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include "selector.h"
#include "defines.h"

static const char* log_level_to_string(int level);
static void write_logs_deferred(struct selector_key* key);
static void free_log(void * data);

static const char * log_level_to_string(int level) {
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
static Buffer logBuffer;
static char * currentLog = NULL;

void logger_init() {
    logQueue = create_queue(free_log, sizeof(char *), MAX_LOG_QUEUE_SIZE);
    buffer_init(&logBuffer, MAX_LOG_SIZE, NULL);
}

int logger_register_selector(FdSelector selector) {
    if (!selector) return 0;

	if (selector_fd_set_nio(STDOUT_FILENO) < 0) {
        fprintf(stderr, "Failed to set STDOUT to non-blocking mode\n");
        return -1;
    };

    static const struct fd_handler logger_handlers = {
        .handle_read = NULL,
        .handle_write = write_logs_deferred,
        .handle_close = free_logger,
    };

	SelectorStatus ss = SELECTOR_SUCCESS;
    ss = selector_register(selector, STDOUT_FILENO, &logger_handlers, OP_WRITE, NULL);
    if (ss != SELECTOR_SUCCESS) {
        fprintf(stderr, "Failed to register logger flush handler: %s\n", selector_error(ss));
    }

    return ss == SELECTOR_SUCCESS ? 0 : -1;
}

static void free_log(void * data) {
    if (data) {
        char * msg = *(char **)data;
        free(msg);
    }
}

static char * format_log_message(const char* level_str, const char * file, int line, const time_t * now_ptr, const char* fmt, va_list args) {
    if (!level_str) level_str = "UNKNOWN";
    if (!fmt) fmt = "";

    char body[MAX_LOG_SIZE];

    vsnprintf(body, (size_t)MAX_LOG_SIZE, fmt, args);

    char *out = (char *)malloc((size_t)MAX_LOG_SIZE);
	if (!out) return NULL;

	time_t now = now_ptr != NULL ? *now_ptr : time(NULL);
    struct tm *t = localtime(&now);

    int prefixSize = snprintf(out, (size_t)MAX_LOG_SIZE , "[%s] [%s:%d @ %04d-%02d-%02d %02d:%02d:%02d] ", level_str, file, line, (t->tm_year + 1900), t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
	snprintf(out + prefixSize, MAX_LOG_SIZE - prefixSize, "%s\n", body);
	const char* ellipsis = "...\n";
	if (strlen(out) >= MAX_LOG_SIZE - 1) {
		strncpy(out + MAX_LOG_SIZE - 5, ellipsis, 5);
		out[MAX_LOG_SIZE - 1] = '\0';
	}
	return out;
}

void logger_log_message_deferred(int level, const char* file, int line, time_t * now, const char* msg, ...) {
    va_list args;
    va_start(args, msg);
    char *formattedMsg = format_log_message(log_level_to_string(level), file, line, now, msg, args);
    va_end(args);

    if (formattedMsg) enqueue(logQueue, &formattedMsg);
}

static void write_logs_deferred(struct selector_key* key) {
	uint8_t* rPtr = 0;
	size_t toRead = 0;
	int written = 0;

	if (buffer_can_read(&logBuffer)) {
		rPtr = buffer_read_ptr(&logBuffer, (size_t *) &toRead);
		written = write(STDOUT_FILENO, rPtr, toRead);
		buffer_read_adv(&logBuffer, written);
        if (written == toRead) {
            free(currentLog);
            currentLog = NULL;
        }
	};

    char * peekPtr = NULL;
	if (toRead == 0 && written == toRead && queue_peek(logQueue, &peekPtr) != NULL) {
        dequeue(logQueue, &currentLog);
        buffer_init(&logBuffer, strlen(currentLog), (uint8_t *) currentLog);
        buffer_write_adv(&logBuffer, strlen(currentLog));
	}
}

void flush_all_logs() {
    if (!logQueue) return;
    while (queue_size(logQueue) > 0) {
        char * msg = NULL;
        dequeue(logQueue, &msg);
        if (msg) {
            fprintf(stdout, "%s\n", msg);
            free(msg);
        }
    }
}

void free_logger() {
    if (!logQueue) return;
	flush_all_logs();
	free_queue(logQueue);
	logQueue = NULL;
}