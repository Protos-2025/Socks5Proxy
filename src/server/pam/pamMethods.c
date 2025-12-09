#include "../include/pamMethods.h"
#include <string.h>
#include "logger.h"
#include "../include/users.h"
#include "../include/pam.h"

void handle_get_connected_users_list(struct pam * connection);
void handle_add_user(struct pam * connection);
void handle_remove_user(struct pam * connection);
void handle_change_password(struct pam * connection);
void handle_change_role(struct pam * connection);
void handle_get_metrics(struct pam * connection);
void handle_reset_historical_connections(struct pam * connection);
void handle_reset_historical_bytes(struct pam * connection);


void handle_pam_request_method(struct pam * connection) {
    switch(connection->client.request.method) {
        case PAM_REQUEST_METHOD_GET_CONNECTED_USERS_LIST:
            handle_get_connected_users_list(connection);
            break;
        case PAM_REQUEST_METHOD_ADD_USER:
            handle_add_user(connection);
            break;
        case PAM_REQUEST_METHOD_REMOVE_USER:
            handle_remove_user(connection);
            break;
        case PAM_REQUEST_METHOD_CHANGE_PASSWORD:
            handle_change_password(connection);
            break;
        case PAM_REQUEST_METHOD_CHANGE_ROLE:
            handle_change_role(connection);
            break;
        case PAM_REQUEST_METHOD_GET_METRICS:
            handle_get_metrics(connection);
            break;
        case PAM_REQUEST_METHOD_RESET_HISTORICAL_CONNECTIONS:
            handle_reset_historical_connections(connection);
            break;
        case PAM_REQUEST_METHOD_RESET_HISTORICAL_BYTES:
            handle_reset_historical_bytes(connection);
            break;
        default:
            LOG_WARN("Invalid PAM request method: 0x%04X", connection->client
                     .request.method);
            connection->client.request.status = PAM_SERVER_ERROR;
            connection->client.request.write_nbody = 0;
            break;
    }
}

void handle_get_connected_users_list(struct pam * connection) {
    LOG_DEBUG("Handling get connected users list request");
    char buffer[BUFFER_SIZE];
    int copied = users_get_connected_users_list(buffer);
    if (copied < 0) {
        LOG_ERROR("Error getting connected users list");
        connection->client.request.status = PAM_SERVER_ERROR;
        connection->client.request.write_nbody = 0;
    } else {
        memcpy(connection->client.request.write_body, buffer, copied);
        connection->client.request.write_nbody = copied;
    }
}

void handle_add_user(struct pam * connection) {


}

void handle_remove_user(struct pam * connection) {


}

void handle_change_password(struct pam * connection) {


}

void handle_change_role(struct pam * connection) {


}

void handle_get_metrics(struct pam * connection) {


}

void handle_reset_historical_connections(struct pam * connection) {


}

void handle_reset_historical_bytes(struct pam * connection) {


}
