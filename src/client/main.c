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


	// <----------------------------------------- IPv4 REQUEST ----------------------------------------->
	// uint8_t request[] = {
	// 	0x05, 					// VER (5)
	// 	0x01, 					// CMD (connect)
	// 	0x00, 					// RSV
	// 	0x01, 					// ATYP (ipv4)
	// 	0x8E, 0xFA, 0x40, 0x6E, // DST. ADDR (142.250.64.110)
	// 	0x00, 0x50				// DST. PORT (80)
	// };

	// if(0 > send(sock, request, sizeof(request), 0)) {
	// 	perror("send request");
	// 	close(sock);
	// 	exit(1);
	// }

	// uint8_t reply[10];
    // if (recv(sock, reply, 10, MSG_WAITALL) != 10) {
    //     printf("reply recv failed\n");
    //     return 1;
    // }

    // if (reply[1] == 0x00) {
	// 	printf("Request successful!\n");
	// } else {
	// 	printf("Request failed (code=0x%02X)\n", reply[1]);
	// }


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
	
	
	// <----------------------------------------- IPv6 REQUEST ----------------------------------------->
	uint8_t request[] = {
		0x05,                               // VER (5)
		0x01,                               // CMD (connect)
		0x00,                               // RSV
		0x04,                               // ATYP (ipv6)
		0x26, 0x20, 0x00, 0xFE,				// DST. ADDR (2620:00fe:0000:0000:0000:0000:0000:00fe)
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0xFE,
		0x00, 0x35                			// DST.PORT = 53
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


	// <----------------------------------------- FINISH ----------------------------------------->
    close(sock);

	return 0;
}
