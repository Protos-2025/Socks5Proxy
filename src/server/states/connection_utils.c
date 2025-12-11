#include "../include/connection_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <errno.h>
#include <arpa/inet.h>
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

void print_connection_access_log(struct socks5 * connection) {
    char originIp[INET6_ADDRSTRLEN] = {0};
    char clientIp[INET6_ADDRSTRLEN] = {0};
	uint16_t clientPort = 0;
    uint16_t originPort = ntohs((uint16_t)atoi((const char *)connection->origin_port));

	if (connection->origin_resolution->ai_family == AF_INET) {
        struct sockaddr_in * s = (struct sockaddr_in *) connection->origin_resolution->ai_addr;
        inet_ntop(AF_INET, &s->sin_addr, originIp, INET6_ADDRSTRLEN);
    } else if (connection->origin_resolution->ai_family == AF_INET6) {
        struct sockaddr_in6 * s = (struct sockaddr_in6 *) connection->origin_resolution->ai_addr;
        inet_ntop(AF_INET6, &s->sin6_addr, originIp, INET6_ADDRSTRLEN);
    }

    if (connection->client_addr.ss_family == AF_INET) {
        struct sockaddr_in * s = (struct sockaddr_in *) &connection->client_addr;
        inet_ntop(AF_INET, &s->sin_addr, clientIp, INET6_ADDRSTRLEN);
        clientPort = ntohs(s->sin_port);
    } else if (connection->client_addr.ss_family == AF_INET6) {
        struct sockaddr_in6 * s = (struct sockaddr_in6 *) &connection->client_addr;
        inet_ntop(AF_INET6, &s->sin6_addr, clientIp, INET6_ADDRSTRLEN);
        clientPort = ntohs(s->sin6_port);
    }

    if (connection->atyp == FQDN) {
        ACCESS_LOG("%s:%hu connected to %s:%hu (IPv%d=%s)",
                    clientIp, clientPort,
                    (char*)connection->origin_host,
                    originPort, connection->origin_resolution->ai_addr->sa_family == AF_INET ? 4 : 6,
                    originIp);
    } else {
        ACCESS_LOG("%s:%hu connected to IPv%d=%s on port %hu",
                    clientIp, clientPort, connection->origin_resolution->ai_addr->sa_family == AF_INET ? 4 : 6,
                    originIp, originPort);
    }
}
