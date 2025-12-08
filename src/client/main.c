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
	uint8_t authMsg[1 + 1 + 255 + 1 + 255];
	inet_pton(AF_INET, "10.0.0.9", &addr.sin_addr);

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
		perror("send greeting");
		close(sock);
		exit(1);
	}

	uint8_t greetingResponse[2];
    recv(sock, greetingResponse, 2, 0);
	if (greetingResponse[1] != 0x02) { 
		printf("Server does not accept USERNAME/PASSWORD, closing.\n");
		close(sock);
		return 1;
	}
	printf("Server reply: VER=%d METHOD=0x%02x\n", greetingResponse[0], greetingResponse[1]);
	printf("Handshake: server requires USERNAME/PASSWORD\n");


	// <----------------------------------------- AUTH ----------------------------------------->
	int idx = 0;
	authMsg[idx++] = 0x01;    
	authMsg[idx++] = ulen;    
	memcpy(&authMsg[idx], user, ulen);
	idx += ulen;
	authMsg[idx++] = plen;    
	memcpy(&authMsg[idx], pass, plen);
	idx += plen;

	// Send auth
	send(sock, authMsg, idx, 0);

	uint8_t authResp[2];
	recv(sock, authResp, 2, 0);

	if (authResp[1] == 0x00) {
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


	// <----------------------------------------- IPv4 REQUEST ----------------------------------------->
	uint8_t request[] = {
		0x05, 					// VER (5)
		0x01, 					// CMD (connect)
		0x00, 					// RSV
		0x01, 					// ATYP (ipv4)
		0x0A, 0x00, 0x00, 0x6F, // DST. ADDR (10.0.0.111)
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

    if (reply[1] == 0x00) {
		printf("Request successful!\n");
	} else {
		printf("Request failed (code=0x%02X)\n", reply[1]);
	}


	// <----------------------------------------- FQDN REQUEST ----------------------------------------->
	// uint8_t request[] = {
	// 	0x05, 											// VER (5)
	// 	0x01, 											// CMD (connect)
	// 	0x00, 											// RSV
	// 	0x03, 											// ATYP (fqdn)
	// 	0x0B,                   						// LEN (11)
    // 	'e','x','a','m','p','l','e','.','c','o','m', 	// DOMAIN (example.com)
	// 	0x00, 0x50										// PORT (80)
	// };

	// if(0 > send(sock, request, sizeof(request), 0)) {
	// 	perror("send request");
	// 	close(sock);
	// 	exit(1);
	// }

	// uint8_t reply[4];
	// if (recv(sock, reply, 4, MSG_WAITALL) != 4) {
	// 	perror("recv header");
	// 	return 1;
	// }

	// if (reply[1] == 0x00) {
	// 	printf("Request successful!\n");
	// } else {
	// 	printf("Request failed (code=0x%02X)\n", reply[1]);
	// }


	// <----------------------------------------- REQUEST DATA ----------------------------------------->

	// Build HTTP request
	const char *httpReq =
		"GET /test_file_01.txt HTTP/1.1\r\n"
		"Host: 10.0.0.111\r\n"
		"\r\n";

	// Send HTTP request through SOCKS5 tunnel
	if (send(sock, httpReq, strlen(httpReq), 0) < 0) {
		perror("send HTTP");
		close(sock);
		exit(1);
	}

	// Receive response
	char buffer[4096] = {0};
	int n;

	printf("----- BEGIN RESPONSE -----\n");

	while ((n = recv(sock, buffer, sizeof(buffer)-1, 0)) > 0) {
		buffer[n] = '\0';
		printf("%s", buffer);
		if (strstr(buffer, "\r\n\r\n") != NULL) {
			break;
		}
	}

	printf("\n----- END RESPONSE -----\n");

	if (n < 0) perror("recv HTTP");

	printf("Connection closed by server.\n");

	printf("DATA: %s\n", buffer);

	// <----------------------------------------- FINISH ----------------------------------------->
    close(sock);

	return 0;
}
