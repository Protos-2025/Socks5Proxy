#include <stdio.h>
#include "client_utils.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h> // htons()
#include <unistd.h>

int send_request(uint8_t ver, uint16_t method, uint16_t n_body, char *body, int sock) {

    uint8_t reserved = 0x00;

    int headerSize = 1 + 1 + 2 + 2; // ver + reserved + method + n_body
    int totalSize = headerSize + n_body;

    uint8_t *requestBuffer = malloc(totalSize);
    if (!requestBuffer) return -1;

    int i = 0;

    requestBuffer[i++] = ver;

    requestBuffer[i++] = reserved;

    uint16_t methodNet = htons(method);
    memcpy(&requestBuffer[i], &methodNet, sizeof(uint16_t));
    i += 2;

    uint16_t nbNet = htons(n_body);
    memcpy(&requestBuffer[i], &nbNet, sizeof(uint16_t));
    i += 2;

    if (n_body > 0 && body != NULL) {
        memcpy(&requestBuffer[i], body, n_body);
        i += n_body;
    }
    else if(body == NULL) {
        char * aux = 0x00;
        memcpy(&requestBuffer[i], aux, n_body);
        i += n_body;
    }

    send(sock, requestBuffer, totalSize, 0);

    free(requestBuffer); 
    return 0;
}

ServerResponse* receive_response(int sock) {
    ServerResponse *resp = malloc(sizeof(ServerResponse));
    if (!resp) return NULL;

    // Receive header: VER (1) + STATUS (1) + NBODY (2)
    uint8_t header[4];
    ssize_t n = recv(sock, header, 4, 0);
    if (n < 4) {
        fprintf(stderr, "Error: Failed to receive response header\n");
        free(resp);
        return NULL;
    }

    int i = 0;
    resp->ver = header[i++];
    resp->status = header[i++];
    
    // NBODY is in network byte order
    uint16_t nbodyNet;
    memcpy(&nbodyNet, &header[i], sizeof(uint16_t));
    resp->nbody = ntohs(nbodyNet);

    // Receive body if present
    if (resp->nbody > 0) {
        resp->body = malloc(resp->nbody);
        if (!resp->body) {
            free(resp);
            return NULL;
        }

        n = recv(sock, resp->body, resp->nbody, 0);
        if (n < resp->nbody) {
            fprintf(stderr, "Error: Failed to receive complete response body\n");
            free(resp->body);
            free(resp);
            return NULL;
        }
    } else {
        resp->body = NULL;
    }

    return resp;
}

void free_response(ServerResponse *resp) {
    if (resp) {
        if (resp->body) {
            free(resp->body);
        }
        free(resp);
    }
}

static void print_response(const char* operation, ServerResponse *resp) {
    if (!resp) {
        fprintf(stderr, "[%s] Error: No response received\n", operation);
        return;
    }

    printf("\n[%s] Response received:\n", operation);   //This is for debug-clean later
    printf("  VER:    0x%02X\n", resp->ver);
    printf("  STATUS: 0x%02X (%s)\n", resp->status, 
           resp->status == 0x00 ? "Success" : "Failed");
    printf("  NBODY:  %d bytes\n", resp->nbody);
    
    if (resp->body && resp->nbody > 0) {
        printf("  BODY:   ");
        fwrite(resp->body, 1, resp->nbody, stdout);
        fflush(stdout);
    }
    printf("\n");
}

int get_users(struct ClientArgs* args, int sock){

    uint16_t outLen = 0x0000;
    uint8_t* body = NULL;

    send_request(PROTOCOL_VERSION, METHOD_USERS, outLen, (char*)body, sock);
    
    ServerResponse *resp = receive_response(sock);
    print_response("GET_USERS", resp);
    
    free_response(resp);
    return 0;
}

int add_user(struct ClientArgs* args, int sock){
    uint8_t ulen = strlen(args->arg1);
    uint8_t plen = strlen(args->arg2);
    uint8_t role = (uint8_t)atoi(args->arg3);

    uint16_t outLen = 0x0000;
    uint8_t* body = NULL;

    uint16_t bodyLen = 1 + ulen + 1 + plen + 1;
    body = malloc(bodyLen);
    
    int idx = 0;
    body[idx++] = ulen;
    memcpy(&body[idx], args->arg1, ulen);
    idx += ulen;
    body[idx++] = plen;
    memcpy(&body[idx], args->arg2, plen);
    idx += plen;
    body[idx++] = role;
    
    outLen = bodyLen;
    send_request(PROTOCOL_VERSION, METHOD_ADD_USER, outLen, (char*)body, sock);
    
    ServerResponse *resp = receive_response(sock);
    print_response("ADD_USER", resp);
    
    free(body);
    free_response(resp);
    return 0;
}

int remove_user(struct ClientArgs* args, int sock){

    uint16_t outLen = 0x0000;
    uint8_t* body = NULL;

    uint8_t ulen = strlen(args->arg1);
    uint16_t bodyLen = 1 + ulen;
    body = malloc(bodyLen);

    int idx = 0;
    body[idx++] = ulen;
    memcpy(&body[idx], args->arg1, ulen);
    outLen = bodyLen;
    send_request(PROTOCOL_VERSION, METHOD_REMOVE_USER, outLen, (char*)body, sock);
    
    ServerResponse *resp = receive_response(sock);
    print_response("REMOVE_USER", resp);

    free(body);
    free_response(resp);
    return 0;
}

int change_password(struct ClientArgs* args, int sock){
    uint8_t ulen = strlen(args->arg1);
    uint8_t nPlen = strlen(args->arg2);

    uint16_t outLen = 0x0000;
    uint8_t* body = NULL;

    uint16_t bodyLen = 1 + ulen + 1 + nPlen;
    body = malloc(bodyLen);
    
    int idx = 0;
    body[idx++] = ulen;
    memcpy(&body[idx], args->arg1, ulen);
    idx += ulen;
    body[idx++] = nPlen;
    memcpy(&body[idx], args->arg2, nPlen);
    idx += nPlen;
    
    outLen = bodyLen;
    send_request(PROTOCOL_VERSION, METHOD_CHANGE_PASSWORD, outLen, (char*)body, sock);
    
    ServerResponse *resp = receive_response(sock);
    print_response("CHANGE_PASSWORD", resp);

    free(body);
    free_response(resp);
    return 0;
}

int change_role(struct ClientArgs* args, int sock){
    uint8_t ulen = strlen(args->arg1);
    uint8_t nRole = (uint8_t)atoi(args->arg2);

    uint16_t outLen = 0x0000;
    uint8_t* body = NULL;

    uint16_t bodyLen = 1 + ulen + 1;
    body = malloc(bodyLen);

    int idx = 0;
    body[idx++] = ulen;
    memcpy(&body[idx], args->arg1, ulen);
    idx += ulen;
    body[idx++] = nRole;

    outLen = bodyLen;
    send_request(PROTOCOL_VERSION, METHOD_CHANGE_ROLE, outLen, (char*)body, sock);
    
    ServerResponse *resp = receive_response(sock);
    print_response("CHANGE_ROLE", resp);

    free(body);
    free_response(resp);
    return 0;
}

int get_metrics(struct ClientArgs* args, int sock){
    uint16_t outLen = 0x0000;
    uint8_t* body = NULL;

    send_request(PROTOCOL_VERSION, METHOD_METRICS, outLen, (char*)body, sock);
    
    ServerResponse *resp = receive_response(sock);
    print_response("GET_METRICS", resp);
    
    free_response(resp);
    return 0;
}
