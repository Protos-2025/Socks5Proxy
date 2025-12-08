#include "../include/reply.h"
#include "../include/socks5nio.h"
#include "../../shared/include/buffer.h"
#include "logger.h"

#define RSV 0x00
// #define FQDN 0x03

void reply_arrival(const unsigned state, struct selector_key * key) {
    LOG_DEBUG("Replying...");

    struct socks5 * connection = ATTACHMENT(key);

    buffer_reset(&connection->client_buffer);
    buffer_write(&connection->client_buffer, SOCKS5_VERSION);
    buffer_write(&connection->client_buffer, connection->client.reply.rep);
    buffer_write(&connection->client_buffer, RSV);
    buffer_write(&connection->client_buffer, connection->client.request.atyp);
    // if (connection->client.request.atyp == FQDN) {
    //     buffer_write(&connection->client_buffer, strlen((char *)connection->origin_host));
    // }
    for (int i = 0; connection->origin_host[i] != '\0' && i < HOST_MAX_LENGHT; i++) {
        buffer_write(&connection->client_buffer, connection->origin_host[i]);
    }
    buffer_write(&connection->client_buffer, connection->origin_port[0]);
    buffer_write(&connection->client_buffer, connection->origin_port[1]);
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

    LOG_INFO("Replied successfully");
    
    buffer_reset(&connection->client_buffer);

    if (connection->client.reply.rep == SUCCEDED) {
        return DONE; // TODO: change to COPY
    }

    selector_set_interest(key->s, connection->client_fd, OP_READ);
    return REQUEST;
}
