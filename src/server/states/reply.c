#include <netdb.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "../include/reply.h"
#include "../include/socks5nio.h"
#include "../../shared/include/buffer.h"
#include "logger.h"

void reply_arrival(const unsigned state, struct selector_key * key) {
    LOG_TRACE("Replying...");

    struct socks5 * connection = ATTACHMENT(key);

    buffer_write(&connection->origin_buffer, SOCKS5_VERSION);
    buffer_write(&connection->origin_buffer, connection->client.reply.rep);
    buffer_write(&connection->origin_buffer, RSV);

    struct sockaddr_storage localAddr;
    socklen_t localAddrLen = sizeof(localAddr);
    uint16_t bndPort = 0;

    if (getsockname(connection->origin_fd, (struct sockaddr *)&localAddr, &localAddrLen) == -1) {
        LOG_ERROR("getsockname failed: %d", errno);
        connection->client.reply.found_bnd_info = false;
        return;
    }

    if (localAddr.ss_family == AF_INET) {
        struct sockaddr_in * sin = (struct sockaddr_in *)&localAddr;

        buffer_write(&connection->origin_buffer, IPv4_ADDR);

        for (size_t i = 0; i < IPv4_ADDR_LEN; i++) {
            buffer_write(&connection->origin_buffer, ((uint8_t *)&sin->sin_addr.s_addr)[i]);
        }

        bndPort = ntohs(sin->sin_port);
    } else if (localAddr.ss_family == AF_INET6) {
        struct sockaddr_in6 * sin6 = (struct sockaddr_in6 *)&localAddr;

        buffer_write(&connection->origin_buffer, IPv6_ADDR);

        for (size_t i = 0; i < IPv6_ADDR_LEN; i++) {
            buffer_write(&connection->origin_buffer, sin6->sin6_addr.s6_addr[i]);
        }

        bndPort = ntohs(sin6->sin6_port);
    }

    buffer_write(&connection->origin_buffer, (bndPort >> 8) & 0xFF);
    buffer_write(&connection->origin_buffer, bndPort & 0xFF);

    connection->client.reply.found_bnd_info = true;
}

unsigned reply_write(struct selector_key * key) {
    LOG_TRACE("Replying...");

    struct socks5 * connection = ATTACHMENT(key);

    if (!connection->client.reply.found_bnd_info) {
        return ERROR;
    }

    uint8_t * rPtr;
    size_t toRead, written;
    
    rPtr = buffer_read_ptr(&connection->origin_buffer, &toRead);
    written = send(connection->client_fd, rPtr, toRead, 0);
    buffer_read_adv(&connection->origin_buffer, written);

    if (written < 0) {
        LOG_FATAL("Send failed (REPLY)");
        return ERROR;
    }

    if (buffer_can_read(&connection->origin_buffer)) {
        return REPLY;
    }
    
    if (connection->client.reply.rep == SUCCEDED) {
        LOG_DEBUG("Replied successfully");
        return COPY;
    }

    selector_set_interest(key->s, connection->client_fd, OP_READ);
    return REQUEST;
}
