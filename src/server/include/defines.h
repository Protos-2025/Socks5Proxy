#ifndef CONSTANTS_H

// <=================== These can be overriden with an env var =====================>

#ifndef MAX_LOG_SIZE
#define MAX_LOG_SIZE 1024
#endif

#ifndef MAX_LOG_QUEUE_SIZE
#define MAX_LOG_QUEUE_SIZE 100
#endif

#ifndef LOGGER_MIN_LEVEL
#define LOGGER_MIN_LEVEL LOGGER_INFO
#endif

#ifndef BUFFER_SIZE
#define BUFFER_SIZE 1024
#endif

#ifndef MAX_PENDING_CONNECTIONS
#define MAX_PENDING_CONNECTIONS 20
#endif

#ifndef SELECTOR_CAPACITY
#define SELECTOR_CAPACITY 1024
#endif

#ifndef LOG_FILE_NAME
#define LOG_FILE_NAME NULL
#endif


// <=================== Cannot be overriden =====================>
#define LOGGER_TRACE 0
#define LOGGER_DEBUG 1
#define LOGGER_INFO 2
#define LOGGER_WARN 3
#define LOGGER_ERROR 4
#define LOGGER_FATAL 5
#define LOGGER_ACCESS_LOG 6


#define SUCCESS 0
#define FAILURE 1
#define SOCKS5_VERSION 0x05
#define PAM_VERSION_1 0x01
#define RSV 0x00
#define IPv4_ADDR 0x01
#define FQDN 0x03
#define IPv6_ADDR 0x04
#define IPv4_ADDR_LEN 4
#define IPv6_ADDR_LEN 16

#endif
