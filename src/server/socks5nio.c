#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>

#include "logger.h"
#include "include/metrics.h"
#include "include/socks5nio.h"
#include "../shared/include/selector.h"

#define N(x) (sizeof(x)/sizeof((x)[0]))

static const struct state_definition socks5States[] = {
    {
        .state = GREETING,
        .on_arrival = greeting_arrival,
        .on_read_ready = greeting_read,
        .on_write_ready = greeting_write
    },
    {
      .state = AUTH,          
        .on_arrival = auth_arrival,
        .on_read_ready = auth_read,
        .on_write_ready = auth_write,
    },
    {
        .state = REQUEST,
        .on_arrival = request_arrival,
        .on_read_ready = request_read,
        .on_block_ready = request_block
    },
    {
        .state = CONNECT,
        .on_write_ready = connect_write
    },
    {
        .state = REPLY,
        .on_arrival = reply_arrival,
        .on_write_ready = reply_write
    },
    {
        .state = COPY,
        .on_read_ready = socksv5_copy_read,
        .on_write_ready = socksv5_copy_write,
        .on_arrival = socksv5_copy_arrival,
        .on_departure = socksv5_copy_close,
    },
    {
        .state = DONE
    },
    {
        .state = ERROR
    }
};

static struct socks5 * socks5_new(int client_fd);
static void socksv5_done(struct selector_key * key);
static void socks5_destroy(struct selector_key * key);

void socksv5_passive_accept(struct selector_key * key) {
    struct sockaddr_storage clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);
    struct socks5 * connection = NULL;
    key->data = NULL;

    const int clientFd = accept(key->fd, (struct sockaddr *) &clientAddr, &clientAddrLen);
    if (clientFd == -1) {
        goto fail;
    }
    if (selector_fd_set_nio(clientFd) == -1) {
        goto fail;
    }

    char clientIp[INET6_ADDRSTRLEN] = {0};
    uint16_t port;
    if (clientAddr.ss_family == AF_INET) {
        struct sockaddr_in * s = (struct sockaddr_in *) &clientAddr;
        inet_ntop(AF_INET, &s->sin_addr, clientIp, INET6_ADDRSTRLEN);
        port = s->sin_port;
    } else if (clientAddr.ss_family == AF_INET6) {
        struct sockaddr_in6 * s = (struct sockaddr_in6 *) &clientAddr;
        inet_ntop(AF_INET6, &s->sin6_addr, clientIp, INET6_ADDRSTRLEN);
        port = s->sin6_port;
    }
    LOG_INFO("Accepted connection from %s:%hu", clientIp, ntohs(port));
    register_new_connection();

    connection = socks5_new(clientFd);

    if (connection == NULL) {
        // sin un estado, nos es imposible manejaro.
        // tal vez deberiamos apagar accept() hasta que detectemos
        // que se liberó alguna conexión.
        goto fail;
    }
    memcpy(&connection->client_addr, &clientAddr, clientAddrLen);
    connection->client_addr_len = clientAddrLen;

    if (SELECTOR_SUCCESS != selector_register(key->s, clientFd, &socks5Handler, OP_READ, connection)) {
        goto fail;
    }
    
    return ;
fail:
    if (clientFd != -1) {
        close(clientFd);
    }
    socks5_destroy(key);
}

static struct socks5 * socks5_new(int client_fd) {
    struct socks5 * new = malloc(sizeof(struct socks5));
    if (new != NULL) {
        new->client_fd = client_fd;
        buffer_init(&new->client_buffer, BUFFER_SIZE, new->client_buffer_data);
        
        new->origin_fd = -1;
        buffer_init(&new->origin_buffer, BUFFER_SIZE, new->origin_buffer_data);
        new->origin_resolution = NULL;

        new->atyp = 0;

        new->stm = (struct state_machine){
            .initial = GREETING,
            .states = socks5States,
            .max_state = ERROR,
            .current = NULL,
        };
        stm_init(&new->stm);

        new->closed = false;
    }

    return new;
}

void socksv5_read(struct selector_key *key) {
    struct state_machine * stm = &ATTACHMENT(key)->stm;
    const enum socks_v5state st = stm_handler_read(stm, key);

    if(ERROR == st || DONE == st) {
        socksv5_done(key);
    }
}

void socksv5_write(struct selector_key * key) {
    struct state_machine * stm   = &ATTACHMENT(key)->stm;
    const enum socks_v5state st = stm_handler_write(stm, key);

    if(ERROR == st || DONE == st) {
        socksv5_done(key);
    }
}

void socksv5_block(struct selector_key * key) {
    struct state_machine *stm   = &ATTACHMENT(key)->stm;
    const enum socks_v5state st = stm_handler_block(stm, key);

    if(ERROR == st || DONE == st) {
        socksv5_done(key);
    }
}

void socksv5_close(struct selector_key * key) {
    struct state_machine *stm   = &ATTACHMENT(key)->stm;
    stm_handler_close(stm, key);
    register_connection_closed();
    socksv5_done(key);
}

static void socksv5_done(struct selector_key * key) {
    struct socks5 * connection = ATTACHMENT(key);

    if (connection->closed) {
        return;
    }

    char * origin = (char *) ATTACHMENT(key)->origin_host;
    char clientIp[INET6_ADDRSTRLEN] = {0};
    if (ATTACHMENT(key)->client_addr.ss_family == AF_INET) {
        struct sockaddr_in * s = (struct sockaddr_in *) &ATTACHMENT(key)->client_addr;
        inet_ntop(AF_INET, &s->sin_addr, clientIp, INET6_ADDRSTRLEN);
    } else if (ATTACHMENT(key)->client_addr.ss_family == AF_INET6) {
        struct sockaddr_in6 * s = (struct sockaddr_in6 *) &ATTACHMENT(key)->client_addr;
        inet_ntop(AF_INET6, &s->sin6_addr, clientIp, INET6_ADDRSTRLEN);
    }
    LOG_INFO("Closing connection from %s %s%s", clientIp, origin != NULL && *origin ? " to " : "", origin != NULL ? origin : "");
    
    connection->closed = true;

    if (connection->origin_fd != -1) {
        selector_unregister_fd(key->s, connection->origin_fd);
        close(connection->origin_fd);
    }
    if (connection->client_fd != -1) {
        selector_unregister_fd(key->s, connection->client_fd);
        close(connection->client_fd);
    }

    socks5_destroy(key);
}

static void socks5_destroy(struct selector_key * key) {
    if (key->data != NULL) {
        struct socks5 * connection = ATTACHMENT(key);
        if (connection->origin_resolution != NULL) {
            if (connection->atyp == IPv4_ADDR || connection->atyp == IPv6_ADDR) {
                free(connection->origin_resolution->ai_addr);
                free(connection->origin_resolution);
            } else if (connection->atyp == FQDN) {
                freeaddrinfo(connection->origin_resolution);
            }
            connection->origin_resolution = NULL;
        }
        free(connection);
        key->data = NULL;
    }
}
