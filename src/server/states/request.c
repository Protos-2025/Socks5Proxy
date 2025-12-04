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
#define RSV 0x00
#define IPv4_ADDR 0x01
#define FQDN 0x03
#define IPv6_ADDR 0x04
#define IPv4_ADDR_LEN 4
#define IPv6_ADDR_LEN 16

#define IS_VALID_ATYP(c) ((c) == IPv4_ADDR || (c) == FQDN || (c) == IPv6_ADDR)
#define ADDR_BYTES_BY_IP_VERSION(v) ((v) == IPv4_ADDR ? IPv4_ADDR_LEN : IPv6_ADDR_LEN)

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
    struct socks5 * connection = ATTACHMENT(key);

    if (connection->client.request.state == VER_CMD_ATYP) {
        uint8_t * w_ptr;
        size_t readn, to_write, to_read;

        w_ptr = buffer_write_ptr(&connection->client_buffer, &to_write);
        readn = recv(key->fd, w_ptr, to_write, 0);
    
        if (readn < 0) {
            connection->client.reply.rep = SERVER_FAILURE;
            return REPLY;
        }
        if (readn == 0) {
            LOG_INFO("Client closed connection (REQUEST)");
            return DONE;
        }
    
        buffer_write_adv(&connection->client_buffer, readn);

        buffer_read_ptr(&connection->client_buffer, &to_read);
        if (to_read < 4) {
            return REQUEST;
        }
    
        if (SOCKS5_VERSION != buffer_read(&connection->client_buffer)) {
            connection->client.reply.rep = INVALID_SOCKS5_VERSION;
            return REPLY;
        }
    
        connection->client.request.cmd = buffer_read(&connection->client_buffer);

        // only connect is supported (for now)
        if (connection->client.request.cmd != CONNECT_CMD) {
            connection->client.reply.rep = COMMAND_NOT_SUPPORTED;
            return REPLY;
        }
    
        if (RSV != buffer_read(&connection->client_buffer)) {
            connection->client.reply.rep = INVALID_RSV;
            return REPLY;
        }
    
        connection->client.request.atyp = buffer_read(&connection->client_buffer);
        if (!IS_VALID_ATYP(connection->client.request.atyp)) {
            connection->client.reply.rep = ADDRESS_TYPE_NOT_SUPPORTED;
            return REPLY;
        }
    
        connection->client.request.state = DST_LEN;
    }

    return resolve_dst_address(key);
}

unsigned request_block(struct selector_key * key) {
    return connect_to_dest(key);
}

static unsigned resolve_dst_address(struct selector_key * key) {
    struct socks5 * connection = ATTACHMENT(key);
    uint8_t * w_ptr;
    uint8_t addr_bytes;
    size_t readn, to_write, to_read;
    
    w_ptr = buffer_write_ptr(&connection->client_buffer, &to_write);
    readn = recv(connection->client_fd, w_ptr, to_write, 0);
    
    if (connection->client.request.state == DST_LEN) {
        if (readn < 0) {
            connection->client.reply.rep = SERVER_FAILURE;
            return REPLY;
        }
        if (readn == 0) {
            LOG_INFO("Client closed connection (REQUEST)");
            return DONE;
        }
        
        buffer_write_adv(&connection->client_buffer, readn);
        
        buffer_read_ptr(&connection->client_buffer, &to_read);
        
        if (connection->client.request.atyp == FQDN) {
            if (to_read < 1) {
                return REQUEST;
            }
            addr_bytes = buffer_read(&connection->client_buffer);
            if (addr_bytes == 0) {
                connection->client.reply.rep = INVALID_FQDN_LENGHT;
                return REPLY;
            }
        } else {
            addr_bytes = ADDR_BYTES_BY_IP_VERSION(connection->client.request.atyp);
        }
        
        if (to_read < (addr_bytes + 2)) {
            return REQUEST;
        }
        
        connection->client.request.state = DST_RES;
    }
    
    switch (connection->client.request.atyp) {
        case IPv4_ADDR:
            if (FAILURE == get_ipv4_address(connection)) {
                connection->client.reply.rep = SERVER_FAILURE;
                return REPLY;
            }
            get_port(connection);
            if (FAILURE == resolve_ipv4(connection)) {
                connection->client.reply.rep = SERVER_FAILURE;
                return REPLY;
            }
            LOG_DEBUG("IPv4 address (%s:%s)", connection->origin_host, connection->origin_port);
            break;
        
        case FQDN:
            get_fqdn(connection, addr_bytes);
            get_port(connection);
            if (FAILURE == resolve_fqdn(key)) {
                connection->client.reply.rep = SERVER_FAILURE;
                return REPLY;
            }
            break;
            
            default:
            if (FAILURE == get_ipv6_address(connection)) {
                connection->client.reply.rep = SERVER_FAILURE;
                return REPLY;
            }
            get_port(connection);
            if (FAILURE == resolve_ipv6(connection)) {
                connection->client.reply.rep = SERVER_FAILURE;
                return REPLY;
            }
            LOG_DEBUG("IPv6 address (%s:%s)", connection->origin_host, connection->origin_port);
            break;
    }

    return connect_to_dest(key);
}

static void get_port(struct socks5 * connection) {
    uint8_t p1 = buffer_read(&connection->client_buffer);
    uint8_t p2 = buffer_read(&connection->client_buffer);
    uint16_t port = (p1 << 8) | p2;
    snprintf((char *)connection->origin_port, 6, "%d", port);
}

static unsigned connect_to_dest(struct selector_key * key) {
    struct socks5 * connection = ATTACHMENT(key);

    if (connection->origin_resolutions_list == NULL) {
        connection->client.reply.rep = HOST_UNREACHABLE;
        return REPLY;
    }

    connection->origin_resolution = connection->origin_resolutions_list;

    int ret = try_connection(key);
    switch (ret) {
        case GENERAL_FAILURE:
        case SELECTOR_REGISTER_FAILED:
            connection->client.reply.rep = SERVER_FAILURE;
            break;

        case NETWORK_UNREACHABLE:
        case HOST_UNREACHABLE:
        case CONNECTION_REFUSED:
        case TTL_EXPIRED:
            connection->client.reply.rep = ret;
            break;

        case CONNECTION_IN_PROGRESS:
            return CONNECT; 
        
        default: {
            selector_set_interest(key->s, connection->origin_fd, OP_NOOP);
            connection->client.reply.rep = SUCCEDED;
        }
    };

    connection->origin_resolutions_list = NULL;

    selector_set_interest(key->s, connection->client_fd, OP_WRITE);

    return REPLY;
}

// <=========================================================== IPv4 ===========================================================>
static uint8_t get_ipv4_address(struct socks5 * connection) {
    struct in_addr addr;
    const char * ret;

    uint8_t * s_addr_ptr = (uint8_t *)&addr.s_addr;
    for (int i = 0; i < IPv4_ADDR_LEN; i++) {
        s_addr_ptr[i] = buffer_read(&connection->client_buffer);
    }

    ret = inet_ntop(AF_INET, &addr, (char *) connection->origin_host, HOST_MAX_LENGHT);

    return ret == NULL  ? FAILURE : SUCCESS;
}

static uint8_t resolve_ipv4(struct socks5 * connection) {
    struct sockaddr_in * sock_addr = calloc(1, sizeof(struct sockaddr_in));
    if (sock_addr == NULL) {
        return FAILURE;
    }
    sock_addr->sin_family = AF_INET;
    sock_addr->sin_port = htons(atoi((const char *) connection->origin_port));

    if (-1 == inet_pton(AF_INET, (const char *) connection->origin_host, &sock_addr->sin_addr)) {
        free(sock_addr);
        return FAILURE;
    }

    struct addrinfo * resolution = calloc(1, sizeof(struct addrinfo));
    if (resolution == NULL) {
        free(sock_addr);
        return FAILURE;
    }
    resolution->ai_family = AF_INET;
    resolution->ai_socktype = SOCK_STREAM;
    resolution->ai_protocol = IPPROTO_TCP;
    resolution->ai_addrlen = sizeof(struct sockaddr_in);
    resolution->ai_addr = (struct sockaddr *) sock_addr;

    connection->origin_resolutions_list = resolution;

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
    struct sockaddr_in6 * sock_addr = malloc(sizeof(struct sockaddr_in6));
    if (sock_addr == NULL) {
        return FAILURE;
    }
    sock_addr->sin6_family = AF_INET6;
    sock_addr->sin6_port = htons(atoi((const char *) connection->origin_port));
    sock_addr->sin6_flowinfo = 0;
    sock_addr->sin6_scope_id = 0;

    if (-1 == inet_pton(AF_INET6, (const char *) connection->origin_host, &sock_addr->sin6_addr)) {
        free(sock_addr);
        return FAILURE;
    }

    struct addrinfo * resolution = malloc(sizeof(struct addrinfo));
    if (resolution == NULL) {
        free(sock_addr);
        return FAILURE;
    }
    resolution->ai_flags = 0;
    resolution->ai_family = AF_INET6;
    resolution->ai_socktype = SOCK_STREAM;
    resolution->ai_protocol = IPPROTO_TCP;
    resolution->ai_addrlen = sizeof(struct sockaddr_in6);
    resolution->ai_addr = (struct sockaddr *) sock_addr;
    resolution->ai_canonname = NULL;
    resolution->ai_next = NULL;

    connection->origin_resolutions_list = resolution;

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
    pthread_t thread;
    if (0 != pthread_create(&thread, NULL, resolve_fqdn_blocking, key)) {
        return FAILURE;
    }

    return SUCCESS;
}

static void * resolve_fqdn_blocking(void * data) {
    struct selector_key * key = (struct selector_key *) data;
    struct socks5 * connection = ATTACHMENT(key);
    struct addrinfo hints;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = 0;
    hints.ai_protocol = 0;
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    if (0 != getaddrinfo((const char *) connection->origin_host, (const char *) connection->origin_port, &hints, &connection->origin_resolutions_list)) {
        // connection->client.reply.rep = SERVER_FAILURE;
        connection->origin_resolutions_list = NULL; // in case it was modify (this is the flag i use to define wheter it was resolved or not)
    }

    selector_notify_block(key->s, key->fd);
    return NULL;
}
// <============================================================================================================================>
