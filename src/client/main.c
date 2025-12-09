#include <stdio.h>      
#include <stdlib.h>     
#include <string.h>     
#include <sys/types.h>  
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <arpa/inet.h>  
#include <unistd.h> 
#include "logs.h"
#include "protocol.h"
#include "client_args.h"

int main(int argc, char* argv[]) {

    // ----------------------------------------------- PARSE ARGS ------------------------------------------------

    struct ClientArgs args;

    int r = parse_client_args(argc, argv, &args);
    if (r != 0) {
        // help already printed / or error printed by parse_args
        return (r == 1 ? 0 : -1);
    }

    // ------------------------------------------- CONNECTION ----------------------------------------------------->

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return -1;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(args.port);

    if (inet_pton(AF_INET, args.host, &addr.sin_addr) <= 0) {
        fprintf(stderr, "Invalid host: %s\n", args.host);
        close(sock);
        return -1;
    }
	printf("Connecting to host %s and port %d...\n", args.host, args.port);
    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("connect");
        close(sock);
        return -1;
    }

    printf("Connected to server at %s:%d\n", args.host, args.port);

    // --------------------------------------------- REQUEST ------------------------------------------------------>

    printf("Option '%s' selected.\n", args.option);

    if (strcmp(args.option, "users") == 0) {
        // TODO: implement
    }
    else if (strcmp(args.option, "add-user") == 0) {
        // args.arg1 = username
        // args.arg2 = password
        // args.arg3 = role
    }
    else if (strcmp(args.option, "remove-user") == 0) {
        // args.arg1 = username
    }
    else if (strcmp(args.option, "change-password") == 0) {
        // args.arg1 = username
        // args.arg2 = password
    }
    else if (strcmp(args.option, "change-rol") == 0) {
        // args.arg1 = username
        // args.arg2 = role
    }
    else if (strcmp(args.option, "metrics") == 0) {
        // no args
    }
	// <----------------------------------------- FINISH ----------------------------------------->
    
	close(sock);

	return 0;
}
