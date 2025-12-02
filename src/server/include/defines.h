#ifndef CONSTANTS_H

#define LOGGER_TRACE 0
#define LOGGER_DEBUG 1
#define LOGGER_INFO 2
#define LOGGER_WARN 3
#define LOGGER_ERROR 4
#define LOGGER_FATAL 5

#ifndef MAX_LOG_SIZE
#define MAX_LOG_SIZE 1024
#endif

#ifndef MAX_LOG_QUEUE_SIZE
#define MAX_LOG_QUEUE_SIZE 100
#endif

#ifndef LOGGER_MIN_LEVEL
#define LOGGER_MIN_LEVEL LOGGER_TRACE
#endif

#ifndef BUFFER_SIZE
#define BUFFER_SIZE 1024
#endif

#ifndef SUCCESS
#define SUCCESS 0
#endif

#ifndef FAILURE
#define FAILURE 1
#endif

#ifndef SOCKS5_VERSION
#define SOCKS5_VERSION 0x05


#define PAM_VERSION_1 0x01
#endif

#endif
