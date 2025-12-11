#ifndef CLIENT_UTILS_H
#define CLIENT_UTILS_H

#include <stdint.h>
#include "client_args.h"

#define PROTOCOL_VERSION 0x01
#define METHOD_USERS 0x0001
#define METHOD_ADD_USER 0x0002
#define METHOD_REMOVE_USER 0x0003
#define METHOD_CHANGE_PASSWORD 0x0004
#define METHOD_CHANGE_ROLE 0x0005
#define METHOD_METRICS 0x0006

typedef struct {
    uint8_t ver;
    uint8_t status;
    uint16_t nbody;
    uint8_t *body;
} ServerResponse;

int send_request(uint8_t ver, uint16_t method, uint16_t n_body, char *body, int sock);
ServerResponse* receive_response(int sock);
void free_response(ServerResponse *resp);
int detect_ip_version(const char *host);

int get_users(struct ClientArgs* args, int sock);
int add_user(struct ClientArgs* args, int sock);
int remove_user(struct ClientArgs* args, int sock);
int change_password(struct ClientArgs* args, int sock);
int change_role(struct ClientArgs* args, int sock);
int get_metrics(struct ClientArgs* args, int sock);

#endif