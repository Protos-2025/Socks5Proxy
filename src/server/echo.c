#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8081
#define BUFFER_SIZE 1024

int main(void) {
    int server_fd, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE] = {0};
    ssize_t bytes_read;

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

    if ((client_socket = accept(server_fd, (struct sockaddr *) &client_addr, &client_addr_len)) == -1) {
        perror("accept failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
    printf("Accepted connection from %s:%d\n", client_ip, ntohs(client_addr.sin_port));

    while ((bytes_read = read(client_socket, buffer, BUFFER_SIZE)) > 0) {
        printf("Received %zd bytes from client\n", bytes_read);

        if (write(client_socket, buffer, bytes_read) == -1) {
            perror("write failed\n");
            break;
        }

        memset(buffer, 0, BUFFER_SIZE);
    }

    if (bytes_read == -1) {
        perror("read failed\n");
    } else {
        printf("Client closed the connection.\n");
    }

    close(client_socket);
    close(server_fd);

    return 0;
}
