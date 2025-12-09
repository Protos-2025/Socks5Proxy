#include "../include/pam.h"
#include "buffer.h"
#include "defines.h"
#include "../include/pamAuth.h"
#include <stdint.h>
#include <stdio.h>
#include <sys/socket.h>
#include "logger.h"
#include "../include/users.h"


void pam_auth_arrival(const unsigned state, struct selector_key * key) {

  struct pam * connection = PAM_ATTACHMENT(key);
  connection->client.auth.state = VER_N_NUSER_N_NPASS;
}
unsigned pam_auth_read(struct selector_key * key) {

  struct pam * connection = PAM_ATTACHMENT(key);
  uint8_t * wPtr;
  size_t count, toRead;
  ssize_t readn;

  wPtr = buffer_write_ptr(&connection->client_buffer, &count);
  readn = recv(key->fd, wPtr, count, 0);

    if (readn < 0) {
        // TODO: handle error correctly
        perror("recv failed (PAM)");
        return PAM_ERROR;
    }
    if (readn == 0) {
        LOG_DEBUG("Client closed connection (PAM)\n");
        return PAM_DONE;
    }

    buffer_write_adv(&connection->client_buffer, readn);
  
    //parse first inner state
  if(connection->client.auth.state == VER_N_NUSER_N_NPASS) {
    buffer_write_ptr(&connection->client_buffer, &toRead);
     
    // wait until we receive first three bytes 
    if(toRead < 3) {
      return PAM_AUTH;
    }
    
    //check version 
    uint8_t ver = buffer_read(&connection->client_buffer);
    if (ver != PAM_VERSION_1) {
        LOG_DEBUG("Unsupported pam version\n");
        return PAM_ERROR;
    }

    //read username lenght
    connection->client.auth.n_user = buffer_read(&connection->client_buffer);
    if (connection->client.auth.n_user == 0) {
        LOG_WARN("Invalid username length");
        return PAM_ERROR;
    }

    //read pass lenght
    connection->client.auth.n_pass = buffer_read(&connection->client_buffer);
    if (connection->client.auth.n_pass == 0) {
        LOG_WARN("Invalid pass length");
        return PAM_ERROR;
    }

    connection->client.auth.state = USERNAME;
    }

    if(connection->client.auth.state == USERNAME) {
      buffer_read_ptr(&connection->client_buffer, &toRead);

      // wait until we receive the whole username
      if(toRead < connection->client.auth.n_user) {
        return PAM_AUTH;
      }

      //read username
      for(size_t i = 0; i < connection->client.auth.n_user; i++) {
        connection->client.auth.username[i] = buffer_read(&connection->client_buffer);
      }
      connection->client.auth.username[connection->client.auth.n_user] = '\0';
      connection->client.auth.state = PASS;
    }


    if(connection->client.auth.state == PASS) {
      buffer_read_ptr(&connection->client_buffer, &toRead);

      // wait until we receive the whole pass
      if(toRead < connection->client.auth.n_pass) {
        return PAM_AUTH;
      }

      //read pass
      for(size_t i = 0; i < connection->client.auth.n_pass && buffer_can_read(&connection->client_buffer); i++) {
        connection->client.auth.pass[i] = buffer_read(&connection->client_buffer);
      }
      connection->client.auth.pass[connection->client.auth.n_pass] = '\0';

      LOG_DEBUG("Received username: %s", connection->client.auth.username);
      LOG_DEBUG("Received password: %s", connection->client.auth.pass);

      UserStatus userStatus = user_authenticate(connection->client.auth.username, connection->client.auth.pass); 

      if (userStatus == USER_OK) {
          LOG_INFO("User '%s' authenticated successfully.", connection->client.auth.username);
          connection->client.auth.status = PAM_AUTH_SUCCESS;
      } else if (userStatus == USER_BADUSERNAME) {
          LOG_INFO("User tried to identify with username %s but there is no such username", connection->client.auth.username);
          connection->client.auth.status = PAM_AUTH_FAILURE;
      } else if (userStatus == USER_WRONGPASSWORD) {
          LOG_INFO("User '%s' provided a wrong password.", connection->client.auth.username);
          connection->client.auth.status = PAM_AUTH_FAILURE;
      } else {
          LOG_ERROR("An error occurred during authentication for user '%s'.", connection->client.auth.username);
          return PAM_ERROR;
      }

      buffer_reset(&connection->client_buffer);
      buffer_write(&connection->client_buffer, PAM_VERSION_1);
      buffer_write(&connection->client_buffer, connection->client.auth.status);
      
      selector_set_interest_key(key, OP_WRITE);
    }

  return PAM_AUTH;
}


unsigned pam_auth_write(struct selector_key * key) {
    struct pam * connection = PAM_ATTACHMENT(key);
    uint8_t * rPtr;
	size_t toRead;
	int written;

	rPtr = buffer_read_ptr(&connection->client_buffer, &toRead);
    written = send(connection->client_fd, rPtr, toRead, 0);
    buffer_read_adv(&connection->client_buffer, written);

    if (written < 0) {
        LOG_FATAL("send failed (PAM_AUTH)");
        return PAM_ERROR;
    }

    if (buffer_can_read(&connection->client_buffer)) {
        return PAM_AUTH;
    }

    LOG_INFO("Pam auth completed");

    buffer_reset(&connection->client_buffer);

    selector_set_interest_key(key, OP_READ); 
    return PAM_REQUEST;
}
