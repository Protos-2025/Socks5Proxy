#include "../include/connect.h"
#include "../include/socks5nio.h"
#include "../include/connection_utils.h"
#include "logger.h"
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <stdlib.h>
#include <errno.h>

unsigned connect_write(struct selector_key * key) {
    // LOG_DEBUG("CONNECT: Processing...");
    
    struct socks5 * connection = ATTACHMENT(key);

    int optVal, ret;
    socklen_t optLen = sizeof(optVal);
    ret = getsockopt(connection->origin_fd, SOL_SOCKET, SO_ERROR, &optVal, &optLen);
    
    if (ret == -1) {
        connection->client.reply.rep = SERVER_FAILURE;
        // LOG_DEBUG("CONNECT: getsockopt failed... (errno = %d, fd = %d)", errno, connection->origin_fd);
    } else if (optVal != 0) {
        // LOG_DEBUG("CONNECT: couldnt connect, trying again...");
        
        int ret = try_connection(key);
        switch (ret) {
            case GENERAL_FAILURE:
                LOG_WARN("General failure connecting to origin");
                connection->client.reply.rep = SERVER_FAILURE;
                break;
            case SELECTOR_REGISTER_FAILED:
                connection->client.reply.rep = SERVER_FAILURE;
                break;
            case NETWORK_UNREACHABLE:
                LOG_WARN("Network unreachable");
                connection->client.reply.rep = ret;
				break;
			case HOST_UNREACHABLE:
                LOG_WARN("Host unreachable");
                connection->client.reply.rep = ret;
                break;
			case CONNECTION_REFUSED:
                LOG_WARN("Connection refused");
                connection->client.reply.rep = ret;
                break;
            case TTL_EXPIRED:
                LOG_WARN("TTL expired");
                connection->client.reply.rep = ret;
                break;
    
            case CONNECTION_IN_PROGRESS:
                LOG_TRACE("Connection in progress");
                return CONNECT; 
            
            default: {
                // LOG_DEBUG("CONNECT: connection succeded after trying again");
                struct addrinfo * copy = malloc(sizeof(struct addrinfo));
                if (copy == NULL) {
                    LOG_DEBUG("REQUEST: setting SERVER_FAILURE (12)");
                    connection->client.reply.rep = SERVER_FAILURE;
                    break;
                }
                memcpy(copy, connection->origin_resolution, sizeof(struct addrinfo));
                copy->ai_next = NULL;
                connection->origin_resolution = copy;
    
                selector_set_interest(key->s, connection->origin_fd, OP_NOOP);
    
                LOG_INFO("Connected to origin: %s:%s", connection->origin_host, connection->origin_port);
                connection->client.reply.rep = SUCCEDED;
            }
        };
    } else {
        LOG_INFO("Connected to origin: %s:%s", connection->origin_host, connection->origin_port);
        connection->client.reply.rep = SUCCEDED;
    }

    connection->origin_resolutions_list = NULL;

    return REPLY;
}
