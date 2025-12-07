#include "../include/connection_utils.h"
#include <stdio.h>
#include <netdb.h>
#include <errno.h>
#include "../include/socks5nio.h"
#include "logger.h"

int try_connection(struct selector_key * key) {
    struct socks5 * connection = ATTACHMENT(key);

    int fd, ret = -1, last_errno = 0;

    if (connection->origin_fd != -1) {
        close(connection->origin_fd);
        connection->origin_fd = -1;
        connection->origin_resolution = connection->origin_resolution->ai_next;
    }

    for (; connection->origin_resolution != NULL; connection->origin_resolution = connection->origin_resolution->ai_next) {
        fd = socket(connection->origin_resolution->ai_family, connection->origin_resolution->ai_socktype, connection->origin_resolution->ai_protocol);
        if (fd == -1) {
            last_errno = errno;
            continue;
        }

        if (selector_fd_set_nio(fd) == -1) {
            last_errno = errno;
            close(fd);
            continue;
        }

        ret = connect(fd, connection->origin_resolution->ai_addr, connection->origin_resolution->ai_addrlen);
        if (ret == 0 || (ret == -1 && (errno == EINPROGRESS || errno == 115))) {
            break;
        }

        last_errno = errno;
        close(fd);
    }

    if (connection->origin_resolution == NULL) {
        connection->origin_fd = -1;
        switch (last_errno) {
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

    connection->origin_fd = fd;

    if (ret == -1 && (last_errno == EINPROGRESS || last_errno == 115)) {
        if (SELECTOR_SUCCESS != selector_register(key->s, fd, &socks5_handler, OP_WRITE, connection)) {
            close(fd);
            connection->origin_fd = -1;
            return SELECTOR_REGISTER_FAILED;
        }
        return CONNECTION_IN_PROGRESS;
    }

    return CONNECTION_DONE;
}
