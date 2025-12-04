#include "../include/connect.h"
#include "../include/socks5nio.h"
#include "../include/connection_utils.h"
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <stdlib.h>

unsigned connect_write(struct selector_key * key) {
    struct socks5 * connection = ATTACHMENT(key);

    int opt_val, ret;
    socklen_t opt_len = sizeof(opt_val);
    if ((ret = getsockopt(connection->origin_fd, SOL_SOCKET, SO_ERROR, &opt_val, &opt_len)) == -1) {
        connection->client.reply.rep = SERVER_FAILURE;
    } else if (opt_val != 0) {
        switch (try_connection(key)) {
            case NO_RESOLUTION_FOUND:
                connection->client.reply.rep = HOST_UNREACHABLE;

            case SELECTOR_REGISTER_FAILED:
                connection->client.reply.rep = SERVER_FAILURE;
    
            case CONNECTION_IN_PROGRESS:
                return CONNECT;
            
            default: {
                struct addrinfo * copy = malloc(sizeof(struct addrinfo));
                if (copy == NULL) {
                    connection->client.reply.rep = SERVER_FAILURE;
                    break;
                }
                memcpy(copy, connection->origin_resolution, sizeof(struct addrinfo));
                copy->ai_next = NULL;
                connection->origin_resolution = copy;

                selector_set_interest_key(key, OP_NOOP);
                selector_set_interest(key->s, connection->client_fd, OP_WRITE);
                
                connection->client.reply.rep = SUCCEDED;
            }
        };
    } else {
        connection->client.reply.rep = SUCCEDED;
    }

    freeaddrinfo(connection->origin_resolutions_list);
    connection->origin_resolutions_list = NULL;

    return REPLY;
}
