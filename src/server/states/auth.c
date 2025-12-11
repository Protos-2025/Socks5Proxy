#include "../include/auth.h"
#include "../include/socks5nio.h"
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include "logger.h"
#include "../include/users.h"

#define AUTH_VERSION 0x01
#define AUTH_SUCCESS 0x00
#define AUTH_FAILURE 0x01

void auth_arrival(const unsigned state, struct selector_key * key) {
    struct socks5 * connection = ATTACHMENT(key);
    connection->client.auth.state = AUTH_VER;
    connection->client.auth.ulen = 0;
    connection->client.auth.plen = 0;
    connection->client.auth.authenticated = false;
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

    buffer_write_adv(&connection->client_buffer, readn);
    buffer_read_ptr(&connection->client_buffer, &toRead);

    if (toRead == 0
        || (readn == 0
            && ((connection->client.auth.state == AUTH_UNAME && (toRead < connection->client.auth.ulen))
                || (connection->client.auth.state == AUTH_PASSWD && (toRead < connection->client.auth.plen)))
            )) {
        LOG_DEBUG("Client closed connection (AUTH)");
        return DONE;
    }

    // Read auth version
    if (connection->client.auth.state == AUTH_VER) {
        uint8_t ver = buffer_read(&connection->client_buffer);
		toRead--;
		if (ver != AUTH_VERSION) {
			LOG_WARN("Invalid auth version: 0x%02X", ver);
            return ERROR;
		}

		connection->client.auth.state = AUTH_ULEN;
    }

    if (connection->client.auth.state == AUTH_ULEN) {
        if (toRead < 1) {
            return AUTH;
        }

        connection->client.auth.ulen = buffer_read(&connection->client_buffer);
		toRead--;
		if (connection->client.auth.ulen == 0 || connection->client.auth.ulen > 255) {
			LOG_WARN("Invalid username length: %d", connection->client.auth.ulen);
            return ERROR;
		}

		connection->client.auth.state = AUTH_UNAME;
    }

    // Read username
    if (connection->client.auth.state == AUTH_UNAME) {
        if (toRead < connection->client.auth.ulen) {
            return AUTH;
        }

        for (int i = 0; i < connection->client.auth.ulen; i++) {
            connection->client.auth.username[i] = buffer_read(&connection->client_buffer);
        }
        connection->client.auth.username[connection->client.auth.ulen] = '\0';
        toRead -= connection->client.auth.ulen;

        LOG_DEBUG("Username received: %s", connection->client.auth.username);
        connection->client.auth.state = AUTH_PLEN;
    }

    if (connection->client.auth.state == AUTH_PLEN) {
        if (toRead < 1) {
            return AUTH;
        }

        connection->client.auth.plen = buffer_read(&connection->client_buffer);
        toRead--;
        if (connection->client.auth.plen == 0 || connection->client.auth.plen > 255) {
            LOG_WARN("Invalid password length: %d", connection->client.auth.plen);
            return ERROR;
        }

        connection->client.auth.state = AUTH_PASSWD;
    }

    // Read password
    if (connection->client.auth.state == AUTH_PASSWD) {
        if (toRead < connection->client.auth.plen) {
            return AUTH;
        }

        for (int i = 0; i < connection->client.auth.plen; i++) {
            connection->client.auth.password[i] = buffer_read(&connection->client_buffer);
        }
        connection->client.auth.password[connection->client.auth.plen] = '\0';
        toRead -= connection->client.auth.plen;

        LOG_TRACE("Password received (length: %d)", connection->client.auth.plen);

        // Check credentials
        switch (user_authenticate((const uint8_t *)connection->client.auth.username, (const uint8_t *)connection->client.auth.password)) {
            case USER_BADUSERNAME:
            case USER_WRONGPASSWORD:
                LOG_WARN("Authentication failed for user: %s", connection->client.auth.username);
                break;
            
            case USER_OK:
            default: {
                connection->client.auth.authenticated = true;
                LOG_DEBUG("Authentication successful for user: %s", connection->client.auth.username);
                break;
            }
        }

        buffer_write(&connection->origin_buffer, AUTH_VERSION);
        buffer_write(&connection->origin_buffer, connection->client.auth.authenticated ? AUTH_SUCCESS : AUTH_FAILURE);

        selector_set_interest_key(key, OP_WRITE);
    }

    return AUTH;
}

unsigned auth_write(struct selector_key * key) {
    LOG_TRACE("Writing auth response...");
    struct socks5 * connection = ATTACHMENT(key);
    uint8_t * rPtr;
    size_t toRead;
    ssize_t written; 
    
    rPtr = buffer_read_ptr(&connection->origin_buffer, &toRead);
    written = send(connection->client_fd, rPtr, toRead, 0);
    buffer_read_adv(&connection->origin_buffer, written);

    if (written < 0) {
        LOG_FATAL("send failed (AUTH)");
        return ERROR;
    }

    if (buffer_can_read(&connection->origin_buffer)) {
        return AUTH;
    }

    // If auth fails, close connection
    if (!connection->client.auth.authenticated) {
        LOG_DEBUG("Closing connection due to authentication failure");
        return ERROR;
    }

    LOG_DEBUG("Authentication completed successfully");
    
    selector_set_interest_key(key, OP_READ);

    return REQUEST;
}