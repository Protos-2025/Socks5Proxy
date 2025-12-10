#include "../include/request.h"
#include "../include/socks5nio.h"
#include "../include/connection_utils.h"
#include "../include/reply.h"
#include "logger.h"
#include <stdio.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>

#define CONNECT_CMD 0x01
// #define BIND_CMD 0x02
// #define UDP_ASSOCIATE_CDM 0x03
#define IPv4_ADDR_LEN 4
#define IPv6_ADDR_LEN 16

#define IS_VALID_ATYP(c) ((c) == IPv4_ADDR || (c) == FQDN || (c) == IPv6_ADDR)
#define ADDR_BYTES_BY_IP_VERSION(v) ((v) == IPv4_ADDR ? IPv4_ADDR_LEN : IPv6_ADDR_LEN)

static unsigned to_reply_state(struct selector_key * key);
static unsigned resolve_dst_address(struct selector_key * key);
static void get_port(struct socks5 * connection);
static unsigned connect_to_dest(struct selector_key * key);
static uint8_t get_ipv4_address(struct socks5 * connection);
static uint8_t resolve_ipv4(struct socks5 * connection);
static uint8_t get_ipv6_address(struct socks5 * connection);
static uint8_t resolve_ipv6(struct socks5 * connection);
static void get_fqdn(struct socks5 * connection, uint8_t lenght);
static uint8_t resolve_fqdn(struct selector_key * key);
static void * resolve_fqdn_blocking(void * data);

void request_arrival(const unsigned state, struct selector_key * key) {
    LOG_DEBUG("Processing request...");
    ATTACHMENT(key)->client.request.state = VER_CMD_ATYP;
}

unsigned request_read(struct selector_key * key) {
    LOG_TRACE("Reading request...");
    struct socks5 * connection = ATTACHMENT(key);

    if (connection->client.request.state == VER_CMD_ATYP) {
        uint8_t * wPtr;
        size_t readn, toWrite, toRead;

        wPtr = buffer_write_ptr(&connection->client_buffer, &toWrite);
        readn = recv(key->fd, wPtr, toWrite, 0);

        buffer_write_adv(&connection->client_buffer, readn);
        buffer_read_ptr(&connection->client_buffer, &toRead);
    
        if (toRead < 0) {
            connection->client.reply.rep = SERVER_FAILURE;
            return to_reply_state(key);
        }

        if (toRead == 0) {
            return DONE;
        }
    
        if (toRead < 4) {
            return REQUEST;
        }

		uint8_t ver = buffer_read(&connection->client_buffer);
		toRead--;
		if (ver != SOCKS5_VERSION) {
			connection->client.reply.rep = INVALID_SOCKS5_VERSION;
            return to_reply_state(key);
		}

		connection->client.request.cmd = buffer_read(&connection->client_buffer);
		toRead--;

		// only connect is supported (for now)
        if (connection->client.request.cmd != CONNECT_CMD) {
            connection->client.reply.rep = COMMAND_NOT_SUPPORTED;
            return to_reply_state(key);
        }

		uint8_t rsv = buffer_read(&connection->client_buffer);
		toRead--;
		if (RSV != rsv) {
			connection->client.reply.rep = INVALID_RSV;
            return to_reply_state(key);
		}

		connection->client.request.atyp = buffer_read(&connection->client_buffer);
		toRead--;
		if (!IS_VALID_ATYP(connection->client.request.atyp)) {
			connection->client.reply.rep = ADDRESS_TYPE_NOT_SUPPORTED;
            return to_reply_state(key);
		}
        
		connection->atyp = connection->client.request.atyp;
        connection->client.request.state = DST_LEN;
    }

    return resolve_dst_address(key);
}

unsigned request_block(struct selector_key * key) {
    return connect_to_dest(key);
}

static unsigned to_reply_state(struct selector_key * key) {
    selector_set_interest(key->s, ATTACHMENT(key)->client_fd, OP_WRITE);
    return REPLY;
}

static unsigned resolve_dst_address(struct selector_key * key) {
    struct socks5 * connection = ATTACHMENT(key);
    uint8_t * wPtr;
    uint8_t addrBytes;
    size_t readn, toWrite, toRead;
    
    wPtr = buffer_write_ptr(&connection->client_buffer, &toWrite);
    readn = recv(connection->client_fd, wPtr, toWrite, 0);

    buffer_write_adv(&connection->client_buffer, readn);
    buffer_read_ptr(&connection->client_buffer, &toRead);
    
    if (connection->client.request.state == DST_LEN) {
        if (toRead < 0) {
            connection->client.reply.rep = SERVER_FAILURE;
            return to_reply_state(key);
        }
        if (toRead == 0) {
            LOG_DEBUG("Client closed connection (REQUEST)");
            return DONE;
        }
        
        if (connection->client.request.atyp == FQDN) {
            if (toRead < 1) {
                return REQUEST;
            }
            addrBytes = buffer_read(&connection->client_buffer);
			toRead--;
			if (addrBytes == 0) {
				connection->client.reply.rep = INVALID_FQDN_LENGHT;
                return to_reply_state(key);
			}
		} else {
            addrBytes = ADDR_BYTES_BY_IP_VERSION(connection->client.request.atyp);
        }
        
        if (toRead < (addrBytes + 1)) {
            return REQUEST;
        }
        
        connection->client.request.state = DST_RES;
    }
    
    switch (connection->client.request.atyp) {
        case IPv4_ADDR:
            if (FAILURE == get_ipv4_address(connection)) {
                connection->client.reply.rep = SERVER_FAILURE;
                return to_reply_state(key);
            }
            get_port(connection);
            LOG_DEBUG("IPv4 address (%s:%s)", connection->origin_host, connection->origin_port);
            if (FAILURE == resolve_ipv4(connection)) {
                connection->client.reply.rep = SERVER_FAILURE;
                return to_reply_state(key);
            }
            break;
        
        case FQDN:
            get_fqdn(connection, addrBytes);
            get_port(connection);
            LOG_TRACE("FQDN address (%s:%s)", connection->origin_host, connection->origin_port);
            LOG_DEBUG("Request processed successfully");
            if (FAILURE == resolve_fqdn(key)) {
                connection->client.reply.rep = SERVER_FAILURE;
                return to_reply_state(key);
            }
            return REQUEST;
            
        default:
            if (FAILURE == get_ipv6_address(connection)) {
                connection->client.reply.rep = SERVER_FAILURE;
                return to_reply_state(key);
            }
            get_port(connection);
            LOG_DEBUG("IPv6 address (%s:%s)", connection->origin_host, connection->origin_port);
            if (FAILURE == resolve_ipv6(connection)) {
                connection->client.reply.rep = SERVER_FAILURE;
                return to_reply_state(key);
            }
            break;
    }

    LOG_DEBUG("Request processed successfully");

    return connect_to_dest(key);
}

static void get_port(struct socks5 * connection) {
    uint8_t p1 = buffer_read(&connection->client_buffer);
    uint8_t p2 = buffer_read(&connection->client_buffer);
    uint16_t port = (p1 << 8) | p2;
    snprintf((char *)connection->origin_port, PORT_MAX_LENGHT, "%u", port);
    connection->origin_port[PORT_MAX_LENGHT - 1] = '\0';
}

static unsigned connect_to_dest(struct selector_key * key) {
    struct socks5 * connection = ATTACHMENT(key);

    LOG_TRACE("Connecting to origin...");

    if (connection->origin_resolution == NULL) {
        connection->client.reply.rep = SERVER_FAILURE;
        return to_reply_state(key);
    }

    int ret = try_connection(key);
    switch (ret) {
        case GENERAL_FAILURE:
            LOG_WARN("General failure connecting to origin");
			connection->client.reply.rep = SERVER_FAILURE;
			break;
		case SELECTOR_REGISTER_FAILED:
            LOG_WARN("Selector register failed");
            connection->client.reply.rep = SERVER_FAILURE;
            break;
        case NETWORK_UNREACHABLE:
            LOG_WARN("Network unreachable");
            connection->client.reply.rep = ret;
            break;
        case HOST_UNREACHABLE:
            LOG_WARN("Host unreachable");
            connection->client.reply.rep = ret;
            break;
        case CONNECTION_REFUSED:
            LOG_WARN("Connection refused");
            connection->client.reply.rep = ret;
            break;
        case TTL_EXPIRED:
            LOG_WARN("TTL expired");
            connection->client.reply.rep = ret;
            break;
        case CONNECTION_IN_PROGRESS:
            LOG_TRACE("Connection in progress");
            selector_set_interest(key->s, connection->client_fd, OP_WRITE);
            return CONNECT; 
        default: {
            LOG_DEBUG("Connected to origin");
            selector_set_interest(key->s, connection->origin_fd, OP_NOOP);
            connection->client.reply.rep = SUCCEDED;
        }
    };

    return to_reply_state(key);
}

// <=========================================================== IPv4 ===========================================================>
static uint8_t get_ipv4_address(struct socks5 * connection) {
    struct in_addr addr;
    const char * ret;

    uint8_t * sAddrPtr = (uint8_t *)&addr.s_addr;
    for (int i = 0; i < IPv4_ADDR_LEN; i++) {
        sAddrPtr[i] = buffer_read(&connection->client_buffer);
    }

    ret = inet_ntop(AF_INET, &addr, (char *) connection->origin_host, HOST_MAX_LENGHT);

    return ret == NULL  ? FAILURE : SUCCESS;
}

static uint8_t resolve_ipv4(struct socks5 * connection) {
    struct sockaddr_in * sockAddr = calloc(1, sizeof(struct sockaddr_in));
    if (sockAddr == NULL) {
        return FAILURE;
    }
    sockAddr->sin_family = AF_INET;
    sockAddr->sin_port = htons(atoi((const char *) connection->origin_port));

    if (-1 == inet_pton(AF_INET, (const char *) connection->origin_host, &sockAddr->sin_addr)) {
        free(sockAddr);
        return FAILURE;
    }

    struct addrinfo * resolution = calloc(1, sizeof(struct addrinfo));
    if (resolution == NULL) {
        free(sockAddr);
        return FAILURE;
    }
    resolution->ai_family = AF_INET;
    resolution->ai_socktype = SOCK_STREAM;
    resolution->ai_protocol = IPPROTO_TCP;
    resolution->ai_addrlen = sizeof(struct sockaddr_in);
    resolution->ai_addr = (struct sockaddr *) sockAddr;

    connection->origin_resolution = resolution;

    return SUCCESS;
}
// <============================================================================================================================>


// <=========================================================== IPv6 ===========================================================>
static uint8_t get_ipv6_address(struct socks5 * connection) {
    struct in6_addr addr;
    const char * ret;

    for (int i = 0; i < IPv6_ADDR_LEN; i++) {
        addr.s6_addr[i] = buffer_read(&connection->client_buffer);
    }
    
    ret = inet_ntop(AF_INET6, &addr, (char *) connection->origin_host, HOST_MAX_LENGHT);

    return ret == NULL ? FAILURE : SUCCESS;
}

static uint8_t resolve_ipv6(struct socks5 * connection) {
    struct sockaddr_in6 * sockAddr = calloc(1, sizeof(struct sockaddr_in6));
    if (sockAddr == NULL) {
        return FAILURE;
    }
    sockAddr->sin6_family = AF_INET6;
    sockAddr->sin6_port = htons(atoi((const char *) connection->origin_port));

    if (-1 == inet_pton(AF_INET6, (const char *) connection->origin_host, &sockAddr->sin6_addr)) {
        free(sockAddr);
        return FAILURE;
    }

    struct addrinfo * resolution = calloc(1, sizeof(struct addrinfo));
    if (resolution == NULL) {
        free(sockAddr);
        return FAILURE;
    }
    resolution->ai_family = AF_INET6;
    resolution->ai_socktype = SOCK_STREAM;
    resolution->ai_protocol = IPPROTO_TCP;
    resolution->ai_addrlen = sizeof(struct sockaddr_in6);
    resolution->ai_addr = (struct sockaddr *) sockAddr;

    connection->origin_resolution = resolution;

    return SUCCESS;
}
// <============================================================================================================================>


// <=========================================================== FQDN ===========================================================>
static void get_fqdn(struct socks5 * connection, uint8_t lenght) {
    int i;
    for (i = 0; i < lenght; i++) {
        connection->origin_host[i] = buffer_read(&connection->client_buffer);
    }
    connection->origin_host[i] = '\0';
}

static uint8_t resolve_fqdn(struct selector_key * key) {
    LOG_DEBUG("Resolving domain name: %s...", ((struct socks5 *)ATTACHMENT(key))->origin_host);

    pthread_t thread;
    struct selector_key * keyCopy = malloc(sizeof(struct selector_key));
    if (keyCopy == NULL) {
        return FAILURE;
    }
    memcpy(keyCopy, key, sizeof(struct selector_key));

    if (0 != pthread_create(&thread, NULL, resolve_fqdn_blocking, keyCopy)) {
        free(keyCopy);
        return FAILURE;
    }

    pthread_detach(thread);

    return SUCCESS;
}

static void * resolve_fqdn_blocking(void * data) {
    struct selector_key * key = (struct selector_key *) data;
    struct socks5 * connection = ATTACHMENT(key);
    struct addrinfo hints;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_protocol = IPPROTO_TCP;

    if (0 != getaddrinfo((const char *) connection->origin_host, (const char *) connection->origin_port, &hints, &connection->origin_resolution)) {
        if (connection->origin_resolution != NULL) {
            freeaddrinfo(connection->origin_resolution);
        }
        // connection->client.reply.rep = SERVER_FAILURE;
        connection->origin_resolution = NULL; // in case it was modify (this is the flag i use to define wheter it was resolved or not)
    }

    selector_notify_block(key->s, key->fd);
    free(key);
    return NULL;
}
// <============================================================================================================================>
