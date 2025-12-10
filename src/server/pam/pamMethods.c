#include "../include/pamMethods.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "logger.h"
#include "../include/users.h"
#include "../include/pam.h"
#include "../include/pamAuth.h"
#include "../include/metrics.h"
#include "../../shared/include/buffer.h"
#include "pamRequest.h"


void handle_get_connected_users_list(struct pam * connection);
void handle_add_user(struct pam * connection);
void handle_remove_user(struct pam * connection);
void handle_change_password(struct pam * connection);
void handle_change_role(struct pam * connection);
void handle_get_metrics(struct pam * connection);
void handle_reset_historical_connections(struct pam * connection);
void handle_reset_historical_bytes(struct pam * connection);


void handle_pam_request_method(struct pam * connection) {
    LOG_DEBUG("entering hanlde pam request method");
    switch(connection->client.request.method) {
        case PAM_REQUEST_METHOD_GET_CONNECTED_USERS_LIST:
            handle_get_connected_users_list(connection);
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
    char buffer[PAM_BUFFER_SIZE];
    char * initalMessage = "+OK listing users\n";
size_t len = strlen(initalMessage);
    memcpy(buffer, initalMessage, len);
    int copied = len;  

    copied += users_get_connected_users_list(buffer, copied);
    if (copied <= len) {
        LOG_ERROR("Error getting connected users list");
        connection->client.request.status = PAM_SERVER_ERROR;
        connection->client.request.write_nbody = 0;
    } else {
        memcpy(connection->client.request.write_body, buffer, copied);
        connection->client.request.write_nbody = copied;
    }
}

void handle_add_user(struct pam * connection) {
// +-------+------------+-------+------------+-------+
// | ULEN  |  USERNAME  | PLEN  |  PASSWORD  |  ROL  |
// +-------+------------+-------+------------+-------+
// |   1   |    ULEN    |   1   |    PLEN    |   1   |
// +-------+------------+-------+------------+-------+
    LOG_DEBUG("Handling add user request");

    uint8_t userLenght = buffer_read(&connection->client_buffer);

    LOG_DEBUG("username length = %d", userLenght);

    uint8_t username[PAM_AUTH_USERNAME_MAX_LENGHT];

    for (size_t i = 0; i < userLenght; i++) {
        username[i] = buffer_read(&connection->client_buffer);
    }
    username[userLenght] = '\0';

    uint8_t passLenght = buffer_read(&connection->client_buffer);

    LOG_DEBUG("username length = %d", userLenght);

    uint8_t password[PAM_AUTH_USERNAME_MAX_LENGHT];

    for (size_t i = 0; i < passLenght ; i++) {
        password[i] = buffer_read(&connection->client_buffer);
    }
    password[passLenght] = '\0';

    uint8_t role = buffer_read(&connection->client_buffer); 

    LOG_DEBUG("Recievied username %s and and password %s, role %d", username, password, role);

    connection->client.request.status = PAM_REQUEST_SUCCESS;    
    
    char * currentUserName = connection->client.auth.username;
    if(!user_is_admin(currentUserName)) {
        connection->client.request.status = PAM_REQUEST_UNAUTHORIZED;
    } else {
        UserStatus userStatus = user_create(username, password, role);
        
        switch(userStatus) {
            case USER_OK:
                LOG_INFO("Sucessfully created user with username %s, password %s and role %d", username, password, role)
                connection->client.request.status = PAM_REQUEST_SUCCESS;    
            break;
            case USER_ALREADYEXISTS:
                LOG_DEBUG("User with username %s already exists", username)
                connection->client.request.status = PAM_REQUEST_USER_ALREADY_EXISTS;    
            break; 
            case USER_WRONGPASSWORD:
                connection->client.request.status = PAM_REQUEST_USER_WRONG_PASSWORD;    
            break;
            case USER_CREDTOOLONG:
                connection->client.request.status = PAM_REQUEST_USER_CREDTOOLONG;    
            break;
            case USER_BADUSERNAME:
                connection->client.request.status = PAM_REQUEST_USER_BADUSERNAME;    
            break;
            default:
                connection->client.request.status = PAM_ERROR;    
            break;
        }
    }
}

void handle_remove_user(struct pam * connection) {


}

void handle_change_password(struct pam * connection) {


}

void handle_change_role(struct pam * connection) {


}

void handle_get_metrics(struct pam * connection) {
    LOG_DEBUG("Handling get metrics request");
    struct metricSnapshot snapshot;
    get_metrics_snapshot(&snapshot);

    char * intialMessage = "+OK Metrics snapshot\n"; 
    size_t copied = 0;
    size_t len = strlen(intialMessage);
    memcpy(connection->client.request.write_body, intialMessage, len);
    copied += len;

    char * metricsInfo = 
        "Current Connections: %d\n"
        "Total Connections: %d\n"
        "Total Bytes Sent: %d\n"
        "Total Bytes Received: %d\n"; 
    int n;
    n = snprintf(
        (char *)connection->client.request.write_body + copied,
        PAM_BUFFER_SIZE - copied,
        metricsInfo,
        snapshot.currentConnections,
        snapshot.totalConnections,
        snapshot.totalBytesSent,
        snapshot.totalBytesReceived
    );

    if (n < 0) {
        LOG_ERROR("Error formatting metrics info");
        connection->client.request.status = PAM_SERVER_ERROR;
        connection->client.request.write_nbody = 0;
    } else {
        copied += (size_t)n;
        connection->client.request.status = PAM_REQUEST_SUCCESS;
        connection->client.request.write_nbody = copied;
    }
}

void handle_reset_historical_connections(struct pam * connection) {


}

void handle_reset_historical_bytes(struct pam * connection) {


}
