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
#include <fcntl.h>
#include "selector.h"
#include "defines.h"
#include "assert.h"

#define LOG_FILE_FLAGS (O_APPEND | O_WRONLY | O_CREAT | O_NONBLOCK)
#define LOG_FILE_PERMISSIONS (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH)

static const char* log_level_to_string(int level);
static void write_logs_deferred(struct selector_key* key);
static void free_log(void * data);

static Queue logQueue = NULL;
static Buffer logBuffer;
static char * currentLog = NULL;
static int minLogLevel = LOGGER_MIN_LEVEL;
static int logFile = -1;
static FdSelector loggerSelector = NULL;
static long logAlwaysIncrementalId = 0; // Used to detect logs that may have been dropped

static const struct fd_handler loggerHandlers = {
    .handle_read = NULL,
    .handle_write = write_logs_deferred,
    .handle_close = NULL,
};

static const char* log_level_to_string(int level) {
    #ifdef NO_COLOR_LOGS
        switch (level) {
            case LOGGER_TRACE: return "TRACE";
            case LOGGER_DEBUG: return "DEBUG";
            case LOGGER_INFO:  return "INFO";
            case LOGGER_WARN:  return "WARN";
            case LOGGER_ERROR: return "ERROR";
            case LOGGER_FATAL: return "FATAL";
            case LOGGER_ACCESS_LOG: return "ACCESS";
            default:    return "UNKNOWN";
        }
    #endif

	int useColorLogs = (logFile == STDOUT_FILENO);

    if (useColorLogs) {
        switch (level) {
            case LOGGER_TRACE: return "\x1b[0;90mTRACE\x1b[0m";
            case LOGGER_DEBUG: return "\x1b[36mDEBUG\x1b[0m";
            case LOGGER_INFO:  return "\x1b[34mINFO\x1b[0m";
            case LOGGER_WARN:  return "\x1b[33mWARN\x1b[0m";
            case LOGGER_ERROR: return "\x1b[31mERROR\x1b[0m";
            case LOGGER_FATAL: return "\x1b[41;37mFATAL\x1b[0m";
            case LOGGER_ACCESS_LOG:   return "\x1b[44m\x1b[37mACCESS\x1b[0m";
            default:    return "UNKNOWN";
        }
    } else {
        switch (level) {
            case LOGGER_TRACE: return "TRACE";
            case LOGGER_DEBUG: return "DEBUG";
            case LOGGER_INFO:  return "INFO";
            case LOGGER_WARN:  return "WARN";
            case LOGGER_ERROR: return "ERROR";
            case LOGGER_FATAL: return "FATAL";
            case LOGGER_ACCESS_LOG:   return "ACCESS";
            default:    return "UNKNOWN";
        }
    }
}

static int open_file_non_blocking(const char * filename) {
    int file = -1;
    if (filename == NULL) {
        file = STDOUT_FILENO;
        LOG_WARN("No file provided, logging to STDOUT");
    } else {
		file = open(filename, LOG_FILE_FLAGS, LOG_FILE_PERMISSIONS);
        if (file < 0) {
            LOG_ERROR("Failed to open log file \"%s\": %s. Logging to STD_OUT", filename, strerror(errno));
            file = STDOUT_FILENO;
        }
    }
    return file;
}

void logger_init() {
    logQueue = create_queue(free_log, sizeof(char *), MAX_LOG_QUEUE_SIZE);
    buffer_init(&logBuffer, MAX_LOG_SIZE, NULL);
    logFile = open_file_non_blocking(LOG_FILE_NAME);
}

int logger_register_selector(FdSelector selector) {
    if (!selector) return 0;

	if (selector_fd_set_nio(logFile) < 0) {
        fprintf(stderr, "Failed to set log file to non-blocking mode\n");
        return -1;
    };


    SelectorStatus ss = SELECTOR_SUCCESS;
    if (loggerSelector == NULL) {
        ss = selector_register(selector, logFile, &loggerHandlers, OP_WRITE, NULL);
    } else {
        selector_set_interest(selector, logFile, OP_WRITE);
    }

    if (loggerSelector == NULL) {
        loggerSelector = selector;
    }

    if (ss != SELECTOR_SUCCESS) {
        fprintf(stderr, "Failed to register logger flush handler: %s\n", selector_error(ss));
    }

    return ss == SELECTOR_SUCCESS ? 0 : -1;
}

int logger_unregister_selector(FdSelector selector) {
    if (!selector) return 0;

    SelectorStatus ss = SELECTOR_SUCCESS;
	// avoid checking, since it's either successfull or was already unregistered
    ss = selector_set_interest(selector, logFile, OP_NOOP);

	return ss == SELECTOR_SUCCESS ? 0 : -1;
}

static void free_log(void * data) {
    if (data) {
        char * msg = *(char **)data;
        free(msg);
    }
}

static char * format_log_message(int level, const char * file, int line, const time_t * now_ptr, const char* fmt, va_list args) {
    const char * levelStr = log_level_to_string(level);
	const long logId = logAlwaysIncrementalId++;
	if (!fmt) fmt = "";

	char body[MAX_LOG_SIZE];

    vsnprintf(body, (size_t)MAX_LOG_SIZE, fmt, args);

    char *out = (char *)malloc((size_t)MAX_LOG_SIZE);
	if (!out) return NULL;

	time_t now = now_ptr != NULL ? *now_ptr : time(NULL);
    struct tm *t = localtime(&now);
    char * padding = (level == LOGGER_INFO || level == LOGGER_WARN) ? " " : "";

    int prefixSize = level == LOGGER_ACCESS_LOG ?
      snprintf(out, (size_t)MAX_LOG_SIZE , "[%s]%s [%06ld] [%04d-%02d-%02d %02d:%02d:%02d]  ", levelStr, padding, logId % 1000000, (t->tm_year + 1900), t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec)
    : snprintf(out, (size_t)MAX_LOG_SIZE , "[%s]%s  [%06ld] [%04d-%02d-%02d %02d:%02d:%02d @ %s:%d] ", levelStr, padding, logId % 1000000, (t->tm_year + 1900), t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec, file, line);
	
    snprintf(out + prefixSize, MAX_LOG_SIZE - prefixSize, "%s\n", body);
	const char* ellipsis = "...\n";
	if (strlen(out) >= MAX_LOG_SIZE - 1) {
		strncpy(out + MAX_LOG_SIZE - 5, ellipsis, 5);
		out[MAX_LOG_SIZE - 1] = '\0';
	}
	return out;
}

void logger_log_message_deferred(int level, const char* file, int line, time_t * now, const char* msg, ...) {
    if (!logQueue || level < minLogLevel) return;
    va_list args;
    va_start(args, msg);
    char *formattedMsg = format_log_message(level, file, line, now, msg, args);
    va_end(args);

    if (formattedMsg) {
        enqueue(logQueue, &formattedMsg);
		if (loggerSelector > 0 && ((logAlwaysIncrementalId % (MAX_LOG_QUEUE_SIZE / 3)) == 0)) {
            logger_register_selector(loggerSelector);
        }
	}
}

static void write_logs_deferred(struct selector_key* key) {
	uint8_t* rPtr = 0;
	size_t toRead = 0, queueSize;
	int written = 0;

    while ((queueSize = queue_size(logQueue)) > 0) {
        char * peekPtr = NULL;
        if (toRead == 0 && written == toRead && queue_peek(logQueue, &peekPtr) != NULL) {
            dequeue(logQueue, &currentLog);
            buffer_init(&logBuffer, strlen(currentLog), (uint8_t *) currentLog);
            buffer_write_adv(&logBuffer, strlen(currentLog));
        } else {
            break;
        }

        if (buffer_can_read(&logBuffer)) {
            rPtr = buffer_read_ptr(&logBuffer, (size_t *) &toRead);
            written = write(logFile, rPtr, toRead);
            buffer_read_adv(&logBuffer, written);
            if (written == toRead) {
                free(currentLog);
                currentLog = NULL;
			}
		}
	}

    if (queueSize == 0 && written == toRead)
        logger_unregister_selector(loggerSelector);
}

void flush_all_logs() {
    if (!logQueue) return;
    while (queue_size(logQueue) > 0) {
        char * msg = NULL;
        dequeue(logQueue, &msg);
        if (msg) {
            write(logFile, msg, strlen(msg));
            free(msg);
        }
    }
}

void free_logger() {
    if (!logQueue) return;
    if (logFile >= 0 && logFile != STDOUT_FILENO) {
        close(logFile);
        logFile = -1;
    }
	flush_all_logs();
	free_queue(logQueue);
	logQueue = NULL;
}

int logger_get_min_level() {
    return minLogLevel;
}

void logger_set_min_level(int level) {
    minLogLevel = level;
}

