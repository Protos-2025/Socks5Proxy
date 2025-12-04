#include "include/request.h"
#include "include/socks5nio.h"
#include <stdio.h>
#include <sys/socket.h>
#include <netdb.h>

#define CONNECT 0x01
#define BIND 0x02
#define UDP_ASSOCIATE 0x03
#define RSV 0x00
#define IPv4_ADDR 0x01
#define FQDN 0x03
#define IPv6_ADDR 0x04

#define IS_VALID_CMD(c) ((c) == CONNECT || (c) == BIND || (c) == UDP_ASSOCIATE)
#define IS_VALID_ATYP(c) ((c) == IPv4_ADDR || (c) == FQDN || (c) == IPv6_ADDR)
#define ADDR_BYTES_BY_IP_VERSION(v) ((v) == IPv4_ADDR ? 4 : 16)

static unsigned resolve_dst_address(struct socks5 * connection);

void request_arrival(struct selector_key * key) {
    struct socks5 * connection = ATTACHMENT(key);

    connection->client.request.state = VER_CMD_ATYP;
}

unsigned request_read(struct selector_key * key) {
    struct socks5 * connection = ATTACHMENT(key);

    if (connection->client.request.state == VER_CMD_ATYP) {
        uint8_t * w_ptr;
        size_t readn, to_write, to_read;

        w_ptr = buffer_write_ptr(&connection->client_buffer, &to_write);
        readn = recv(key->fd, w_ptr, to_write, 0);
    
        if (readn < 0) {
            // TODO: handle error correctly
            perror("recv failed (REQUEST)");
            return ERROR;
        }
        if (readn == 0) {
            fprintf(stdout, "Client closed connection (REQUEST)\n");
            return DONE;
        }
    
        buffer_write_adv(&connection->client_buffer, readn);

        buffer_read_ptr(&connection->client_buffer, &to_read);
        if (to_read < 4) {
            return REQUEST;
        }
    
        if (SOCKS5_VERSION != buffer_read(&connection->client_buffer)) {
            fprintf(stdout, "Unsupported version\n");
            return ERROR;
        }
    
        connection->client.request.cmd = buffer_read(&connection->client_buffer);
        if (!IS_VALID_CMD(connection->client.request.cmd)) {
            fprintf(stdout, "Invalid cmd\n");
            return ERROR;
        }
    
        if (RSV != buffer_read(&connection->client_buffer)) {
            fprintf(stdout, "Invalid rsv\n");
            return ERROR;
        }
    
        connection->client.request.atyp = buffer_read(&connection->client_buffer);
        if (!IS_VALID_ATYP(connection->client.request.atyp)) {
            fprintf(stdout, "Invalid atyp\n");
            return ERROR;
        }
    
        connection->client.request.state = DST_LEN;
    }

    return resolve_dst_address(connection);
}

static unsigned resolve_dst_address(struct socks5 * connection) {
    uint8_t * w_ptr, r_ptr;
    uint8_t addr_bytes;
    size_t readn, to_write, to_read;

    w_ptr = buffer_write_ptr(&connection->client_buffer, &to_write);
    readn = recv(connection->client_fd, w_ptr, to_write, 0);

    if (connection->client.request.state == DST_LEN) {
        if (readn < 0) {
            // TODO: handle error correctly
            perror("recv failed (REQUEST)");
            return ERROR;
        }
        if (readn == 0) {
            fprintf(stdout, "Client closed connection (REQUEST)\n");
            return DONE;
        }
    
        buffer_write_adv(&connection->client_buffer, readn);
        
        buffer_read_ptr(&connection->client_buffer, &to_read);
    
        if (connection->client.request.atyp == FQDN) {
            if (to_read < 1) {
                return REQUEST;
            }
            addr_bytes = buffer_read(&connection->client_buffer);
            if (addr_bytes == 0) {
                fprintf(stdout, "Invalid fqdn lenght\n");
                return ERROR;
            }
            // TODO: manage dns resolution
        } else {
            addr_bytes = ADDR_BYTES_BY_IP_VERSION(connection->client.request.atyp);
            if (to_read < addr_bytes) {
                return REQUEST;
            }
        }

        connection->client.request.state = DST_RES;
    }
}

static unsigned resolve_fqdn(struct socks5 * connection) {
    // struct addrinfo hints;
    // struct addrinfo * result, * rp;

    // memset(&hints, 0, sizeof(hints));
    // hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
    // hints.ai_socktype = SOCK_STREAM;
    // hints.ai_flags = 0;
    // hints.ai_protocol = 0;
    // hints.ai_canonname = NULL;
    // hints.ai_addr = NULL;
    // hints.ai_next = NULL;

    // if (0 != getaddrinfo("google.com", "80", &hints, &result)) {
    //     perror("getaddrinfo failed");
    //     return ERROR;
    // }

    // int i;
    // for (rp = result, i = 0; rp != NULL; rp = rp->ai_next, i++) {
    //     connection->origin_fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    //     if (connection->origin_fd == -1) {
    //         continue;
    //     }

    //     if (connect(connection->origin_fd, rp->ai_addr, rp->ai_addrlen) != -1) {
    //         break;
    //     }

    //     close(connection->origin_fd);
    // }

    // freeaddrinfo(result);

    // if (rp == NULL) {
    //     perror("connect failed (REPLY)\n");
    //     return ERROR;
    // }

    // return ;
}

unsigned request_write(struct selector_key * key) {

}
