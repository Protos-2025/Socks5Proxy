#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "../shared/include/selector.h"

#define PORT 8080
#define BUFFER_SIZE 1024

static void handle_read(struct selector_key * key);
static void handle_write(struct selector_key * key);
static void terminate_connection(int exit_code);

typedef struct custom_key {
    uint8_t * read_buffer;
    uint32_t readn;
} custom_key;

static int server_fd, client_fd;
static fd_selector selector;
static custom_key client_custom_key;

int main(void) {
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    selector_status ret;


    // <---------------------------- create server passive socket ---------------------------->
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror("setsockopt failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) == -1) {
        perror("bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 10) == -1) {
        perror("listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Echo server listening on port %d\n", PORT);


    // <------------------------------------ accept client ------------------------------------>
    if ((client_fd = accept(server_fd, (struct sockaddr *) &client_addr, &client_addr_len)) == -1) {
        perror("accept failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
    printf("Accepted connection from %s:%d\n", client_ip, ntohs(client_addr.sin_port));


    // <--------------------------------- configure selector --------------------------------->
    client_custom_key.read_buffer = malloc(BUFFER_SIZE);
    client_custom_key.readn = 0;

    if ((selector = selector_new(10)) == NULL) {
        perror("selector new failed");
        free(client_custom_key.read_buffer);
        close(server_fd);
        close(client_fd);
        exit(EXIT_FAILURE);
    }

    fd_handler handler = {handle_read, handle_write, NULL};
    
    if((ret = selector_register(selector, client_fd, &handler, OP_READ, &client_custom_key) != SELECTOR_SUCCESS)) {
        perror("selector register failed");
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

static void handle_read(struct selector_key * key) {
    int readn = read(key->fd, ((custom_key *) key->data)->read_buffer, BUFFER_SIZE);

    if (readn == 0) {
        printf("Client closed the connection.\n");
        terminate_connection(EXIT_SUCCESS);
    }

    printf("Received %d bytes from client\n", readn);

    ((custom_key *) key->data)->readn = readn;

    selector_set_interest(key->s, key->fd, OP_WRITE);
}

static void handle_write(struct selector_key *key){
    int written = write(key->fd, ((custom_key *) key->data)->read_buffer, ((custom_key *) key->data)->readn);

    if (written == -1) {
        perror("write failed");
        terminate_connection(EXIT_FAILURE);
    }

    selector_set_interest(key->s, key->fd, OP_READ);
}

static void terminate_connection(int exit_code) {
    selector_destroy(selector);
    free(client_custom_key.read_buffer);
    close(server_fd);
    close(client_fd);
    exit(exit_code);
}
