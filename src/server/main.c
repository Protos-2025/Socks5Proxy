#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "../shared/include/selector.h"

#define PROXY_SERVER_PORT 8080
#define BUFFER_SIZE 1024

#define GET_BUFFER_IDX(i, n) (((i) + (n)) == BUFFER_SIZE ? 0 : ((i) + (n)))
#define GET_BUFFER_MAX_IDX_TO_READ(w) ((w) == 0 ? BUFFER_SIZE : (w))

static void handle_client_read(struct selector_key * key);
static void handle_client_write(struct selector_key * key);
static void handle_origin_read(struct selector_key * key);
static void handle_origin_write(struct selector_key * key);
static void terminate_connection(int exit_code);

typedef struct custom_key {
    uint8_t * buffer;
    uint32_t idx_to_read;
    uint32_t idx_to_write;
    struct custom_key * other_party;
} custom_key;

static int proxy_server_fd, client_fd, origin_fd;
static fd_selector selector;
static custom_key client_custom_key, origin_custom_key;

int main(void) {
    struct sockaddr_in proxy_server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    selector_status ret;
    struct addrinfo hints;
    struct addrinfo * result, * rp;
    

    // <---------------------------- create proxy server socket ---------------------------->
    if ((proxy_server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket failed (proxy server)");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    if (setsockopt(proxy_server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror("setsockopt failed");
        close(proxy_server_fd);
        exit(EXIT_FAILURE);
    }

    proxy_server_addr.sin_family = AF_INET;
    proxy_server_addr.sin_addr.s_addr = INADDR_ANY;
    proxy_server_addr.sin_port = htons(PROXY_SERVER_PORT);

    if (bind(proxy_server_fd, (struct sockaddr *) &proxy_server_addr, sizeof(proxy_server_addr)) == -1) {
        perror("bind failed (proxy server)");
        close(proxy_server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(proxy_server_fd, 10) == -1) {
        perror("listen failed");
        close(proxy_server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Proxy server listening on port %d\n", PROXY_SERVER_PORT);


    // <------------------------------------ accept client ------------------------------------>
    if ((client_fd = accept(proxy_server_fd, (struct sockaddr *) &client_addr, &client_addr_len)) == -1) {
        perror("accept failed");
        close(proxy_server_fd);
        exit(EXIT_FAILURE);
    }

    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
    printf("Accepted connection from %s:%d\n", client_ip, ntohs(client_addr.sin_port));


    // <---------------------------------- connect to origin ---------------------------------->
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = 0;
    hints.ai_protocol = 0;
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    if (0 != getaddrinfo("google.com", "80", &hints, &result)) {
        perror("getaddrinfo failed");
        exit(EXIT_FAILURE);
    }

    printf("Found google\n");

    int i;
    for (rp = result, i = 0; rp != NULL; rp = rp->ai_next, i++) {
        printf("Doing socket (%d)...\n", i);
        origin_fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        printf("Socket done (%d)...\n", i);
        if (origin_fd == -1) {
            printf("%d socket failed\n", i);
            continue;
        }

        printf("Doing connect (%d)...\n", i);
        if (connect(origin_fd, rp->ai_addr, rp->ai_addrlen) != -1) {
            printf("Connect done (%d)...\n", i);
            break;
        }

        close(origin_fd);
        printf("%d connect failed\n", i);
    }

    freeaddrinfo(result);

    if (rp == NULL) {
        perror("Could not connect to origin\n");
        exit(EXIT_FAILURE);
    }

    printf("Connected to google\n");


    // <--------------------------------- configure selector --------------------------------->
    client_custom_key.buffer = malloc(BUFFER_SIZE);
    client_custom_key.idx_to_read = 0;
    client_custom_key.idx_to_write = 0;
    client_custom_key.other_party = &origin_custom_key;
    origin_custom_key.buffer = malloc(BUFFER_SIZE);
    origin_custom_key.idx_to_read = 0;
    origin_custom_key.idx_to_write = 0;
    origin_custom_key.other_party = &client_custom_key;

    if ((selector = selector_new(10)) == NULL) {
        perror("selector new failed");
        free(client_custom_key.buffer);
        free(origin_custom_key.buffer);
        close(proxy_server_fd);
        close(client_fd);
        close(origin_fd);
        exit(EXIT_FAILURE);
    }

    fd_handler client_handler = {handle_client_read, handle_client_write, NULL};
    fd_handler origin_handler = {handle_origin_read, handle_origin_write, NULL};
    
    if(selector_register(selector, client_fd, &client_handler, OP_READ, &client_custom_key) != SELECTOR_SUCCESS) {
        perror("selector register (client) failed");
        terminate_connection(EXIT_FAILURE);
    }

    if(selector_register(selector, origin_fd, &origin_handler, OP_NOOP, &origin_custom_key) != SELECTOR_SUCCESS) {
        perror("selector register (origin) failed");
        terminate_connection(EXIT_FAILURE);
    }


    // <--------------------------------- execute server ---------------------------------->
    while(1) {
        ret = selector_select(selector);
        if(ret != SELECTOR_SUCCESS) {
            perror("selector select failed");
            terminate_connection(EXIT_FAILURE);
        }
    }

    return 0;
}

static void handle_client_read(struct selector_key * key) {
    custom_key * client_key_data = (custom_key *) key->data;
    int readn = read(key->fd, client_key_data->buffer + client_key_data->idx_to_write, BUFFER_SIZE - client_key_data->idx_to_write);

    if (readn == 0) {
        // TODO: manage correctly
        printf("Client closed the connection.\n");
        terminate_connection(EXIT_SUCCESS);
    }

    printf("Received %d bytes from client\n", readn);

    client_key_data->idx_to_write = GET_BUFFER_IDX(client_key_data->idx_to_write, readn);

    selector_set_interest(key->s, key->fd, OP_NOOP);
    selector_set_interest(key->s, origin_fd, OP_WRITE);
}

static void handle_client_write(struct selector_key * key) {
    custom_key * client_key_data = (custom_key *) key->data;
    int written = write(key->fd,
                        client_key_data->other_party->buffer + client_key_data->other_party->idx_to_read,
                        GET_BUFFER_MAX_IDX_TO_READ(client_key_data->other_party->idx_to_write) - client_key_data->other_party->idx_to_read);

    if (written == -1) {
        perror("write failed");
        terminate_connection(EXIT_FAILURE);
    }

    printf("Sent %d bytes to client\n", written);

    client_key_data->other_party->idx_to_read = GET_BUFFER_IDX(client_key_data->other_party->idx_to_read, written);

    selector_set_interest(key->s, key->fd, OP_READ);
}

static void handle_origin_read(struct selector_key * key) {
    custom_key * origin_key_data = (custom_key *) key->data;
    int readn = read(key->fd, origin_key_data->buffer + origin_key_data->idx_to_write, BUFFER_SIZE - origin_key_data->idx_to_write);

    if (readn == 0) {
        // TODO: manage correctly
        printf("Origin closed the connection.\n");
        terminate_connection(EXIT_SUCCESS);
    }

    printf("Received %d bytes from origin\n", readn);

    origin_key_data->idx_to_write = GET_BUFFER_IDX(origin_key_data->idx_to_write, readn);

    selector_set_interest(key->s, key->fd, OP_NOOP);
    selector_set_interest(key->s, client_fd, OP_WRITE);
}

static void handle_origin_write(struct selector_key * key) {
    custom_key * origin_key_data = (custom_key *) key->data;
    int written = write(key->fd,
                        origin_key_data->other_party->buffer + origin_key_data->other_party->idx_to_read,
                        GET_BUFFER_MAX_IDX_TO_READ(origin_key_data->other_party->idx_to_write) - origin_key_data->other_party->idx_to_read);

    if (written == -1) {
        perror("write failed");
        terminate_connection(EXIT_FAILURE);
    }

    printf("Sent %d bytes to origin\n", written);

    origin_key_data->other_party->idx_to_read = GET_BUFFER_IDX(origin_key_data->other_party->idx_to_read, written);

    selector_set_interest(key->s, key->fd, OP_READ);
}

static void terminate_connection(int exit_code) {
    selector_destroy(selector);
    free(client_custom_key.buffer);
    free(origin_custom_key.buffer);
    close(proxy_server_fd);
    close(client_fd);
    close(origin_fd);
    exit(exit_code);
}
