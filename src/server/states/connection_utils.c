#include "../include/connection_utils.h"
#include <stdio.h>
#include <netdb.h>
#include <errno.h>
#include "../include/socks5nio.h"
#include "logger.h"

int try_connection(struct selector_key * key) {
    struct socks5 * connection = ATTACHMENT(key);

    int ret;

    connection->origin_fd = socket(connection->origin_resolution->ai_family, connection->origin_resolution->ai_socktype, connection->origin_resolution->ai_protocol);
    if (connection->origin_fd == -1) {
        get_next_resolution(key);
        return try_connection(key);
    }

    if (selector_fd_set_nio(connection->origin_fd) == -1) {
        close(connection->origin_fd);
        connection->origin_fd = -1;
        get_next_resolution(key);
        return try_connection(key);
    }

    ret = connect(connection->origin_fd, connection->origin_resolution->ai_addr, connection->origin_resolution->ai_addrlen);
    if (ret == 0) {
        return CONNECTION_DONE;
    } else if (errno == EINPROGRESS) {
        if (SELECTOR_SUCCESS != selector_register(key->s, connection->origin_fd, &socks5Handler, OP_WRITE, connection)) {
            return SELECTOR_REGISTER_FAILED;
        }
        return CONNECTION_IN_PROGRESS;
    }

    if (connection->origin_resolution->ai_next != NULL) {
        selector_unregister_fd_without_closing(key->s, connection->origin_fd);
        close(connection->origin_fd);
        connection->origin_fd = -1;
        get_next_resolution(key);
        return try_connection(key);
    }

    switch (errno) {
        case ENETUNREACH:
            return NETWORK_UNREACHABLE;

        case EHOSTUNREACH:
            return HOST_UNREACHABLE;

        case ECONNREFUSED:
            return CONNECTION_REFUSED;

        case ETIMEDOUT:
            return TTL_EXPIRED;

        default:
            return GENERAL_FAILURE;
    }
}

void get_next_resolution(struct selector_key * key) {
    struct socks5 * connection = ATTACHMENT(key);
    struct addrinfo * next = connection->origin_resolution->ai_next;
    connection->origin_resolution->ai_next = NULL;
    freeaddrinfo(connection->origin_resolution);
    connection->origin_resolution = next;
}
