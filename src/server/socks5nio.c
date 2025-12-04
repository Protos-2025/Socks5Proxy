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
#include "include/socks5nio.h"
#include "../shared/include/selector.h"

#define N(x) (sizeof(x)/sizeof((x)[0]))

static const struct state_definition socks5_states[] = {
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
        .state = DONE
    },
    {
        .state = ERROR
    }
};

static struct socks5 * socks5_new(int client_fd);
static void socksv5_done(struct selector_key* key);
static void socks5_destroy(struct socks5 * s);

void socksv5_passive_accept(struct selector_key * key) {
    struct sockaddr_storage client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    struct socks5 * connection = NULL;

    const int client_fd = accept(key->fd, (struct sockaddr *) &client_addr, &client_addr_len);
    if (client_fd == -1) {
        goto fail;
    }
    if (selector_fd_set_nio(client_fd) == -1) {
        goto fail;
    }

    struct sockaddr_in * s = (struct sockaddr_in *) &client_addr;
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &s->sin_addr, client_ip, INET_ADDRSTRLEN); // TODO manage IPv6
    LOG_INFO("Accepted connection from %s:%d", client_ip, ntohs(s->sin_port));

    connection = socks5_new(client_fd);

    if (connection == NULL) {
        // sin un estado, nos es imposible manejaro.
        // tal vez deberiamos apagar accept() hasta que detectemos
        // que se liberó alguna conexión.
        goto fail;
    }
    memcpy(&connection->client_addr, &client_addr, client_addr_len);
    connection->client_addr_len = client_addr_len;

    if (SELECTOR_SUCCESS != selector_register(key->s, client_fd, &socks5_handler, OP_READ, connection)) {
        goto fail;
    }
    
    return ;
fail:
    if (client_fd != -1) {
        close(client_fd);
    }
    socks5_destroy(connection);
}

static struct socks5 * socks5_new(int client_fd) {
    struct socks5 * new = malloc(sizeof(struct socks5));
    if (new != NULL) {
        new->client_fd = client_fd;
        buffer_init(&new->client_buffer, BUFFER_SIZE, new->client_buffer_data);
        buffer_init(&new->origin_buffer, BUFFER_SIZE, new->origin_buffer_data);
        new->origin_fd = -1;
        new->origin_resolution = NULL;
        new->origin_resolutions_list = NULL;
        new->stm = (struct state_machine){
            .initial = GREETING,
            .states = socks5_states,
            .max_state = ERROR,
            .current = NULL,
        };
        stm_init(&new->stm);

        new->references = 1;
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
    socks5_destroy(ATTACHMENT(key));
}

static void socksv5_done(struct selector_key * key) {
    const int fds[] = {
        ATTACHMENT(key)->client_fd,
        ATTACHMENT(key)->origin_fd,
    };
    for (unsigned i = 0; i < N(fds); i++) {
        if (fds[i] != -1) {
            if (SELECTOR_SUCCESS != selector_unregister_fd(key->s, fds[i])) {
                abort();
            }
            close(fds[i]);
        }
    }
}

static void socks5_destroy(struct socks5 * s) {
    if(s->origin_resolution != NULL) {
        freeaddrinfo(s->origin_resolution);
        s->origin_resolution = 0;
    }
    free(s);
}

/**
 * destruye un  `struct socks5', tiene en cuenta las referencias
 * y el pool de objetos.
 */
// static void
// socks5_destroy(struct socks5 *s) {
//     if(s == NULL) {
//         // nada para hacer
//     } else if(s->references == 1) {
//         if(s != NULL) {
//             if(pool_size < max_pool) {
//                 s->next = pool;
//                 pool    = s;
//                 pool_size++;
//             } else {
//                 _(s);
//             }
//         }
//     } else {
//         s->references -= 1;
//     }
// }

// void
// socksv5_pool_destroy(void) {
//     struct socks5 * next, *s;
//     for(s = pool; s != NULL ; s = next) {
//         next = s->next;
//         free(s);
//     }
// }
