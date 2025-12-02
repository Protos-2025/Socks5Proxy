#include "../include/pam.h"
#include "buffer.h"
#include "defines.h"
#include "../include/pamAuth.h"
#include <stdint.h>
#include <stdio.h>
#include <sys/socket.h>
#include "logger.h"


void auth_arrival(const unsigned state, struct selector_key * key) {

  struct pam * connection = PAM_ATTACHMENT(key);
  connection->client.auth.state = VER_N_NUSER_N_NPASS;
}
unsigned auth_read(struct selector_key * key) {

  struct pam * connection = PAM_ATTACHMENT(key);
  uint8_t * w_ptr;
  size_t count, readn, to_read;

  w_ptr = buffer_write_ptr(&connection->client_buffer, &count);
  readn = recv(key->fd, w_ptr, count, 0);

    if (readn < 0) {
        // TODO: handle error correctly
        perror("recv failed (PAM)");
        return PAM_ERROR;
    }
    if (readn == 0) {
        fprintf(stdout, "Client closed connection (PAM)\n");
        return PAM_DONE;
    }

    buffer_write_adv(&connection->client_buffer, readn);
  
    //parse first inner state
  if(connection->client.auth.state == VER_N_NUSER_N_NPASS) {
    buffer_write_ptr(&connection->client_buffer, &to_read);
     
    // wait until we receive first three bytes 
    if(to_read < 3) {
      return AUTH;
    }  
    
    //check version 
    uint8_t ver = buffer_read(&connection->client_buffer);
    if (ver != PAM_VERSION) {
        fprintf(stdout, "Unsupported pam version\n");
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
      buffer_read_ptr(&connection->client_buffer, &to_read);

      // wait until we receive the whole username
      if(to_read < connection->client.auth.n_user) {
        return AUTH;
      }  

      //read username
      for(size_t i = 0; i < connection->client.auth.n_user; i++) {
        connection->client.auth.username[i] = buffer_read(&connection->client_buffer);
      }
      connection->client.auth.username[connection->client.auth.n_user] = '\0';
      connection->client.auth.state = PASS;
    }


    if(connection->client.auth.state == PASS) {
      buffer_read_ptr(&connection->client_buffer, &to_read);

      // wait until we receive the whole pass
      if(to_read < connection->client.auth.n_pass) {
        return AUTH;
      }  

      //read pass
      for(size_t i = 0; i < connection->client.auth.n_pass && buffer_can_read(&connection->client_buffer); i++) {
        connection->client.auth.pass[i] = buffer_read(&connection->client_buffer);
      }
      connection->client.auth.pass[connection->client.auth.n_pass] = '\0';

      LOG_DEBUG("Received username: %s", connection->client.auth.username);
      LOG_DEBUG("Received password: %s", connection->client.auth.pass);
      connection->client.auth.status = AUTH_SUCCESS; // TODO: validate credentials

      buffer_reset(&connection->client_buffer);
      buffer_write(&connection->client_buffer, SOCKS5_VERSION);
      buffer_write(&connection->client_buffer, connection->client.auth.status);
      
      selector_set_interest_key(key, OP_WRITE);
    }

  return AUTH;
}


unsigned greeting_write(struct selector_key * key) {
    struct pam * connection = PAM_ATTACHMENT(key);
    uint8_t * r_ptr;
	size_t to_read;
	int written;

	r_ptr = buffer_read_ptr(&connection->client_buffer, &to_read);
    written = send(connection->client_fd, r_ptr, to_read, 0);
    buffer_read_adv(&connection->client_buffer, written);

    if (written < 0) {
        LOG_FATAL("send failed (PAM_AUTH)");
        return PAM_ERROR;
    }

    if (buffer_can_read(&connection->client_buffer)) {
        return AUTH;
    }

    LOG_INFO("Pam auth completed");

    buffer_reset(&connection->client_buffer);

    // TODO: replace both lines once the next state is implemented!
    selector_set_interest_key(key, OP_NOOP); // selector_set_interest_key(key, OP_READ);
    return PAM_DONE; // return next state;
}
