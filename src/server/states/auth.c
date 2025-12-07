#include "../include/auth.h"
#include "../include/socks5nio.h"
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include "logger.h"

#define AUTH_VERSION 0x01
#define AUTH_SUCCESS 0x00
#define AUTH_FAILURE 0x01

// Todo: replace with valid mechanism (harcodaeads por ahora)
#define VALID_USERNAME "admin"
#define VALID_PASSWORD "password"

void auth_arrival(const unsigned state, struct selector_key * key) {
    struct socks5 * connection = ATTACHMENT(key);
    connection->client.auth.state = AUTH_VER;
    connection->client.auth.ulen = 0;
    connection->client.auth.plen = 0;
}

unsigned auth_read(struct selector_key * key) {
    struct socks5 * connection = ATTACHMENT(key);
    uint8_t * wPtr;
    size_t count, toRead;
    ssize_t readn; 

    wPtr = buffer_write_ptr(&connection->client_buffer, &count);
    readn = recv(key->fd, wPtr, count, 0);

    if (readn < 0) {
        LOG_FATAL("failed (AUTH)");
        return ERROR;
    }
    if (readn == 0) {
        LOG_INFO("Client closed connection (AUTH)");
        return DONE;
    }

    buffer_write_adv(&connection->client_buffer, readn);

    // Read auth version
    if (connection->client.auth.state == AUTH_VER) {
        buffer_read_ptr(&connection->client_buffer, &toRead);

        if (toRead < 1) {
            return AUTH;
        }

        uint8_t ver = buffer_read(&connection->client_buffer);
        if (ver != AUTH_VERSION) {
            LOG_WARN("Invalid auth version: 0x%02X", ver);
            return ERROR;
        }

        connection->client.auth.state = AUTH_ULEN;
    }

    if (connection->client.auth.state == AUTH_ULEN) {
        buffer_read_ptr(&connection->client_buffer, &toRead);

        if (toRead < 1) {
            return AUTH;
        }

        connection->client.auth.ulen = buffer_read(&connection->client_buffer);
        if (connection->client.auth.ulen == 0 || connection->client.auth.ulen > 255) { 
            LOG_WARN("Invalid username length: %d", connection->client.auth.ulen);
            return ERROR;
        }

        connection->client.auth.state = AUTH_UNAME;
    }

    // Read username
    if (connection->client.auth.state == AUTH_UNAME) {
        buffer_read_ptr(&connection->client_buffer, &toRead);

        if (toRead < connection->client.auth.ulen) {
            return AUTH;
        }

        for (int i = 0; i < connection->client.auth.ulen; i++) {
            connection->client.auth.username[i] = buffer_read(&connection->client_buffer);
        }
        connection->client.auth.username[connection->client.auth.ulen] = '\0';

        LOG_DEBUG("Username received: %s", connection->client.auth.username);
        connection->client.auth.state = AUTH_PLEN;
    }

    if (connection->client.auth.state == AUTH_PLEN) {
        buffer_read_ptr(&connection->client_buffer, &toRead);

        if (toRead < 1) {
            return AUTH;
        }

        connection->client.auth.plen = buffer_read(&connection->client_buffer);
        if (connection->client.auth.plen == 0 || connection->client.auth.plen > 255) {
            LOG_WARN("Invalid password length: %d", connection->client.auth.plen);
            return ERROR;
        }

        connection->client.auth.state = AUTH_PASSWD;
    }

    // Read password
    if (connection->client.auth.state == AUTH_PASSWD) {
        buffer_read_ptr(&connection->client_buffer, &toRead);

        if (toRead < connection->client.auth.plen) {
            return AUTH;
        }

        for (int i = 0; i < connection->client.auth.plen; i++) {
            connection->client.auth.password[i] = buffer_read(&connection->client_buffer);
        }
        connection->client.auth.password[connection->client.auth.plen] = '\0';

        LOG_DEBUG("Password received (length: %d)", connection->client.auth.plen);

        // Check credentials
        bool authSuccess = false;
        if (strcmp(connection->client.auth.username, VALID_USERNAME) == 0 &&
            strcmp(connection->client.auth.password, VALID_PASSWORD) == 0) {
            authSuccess = true;
            LOG_INFO("Authentication successful for user: %s", connection->client.auth.username);
        } else {
            LOG_WARN("Authentication failed for user: %s", connection->client.auth.username);
        }

        // Prepare response
        buffer_reset(&connection->client_buffer);
        buffer_write(&connection->client_buffer, AUTH_VERSION);
        buffer_write(&connection->client_buffer, authSuccess ? AUTH_SUCCESS : AUTH_FAILURE);

        connection->client.auth.authenticated = authSuccess;
        selector_set_interest_key(key, OP_WRITE);
    }

    return AUTH;
}

unsigned auth_write(struct selector_key * key) {
    struct socks5 * connection = ATTACHMENT(key);
    uint8_t * rPtr;
    size_t toRead;
    ssize_t written; 
    
    rPtr = buffer_read_ptr(&connection->client_buffer, &toRead);
    written = send(connection->client_fd, rPtr, toRead, 0);
    buffer_read_adv(&connection->client_buffer, written);

    if (written < 0) {
        LOG_FATAL("send failed (AUTH)");
        return ERROR;
    }

    if (buffer_can_read(&connection->client_buffer)) {
        return AUTH;
    }

    // If auth fails, close connection
    if (!connection->client.auth.authenticated) {
        LOG_INFO("Closing connection due to authentication failure");
        return ERROR;
    }

    LOG_INFO("Authentication completed successfully");
    buffer_reset(&connection->client_buffer);
    
    selector_set_interest_key(key, OP_READ);

    return REQUEST;
}