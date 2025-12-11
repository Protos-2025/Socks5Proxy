#include "../include/pam.h"
#include "../include/pamRequest.h"
#include "../include/pamMethods.h"

#include "buffer.h"
#include <stdint.h>
#include <stdio.h>
#include <sys/socket.h>
#include "logger.h"
#include "selector.h"
#include <arpa/inet.h>
#include <stdlib.h>

void pam_request_arrival(const unsigned state, struct selector_key * key) {
    LOG_DEBUG("PAM REQUEST state arrival");
    struct pam * connection = PAM_ATTACHMENT(key);
    connection->client.request.state = VER_N_RESERVED;
    connection->client.request.reserved = RESERVED_BYTE; 
}

unsigned pam_request_read(struct selector_key * key){
    struct pam * connection = PAM_ATTACHMENT(key); 

    uint8_t * wPtr;
    size_t count, toRead;
    ssize_t readn;

    wPtr = buffer_write_ptr(&connection->client_buffer, &count);
    readn = recv(key->fd, wPtr, count, 0);

    if (readn < 0) {
        // TODO: handle error correctly
        LOG_FATAL("recv failed (PAM)");
        return PAM_ERROR;
    }
    if (readn == 0) {
        LOG_DEBUG("Client closed connection (PAM)\n");
        return PAM_DONE;
    }

    buffer_write_adv(&connection->client_buffer, readn);

    // parse version and reserved bytes
    if(connection->client.request.state == VER_N_RESERVED) {
        buffer_write_ptr(&connection->client_buffer, &toRead);

        if (toRead < 2) {
            return PAM_REQUEST;
        }

        //version
        uint8_t ver = buffer_read(&connection->client_buffer);
        if (ver != PAM_VERSION_1) {
            LOG_DEBUG("Unsupported pam version");
            return PAM_ERROR;
        }

        uint8_t reserved = buffer_read(&connection->client_buffer);
        if (reserved != connection->client.request.reserved) {
            LOG_DEBUG("Reserved byte not received as 0x00");
            return PAM_ERROR;
        }

        connection->client.request.state = METHOD;
    }

    if(connection->client.request.state == METHOD) {
        buffer_write_ptr(&connection->client_buffer, &toRead);
        if (toRead < 2) {
            return PAM_REQUEST;
        }
        
        uint8_t *ptr = buffer_read_ptr(&connection->client_buffer, &toRead);
        uint16_t method = *(uint16_t *)ptr;
        connection->client.request.method = ntohs(method);
        
        LOG_DEBUG("Received PAM request method: 0x%04X", connection->client.request.method); 

        buffer_read_adv(&connection->client_buffer, 2);
        connection->client.request.state = NBODY;
    }

    if (connection->client.request.state == NBODY) {
        buffer_write_ptr(&connection->client_buffer, &toRead);
        if (toRead < 2) {
            return PAM_REQUEST;
        }

        uint8_t *ptr = buffer_read_ptr(&connection->client_buffer, &toRead);
        uint16_t nbody = *(uint16_t *)ptr;
        connection->client.request.read_nbody = ntohs(nbody);

        LOG_DEBUG("PAM request body length: %u", connection->client.request.read_nbody);

        buffer_read_adv(&connection->client_buffer, 2);
        connection->client.request.state = READ_BODY;
    }
    
    if (connection->client.request.state == READ_BODY) {
        buffer_write_ptr(&connection->client_buffer, &toRead);
        if (toRead < connection->client.request.read_nbody) {
            return PAM_REQUEST;
        }

        //===========================================================================

        for (size_t i = 0; i < connection->client.request.read_nbody; i++) {
            connection->client.request.read_body[i] = buffer_read(&connection->client_buffer);
        }
        connection->client.request.read_body[connection->client.request.read_nbody] = '\0';

        Buffer buffer;
        buffer_init(&buffer, PAM_BUFFER_SIZE, connection->client.request.read_body);
        buffer_write_adv(&buffer, connection->client.request.read_nbody);

        LOG_DEBUG("PAM request body received");
        
        // writes on client.request.write_body
        handle_pam_request_method(connection, &buffer);

        buffer_reset(&connection->client_buffer);
        buffer_write(&connection->client_buffer, PAM_VERSION_1);
        buffer_write(&connection->client_buffer, connection->client.request.status);
        
        uint16_t nbodyNet = htons(connection->client.request.write_nbody);
        uint8_t *nbodyBytes = (uint8_t *)&nbodyNet;
        buffer_write(&connection->client_buffer, nbodyBytes[0]);
        buffer_write(&connection->client_buffer, nbodyBytes[1]);

        for (size_t i = 0; i < connection->client.request.write_nbody; i++) {
            buffer_write(&connection->client_buffer, connection->client.request.write_body[i]);
        }

        selector_set_interest_key(key, OP_WRITE);
    }

    return PAM_REQUEST;
}

unsigned pam_request_write(struct selector_key * key) {
    struct pam * connection = PAM_ATTACHMENT(key);
    uint8_t * rPtr;
	size_t toRead;
	int written;

	rPtr = buffer_read_ptr(&connection->client_buffer, &toRead);
    written = send(connection->client_fd, rPtr, toRead, 0);
    buffer_read_adv(&connection->client_buffer, written);

    if (written < 0) {
        LOG_FATAL("send failed (PAM_REQUEST)");
        return PAM_ERROR;
    }

    if (buffer_can_read(&connection->client_buffer)) {
        return PAM_REQUEST;
    }

    LOG_INFO("Pam request completed");

    buffer_reset(&connection->client_buffer);

    selector_set_interest_key(key, OP_NOOP); 
    return PAM_DONE;
}
