#include "include/greeting.h"
#include "include/socks5nio.h"
#include <stdio.h>
#include <sys/socket.h>

#define NO_AUTH_METHOD 0x00
#define AUTH_METHOD 0x02

void greeting_arrival(const unsigned state, struct selector_key * key) {
    struct socks5 * connection = ATTACHMENT(key);
    connection->client.greeting.state = VER_N_NMETHODS;
    connection->client.greeting.method = NO_AUTH_METHOD;
}

unsigned greeting_read(struct selector_key * key) {
    struct socks5 * connection = ATTACHMENT(key);
    uint8_t * w_ptr;
    size_t count, readn, to_read;

    w_ptr = buffer_write_ptr(&connection->client_buffer, &count);
    readn = recv(key->fd, w_ptr, count, 0);

    if (readn < 0) {
        // TODO: handle error correctly
        perror("recv failed (GREETING)");
        return ERROR;
    }
    if (readn == 0) {
        fprintf(stdout, "Client closed connection (GREETING)\n");
        return DONE;
    }

    buffer_write_adv(&connection->client_buffer, readn);

    if (connection->client.greeting.state == VER_N_NMETHODS) {
        buffer_read_ptr(&connection->client_buffer, &to_read);

        if (to_read < 2) {
            return GREETING;
        }
    
        // check version
        uint8_t ver = buffer_read(&connection->client_buffer);
        if (ver != SOCKS5_VERSION) {
            fprintf(stdout, "Unsupported version\n");
            return ERROR;
        }
    
        // check methods count
        connection->client.greeting.n_methods = buffer_read(&connection->client_buffer);
        if (connection->client.greeting.n_methods == 0) {
            fprintf(stdout, "Invalid amount of methods\n");
            return ERROR;
        }

        connection->client.greeting.state = METHODS;
    }

    if (connection->client.greeting.state == METHODS) {
        buffer_read_ptr(&connection->client_buffer, &to_read);

        if (to_read < connection->client.greeting.n_methods) {
            return GREETING;
        }

        bool found_auth_method = false;
        for (int i = 0; i < connection->client.greeting.n_methods && buffer_can_read(&connection->client_buffer) && !found_auth_method; i++) {
            if (AUTH_METHOD == buffer_read(&connection->client_buffer)) {
                connection->client.greeting.method = AUTH_METHOD;
                found_auth_method = true;
                fprintf(stdout, "Auth method chosen\n");
            }
        }

        if (found_auth_method) {
            // TODO
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
    uint8_t * r_ptr;
    size_t to_read, written;
    
    r_ptr = buffer_read_ptr(&connection->client_buffer, &to_read);
    written = send(connection->client_fd, r_ptr, to_read, 0);
    buffer_read_adv(&connection->client_buffer, written);

    if (written < 0) {
        perror("send failed (GREETING)");
        return ERROR;
    }

    if (buffer_can_read(&connection->client_buffer)) {
        return GREETING;
    }

    fprintf(stdout, "Greeting completed\n");

    buffer_reset(&connection->client_buffer);

    // TODO: replace both lines once request_read is implemented
    selector_set_interest_key(key, OP_NOOP); // selector_set_interest_key(key, OP_READ);
    return DONE; // return REQUEST;
}
