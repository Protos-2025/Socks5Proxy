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
#include "client_utils.h"

#define PROTOCOL_VERSION 0x01

int main(int argc, char* argv[]) {

    // ----------------------------------------------- PARSE ARGS ------------------------------------------------

    struct ClientArgs args;

    int r = parse_client_args(argc, argv, &args);
    if (r != 0) {
        // help already printed / or error printed by parse_args
        return (r == 1 ? 0 : -1);
    }
    if(args.user == NULL || args.password == NULL){
        printf("[Error]: user or password missing.");
        print_help(argv[0]);
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
        fprintf(stderr, "Error: Invalid host: %s\n", args.host);
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

    // --------------------------------------------- AUTH --------------------------------------------------------->

    const char *user = args.user;
	const char *pass = args.password;

    size_t ulen = strlen(user);
    size_t plen = strlen(pass);

    if (ulen > 255 || plen > 255) {
        fprintf(stderr, "Error: User or Password too long\n");
        return -1;
    }

	size_t total = 3 + ulen + plen;
    uint8_t *authMsg = malloc(total);

    int idx = 0;
	authMsg[idx++] = 0x01; 
	authMsg[idx++] = ulen;    
	authMsg[idx++] = plen;    
	memcpy(&authMsg[idx], user, ulen);
    idx += ulen;
	memcpy(&authMsg[idx], pass, plen);
	idx += plen;
    printf("Sending authentication...\n");
    // Send auth
	send(sock, authMsg, idx, 0);
    free(authMsg);
	uint8_t authResp[2];
	recv(sock, authResp, 2, 0);

    if (authResp[1] == 0x00) {
		printf("Authentication successful\n");
	} else {
		printf("Error: Authentication failed!\n");
		close(sock);
		return 1;
	}

    printf("Server response to authentication: VER=0x%02X, STATUS=0x%02X\n", authResp[0], authResp[1]);

    // --------------------------------------------- REQUEST ------------------------------------------------------>

    if (strcmp(args.option, "users") == 0) {
        get_users(&args, sock);
    }
    else if (strcmp(args.option, "add-user") == 0) {
        add_user(&args, sock);
    }   
    else if (strcmp(args.option, "remove-user") == 0) {
        remove_user(&args, sock);
    }
    else if (strcmp(args.option, "change-password") == 0) {
       change_password(&args, sock);
    }
    else if (strcmp(args.option, "change-role") == 0) {
        change_role(&args, sock);
    }
    else if (strcmp(args.option, "metrics") == 0) {
        get_metrics(&args, sock);
    }

	// <----------------------------------------- FINISH ----------------------------------------->
    
	close(sock);

	return 0;
}