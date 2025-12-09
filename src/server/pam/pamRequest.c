#include "../include/pam.h"
#include "../include/pamRequest.h"
#include "../include/pamMethods.h"

#include "buffer.h"
#include <stdint.h>
#include <stdio.h>
#include <sys/socket.h>
#include "logger.h"
#include "defines.h"
#include <arpa/inet.h>
#include <stdlib.h>

void pam_request_arrival(const unsigned state, struct selector_key * key) {

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
        perror("recv failed (PAM)");
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

        connection->client.request.state = READ_BODY;
    }
    
    if (connection->client.request.state == READ_BODY) {
        buffer_write_ptr(&connection->client_buffer, &toRead);
        if (toRead < connection->client.request.read_nbody) {
            return PAM_REQUEST;
        }

        for (size_t i = 0; i < connection->client.request.read_nbody; i++) {
            connection->client.request.read_body[i] = buffer_read(&connection->client_buffer);
        }

        LOG_DEBUG("PAM request body received");

        // TODO: add handling of the request
        // TODO: handle method
        buffer_reset(&connection->client_buffer);
        buffer_write(&connection->client_buffer, PAM_VERSION_1);
        buffer_write(&connection->client_buffer, connection->client.request.status);
        buffer_write(&connection->client_buffer, connection->client.request.write_nbody);
 
        // writes on client.request.write_body
        handle_pam_request_method(connection);
        buffer_write_adv(&connection->client_buffer, connection->client.request.write_nbody); 
      
      selector_set_interest_key(key, OP_WRITE);
    }


    return PAM_REQUEST;
}
unsigned pam_request_write(struct selector_key * key){


    return PAM_REQUEST;
}
