#include <stdio.h>
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
	uint8_t auth_msg[1 + 1 + 255 + 1 + 255];
	inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

	if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
		perror("connect");
		exit(1);
	}
	printf("Connected to SOCKS5 server!\n");

	uint8_t greeting[] = {
		0x05, // SOCKS version
		0x01, // Number of authentication methods supported
		0x02  // No authentication
	};

	if(send(sock, greeting, sizeof(greeting), 0) < 0) {
		perror("send greeting");
		close(sock);
		exit(1);
	}

	uint8_t response[2];
    recv(sock, response, 2, 0);
	if (response[1] != 0x02) { 
    printf("Server does not accept USERNAME/PASSWORD, closing.\n");
    close(sock);
    return 1;
	}
	printf("Server reply: VER=%d METHOD=0x%02x\n", response[0], response[1]);
	printf("Handshake: server requires USERNAME/PASSWORD\n");

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

    close(sock);
	
	return test();
}
