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
	uint8_t auth_msg[64]; // Sufficient size for auth message (might whant to check bounds in the future)
	inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

	if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
		printf("Error conecting to socket\n");
		exit(1);
	}
	printf("Connected to SOCKS5 server!\n");

	uint8_t greeting[] = {
		0x05, // SOCKS version
		0x01, // Number of authentication methods supported
		0x03  // USR/PASS authentication
	};

	if(send(sock, greeting, sizeof(greeting), 0) < 0) {
		printf("Error send greeting\n");
		close(sock);
		exit(1);
	}

	uint8_t response[2];
	ssize_t bytes_recived = recv(sock, response, 2, 0);
	if (bytes_recived <= 0) {
		printf("No respone bytes recived, error\n");
		close(sock);
		exit(1);
	}
	if (bytes_recived != 2) {
		printf("Expected 2 bytes, got %zd\n", bytes_recived);
		close(sock);
		exit(1);
	}
	if (response[1] != 0x02) { 
    printf("Error, server response %d, closing.\n", response[1]);
    close(sock);
    return 1;
	}
	printf("Server reply: VER=%d METHOD=0x%d\n", response[0], response[1]);
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
	ssize_t sent = send(sock, auth_msg, idx, 0);
	if (sent < 0) {
		printf("Error sending auth\n");
		close(sock);
		exit(1);
	}
	if (sent != idx) {
		printf("Partial send: %zd of %d bytes\n", sent, idx);
	}

	uint8_t auth_resp[2];
	recv(sock, auth_resp, 2, 0);
	bytes_recived = recv(sock, auth_resp, 2, 0);
	if (bytes_recived <= 0) {
		printf("No response bytes recived, error\n");
		close(sock);
		exit(1);
	}
	if (bytes_recived != 2) {
		printf("Expected 2 bytes in auth response, got %zd\n", bytes_recived);
		close(sock);
		exit(1);
	}
	printf("Server reply: VER=%d STATE=0x%d\n", auth_resp[0], auth_resp[1]);
	if (auth_resp[1] == 0x00) {
		printf("Authentication successful!\n");
	} else {
		printf("Authentication failed!\n");
		close(sock);
		return 1;
	}

    printf("Handshake successful!\n");

    close(sock);
	
	return test();
}
