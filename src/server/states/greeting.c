#include "../include/greeting.h"
#include "../include/socks5nio.h"
#include <stdio.h>
#include <sys/socket.h>
#include "logger.h"

#define NO_AUTH_METHOD 0x00
#define AUTH_METHOD 0x02

void greeting_arrival(const unsigned state, struct selector_key * key) {
    struct socks5 * connection = ATTACHMENT(key);
    connection->client.greeting.state = VER_N_NMETHODS;
    connection->client.greeting.method = NO_AUTH_METHOD;
}

unsigned greeting_read(struct selector_key * key) {
    struct socks5 * connection = ATTACHMENT(key);
    uint8_t * wPtr;
	size_t count, toRead;
	ssize_t readn;

	wPtr = buffer_write_ptr(&connection->client_buffer, &count);
    readn = recv(key->fd, wPtr, count, 0);

    if (readn < 0) {
        // TODO: handle error correctly
        LOG_FATAL("recv failed (GREETING)");
        return ERROR;
    }
    if (readn == 0) {
        LOG_INFO("Client closed connection (GREETING)");
        return DONE;
    }

    buffer_write_adv(&connection->client_buffer, readn);

    if (connection->client.greeting.state == VER_N_NMETHODS) {
        buffer_read_ptr(&connection->client_buffer, &toRead);

        if (toRead < 2) {
            return GREETING;
        }
    
        // check version
        uint8_t ver = buffer_read(&connection->client_buffer);
        if (ver != SOCKS5_VERSION) {
            LOG_WARN("Unsupported version");
            return ERROR;
        }
    
        // check methods count
        connection->client.greeting.n_methods = buffer_read(&connection->client_buffer);
        if (connection->client.greeting.n_methods == 0) {
            LOG_WARN("Invalid amount of methods");
            return ERROR;
        }

        connection->client.greeting.state = METHODS;
    }

    if (connection->client.greeting.state == METHODS) {
        buffer_read_ptr(&connection->client_buffer, &toRead);

        if (toRead < connection->client.greeting.n_methods) {
            return GREETING;
        }

        bool foundAuthMethod = false;
        for (int i = 0; i < connection->client.greeting.n_methods && buffer_can_read(&connection->client_buffer); i++) {
            uint8_t method = buffer_read(&connection->client_buffer);
            if (method == AUTH_METHOD) {
                connection->client.greeting.method = AUTH_METHOD;
                foundAuthMethod = true;
                LOG_DEBUG("Auth method chosen: 0x02 (USERNAME/PASSWORD)");
                break;
            } else if (method == NO_AUTH_METHOD && connection->client.greeting.method != AUTH_METHOD) {
                connection->client.greeting.method = NO_AUTH_METHOD;
                LOG_DEBUG("Auth method: 0x00 (NO AUTH) - fallback");
            }
        }

        // If no valid method found, set to no acceptable methods
        if (connection->client.greeting.method == NO_AUTH_METHOD && !foundAuthMethod) {
            LOG_WARN("No supported auth methods found");
            connection->client.greeting.method = 0xFF; // No acceptable methods
            return ERROR;
        }
        
        buffer_reset(&connection->client_buffer);
        buffer_write(&connection->client_buffer, SOCKS5_VERSION);
        buffer_write(&connection->client_buffer, connection->client.greeting.method);
        
        selector_set_interest_key(key, OP_WRITE);
    }

    return GREETING;
}

unsigned greeting_write(struct selector_key * key) {
    struct socks5 * connection = ATTACHMENT(key);
    uint8_t * rPtr;
	size_t toRead;
	int written;

	rPtr = buffer_read_ptr(&connection->client_buffer, &toRead);
    written = send(connection->client_fd, rPtr, toRead, 0);
    buffer_read_adv(&connection->client_buffer, written);

    if (written < 0) {
        LOG_FATAL("send failed (GREETING)");
        return ERROR;
    }

    if (buffer_can_read(&connection->client_buffer)) {
        return GREETING;
    }

    LOG_INFO("Greeting completed");

    buffer_reset(&connection->client_buffer);

  
    if (connection->client.greeting.method == AUTH_METHOD) {
        selector_set_interest_key(key, OP_READ);
        return AUTH;
    }

    // If no auth set, go to request state
    selector_set_interest_key(key, OP_READ);
    return REQUEST;
}
