#include <netdb.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include "../include/reply.h"
#include "../include/socks5nio.h"
#include "../../shared/include/buffer.h"
#include "logger.h"

#define RSV 0x00
// #define FQDN 0x03

void reply_arrival(const unsigned state, struct selector_key * key) {
    LOG_TRACE("Replying...");

    struct socks5 * connection = ATTACHMENT(key);

    buffer_reset(&connection->client_buffer);
    buffer_write(&connection->client_buffer, SOCKS5_VERSION);
    buffer_write(&connection->client_buffer, connection->client.reply.rep);
    buffer_write(&connection->client_buffer, RSV);
    buffer_write(&connection->client_buffer, connection->atyp);
    // if (connection->client.request.atyp == FQDN) {
    //     buffer_write(&connection->client_buffer, strlen((char *)connection->origin_host));
    // }

    if (connection->client_addr.ss_family == AF_INET) {
        struct sockaddr_in * s = (struct sockaddr_in *) &connection->client_addr;
        for (int i = 0; i < 4; i++) {
            buffer_write(&connection->client_buffer, ((uint8_t *)&s->sin_addr.s_addr)[i]);
        }
    } else if (connection->client_addr.ss_family == AF_INET6) {
        struct sockaddr_in6 * s = (struct sockaddr_in6 *) &connection->client_addr;
        for (int i = 0; i < 16; i++) {
            buffer_write(&connection->client_buffer, s->sin6_addr.s6_addr[i]);
        }
    }
    // else if (connection->client.request.atyp == FQDN) {
    //     buffer_write(&connection->client_buffer, strlen((char *)connection->origin_host));
    //     for (size_t i = 0; i < strlen((const char *) connection->origin_host); i++) {
    //         buffer_write(&connection->client_buffer, connection->origin_host[i]);
    //     }
    // }

    buffer_write(&connection->client_buffer, htons(atoi((const char *) connection->origin_port)) >> 8);
    buffer_write(&connection->client_buffer, htons(atoi((const char *) connection->origin_port)) & 0xFF);
}

unsigned reply_write(struct selector_key * key) {
    struct socks5 * connection = ATTACHMENT(key);
    uint8_t * rPtr;
    size_t toRead, written;
    
    rPtr = buffer_read_ptr(&connection->client_buffer, &toRead);
    written = send(connection->client_fd, rPtr, toRead, 0);
    buffer_read_adv(&connection->client_buffer, written);

    if (written < 0) {
        LOG_FATAL("Send failed (REPLY)");
        return ERROR;
    }

    if (buffer_can_read(&connection->client_buffer)) {
        return REPLY;
    }
    
    buffer_reset(&connection->client_buffer);
    
    if (connection->client.reply.rep == SUCCEDED) {
        LOG_DEBUG("Replied successfully");
        return COPY;
    }

    selector_set_interest(key->s, connection->client_fd, OP_READ);
    return REQUEST;
}
