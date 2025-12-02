#include "../include/pam.h"
#include "../include/pamAuth.h"
#include "buffer.h"
#include "selector.h"
#include "stm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "../../shared/include/selector.h"


static const struct state_definition pam_states[] = {
  {
    .state = AUTH,
    .on_arrival = auth_arrival,
    .on_read_ready = auth_read,
    .on_write_ready = auth_write
  },
  //  TODO: add more!
};

static struct pam * pam_new(int client_fd);
static void pam_destroy(struct pam * pam);

static void pam_read(struct selector_key * key);
static void pam_write(struct selector_key * key);
static void pam_block(struct selector_key * key);
static void pam_close(struct selector_key * key);


static const struct fd_handler pam_handler = {
    .handle_read   = pam_read,
    .handle_write  = pam_write,
    .handle_close  = pam_close,
    .handle_block  = pam_block,
};


void pam_passive_accept(struct selector_key * key) {
    struct sockaddr_storage client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    struct pam * connection = NULL;

    const int client_fd = accept(key->fd, (struct sockaddr *) &client_addr, &client_addr_len);
    if (client_fd == -1) {
        goto fail;
    }
    if (selector_fd_set_nio(client_fd) == -1) {
        goto fail;
    }

    struct sockaddr_in * s = (struct sockaddr_in *) &client_addr;
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &s->sin_addr, client_ip, INET_ADDRSTRLEN); // TODO: manage IPv6
    fprintf(stdout, "Accepted connection from %s:%d\n", client_ip, ntohs(s->sin_port));

    connection = pam_new(client_fd);

    if (connection == NULL) {
        // sin un estado, nos es imposible manejaro.
        // tal vez deberiamos apagar accept() hasta que detectemos
        // que se liberó alguna conexión.
        goto fail;
    }
    memcpy(&connection->client_addr, &client_addr, client_addr_len);
    connection->client_addr_len = client_addr_len;

    if (SELECTOR_SUCCESS != selector_register(key->s, client_fd, &pam_handler, OP_READ, connection)) {
        goto fail;
    }
    
    return ;
fail:
    if (client_fd != -1) {
        close(client_fd);
    }
    pam_destroy(connection);
}


static struct pam * pam_new(int client_fd) {
  struct pam * new = malloc(sizeof(struct pam));

  if(new != NULL) {
    new->client_fd = client_fd;
    buffer_init(&new->client_buffer, BUFFER_SIZE, new->client_buffer_data);
    new->stm = (struct state_machine){
      .initial = AUTH,
      .states = pam_states,
      .max_state = AUTH,
      .current = NULL,
    };
    stm_init(&new->stm);
    new->references = 1;

  }
  return new;
}


static void pam_read(struct selector_key * key) {

}
static void pam_write(struct selector_key * key) {

}
static void pam_block(struct selector_key * key) {

}
static void pam_close(struct selector_key * key) {

}

static void pam_destroy(struct pam * pam) {
  free(pam);
}
