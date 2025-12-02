#ifndef SOCKS5NIO_H_
#define SOCKS5NIO_H_

#include "defines.h"
#include "../../shared/include/buffer.h"
#include "greeting.h"
#include "auth.h" 
#include "copy.h"

// #include "request.h"
#include "../../shared/include/stm.h"
#include <sys/socket.h>

#define HOST_MAX_LENGHT 256
#define PORT_MAX_LENGHT 6
#define USERNAME_MAX_LENGHT 256
#define PASSWORD_MAX_LENGHT 256

#define ATTACHMENT(key) ( (struct socks5 *)(key)->data)

enum socks_v5state {
    GREETING,
    AUTH,
    REQUEST,
    RESPONSE,
    DNS_RESOLUTION,
    COPY,
    DONE,
    ERROR,
};

/*
 * Si bien cada estado tiene su propio struct que le da un alcance
 * acotado, disponemos de la siguiente estructura para hacer una única
 * alocación cuando recibimos la conexión.
 *
 * Se utiliza un contador de referencias (references) para saber cuando debemos
 * liberarlo finalmente, y un pool para reusar alocaciones previas.
 */
struct socks5 {
    /* client */
    int client_fd;
    buffer client_buffer;
    uint8_t client_buffer_data[BUFFER_SIZE];
    struct sockaddr_storage client_addr;
    socklen_t client_addr_len;
    union {
        struct greeting_st greeting;
        struct auth_st auth;
        // struct request_st request;
        struct copy_st copy;
    } client;
    uint8_t username[USERNAME_MAX_LENGHT];
    uint8_t password[PASSWORD_MAX_LENGHT];

    /* origin */
    int origin_fd;
    char origin_host[HOST_MAX_LENGHT];
    char origin_port[PORT_MAX_LENGHT];
    struct addrinfo * origin_resolution;
    buffer origin_buffer;
    uint8_t origin_buffer_data[BUFFER_SIZE];
    union {
        // struct connecting conn;
        struct copy_st copy;
    } origin_st;

    struct state_machine stm;

    int references;

    // TODO: use this?
    // struct socks5 * next;
};

void socksv5_passive_accept(struct selector_key * key);

#endif
