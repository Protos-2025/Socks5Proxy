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

int main() {
	int sock = socket(AF_INET, SOCK_STREAM, 0);

	struct sockaddr_in addr = {
		.sin_family = AF_INET,
		.sin_port = htons(1080),    
	};

	char *user = "admin";
	char *pass = "password";
	uint8_t ulen = strlen(user);
	uint8_t plen = strlen(pass);
	uint8_t auth_msg[64]; // Sufficient size for auth message (might whant to check bounds in the future)
	inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

	if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
		perror("connect");
		exit(1);
	}
	printf("Connected to SOCKS5 server!\n");


	// <----------------------------------------- GREETING ----------------------------------------->
	uint8_t greeting[] = {
		0x05, // SOCKS version
		0x01, // Number of authentication methods supported
		0x02  // username/password auth
	};

	if(send(sock, greeting, sizeof(greeting), 0) < 0) {
		printf("Error send greeting\n");
		close(sock);
		exit(1);
	}


	uint8_t greeting_response[2];
    recv(sock, greeting_response, 2, 0);
	if (greeting_response[1] != 0x02) { 
		printf("Server does not accept USERNAME/PASSWORD, closing.\n");
		close(sock);
		return 1;
	}
	printf("Server reply: VER=%d METHOD=0x%02x\n", greeting_response[0], greeting_response[1]);
	printf("Handshake: server requires USERNAME/PASSWORD\n");


	// <----------------------------------------- AUTH ----------------------------------------->
	int idx = 0;
	auth_msg[idx++] = 0x01;    
	auth_msg[idx++] = ulen;    
	memcpy(&auth_msg[idx], user, ulen);
	idx += ulen;
	auth_msg[idx++] = plen;    
	memcpy(&auth_msg[idx], pass, plen);
	idx += plen;

	// Send auth
	send(sock, auth_msg, idx, 0);

	uint8_t auth_resp[2];
	recv(sock, auth_resp, 2, 0);

	if (auth_resp[1] == 0x00) {
		printf("Authentication successful!\n");
	} else {
		printf("Authentication failed!\n");
		close(sock);
		return 1;
	}

	// if (response[1] != 0x00) {
    //     printf("Server does not accept NO AUTH, closing.\n");
    //     close(sock);
    //     return 1;
    // }

    printf("Handshake successful!\n");


	// <----------------------------------------- REQUEST ----------------------------------------->
	uint8_t request[] = {
		0x05, 					// VER (5)
		0x01, 					// CMD (connect)
		0x00, 					// RSV
		0x01, 					// ATYP (ipv4)
		0x8E, 0xFA, 0x40, 0x6E, // DST. ADDR (142.250.64.110)
		0x00, 0x50				// DST. PORT (80)
	};

	if(0 > send(sock, request, sizeof(request), 0)) {
		perror("send request");
		close(sock);
		exit(1);
	}

	uint8_t reply[10];
    if (recv(sock, reply, 10, MSG_WAITALL) != 10) {
        printf("reply recv failed\n");
        return 1;
    }

    printf("Reply code = 0x%02X (%s)\n", reply[1], reply[1] == 0x00 ? "succeded" : "failed");

	// <----------------------------------------- FINISH ----------------------------------------->
    close(sock);

	return 0;
}
