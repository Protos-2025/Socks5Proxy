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
    LOG_TRACE("CONNECT: Processing...");
    struct socks5 * connection = ATTACHMENT(key);
    LOG_WARN("Data on client buffer: %.*s", (int)(connection->client_buffer.limit - connection->client_buffer.data), connection->client_buffer.data);
    LOG_WARN("Data on origin buffer: %.*s", (int)(connection->origin_buffer.limit - connection->origin_buffer.data), connection->origin_buffer.data);
    
    int optVal, ret;
    socklen_t optLen = sizeof(optVal);
    ret = getsockopt(connection->origin_fd, SOL_SOCKET, SO_ERROR, &optVal, &optLen);
    
    if (ret == -1) {
        connection->client.reply.rep = SERVER_FAILURE;
        // LOG_DEBUG("CONNECT: getsockopt failed... (errno = %d, fd = %d)", errno, connection->origin_fd);
    } else if (optVal != 0) {
        selector_unregister_fd_without_closing(key->s, connection->origin_fd);
        close(connection->origin_fd);
        connection->origin_fd = -1;

        if (connection->origin_resolution->ai_next == NULL) {
            switch (optVal) {
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
                default:
                    LOG_WARN("General failure connecting to origin");
                    connection->client.reply.rep = SERVER_FAILURE;
                    break;
            }
            selector_set_interest(key->s, connection->client_fd, OP_WRITE);
            return REPLY;
        }
        
        // LOG_DEBUG("CONNECT: couldnt connect, trying again...");
        get_next_resolution(key);
        
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
                LOG_INFO("Connected to origin: %s:%s", connection->origin_host, connection->origin_port);
                selector_set_interest(key->s, connection->origin_fd, OP_NOOP);
                connection->client.reply.rep = SUCCEDED;
            }
        };
    } else {
        LOG_INFO("Connected to origin: %s:%s", connection->origin_host, connection->origin_port);
        selector_set_interest(key->s, connection->origin_fd, OP_NOOP);
        connection->client.reply.rep = SUCCEDED;
    }

    selector_set_interest(key->s, connection->client_fd, OP_WRITE);
    return REPLY;
}
