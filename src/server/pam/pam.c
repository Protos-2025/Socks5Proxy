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
#include "logger.h"


#include "../../shared/include/selector.h"


static const struct state_definition pamStates[] = {
  {
    .state = PAM_AUTH,
    .on_arrival = pam_auth_arrival,
    .on_read_ready = pam_auth_read,
    .on_write_ready = pam_auth_write
  },
  {
    .state = PAM_DONE
  },
  {
    .state = PAM_ERROR
  }
  //  TODO: add more!
};

static struct pam * pam_new(int client_fd);
static void pam_destroy(struct pam * pam);

static void pam_read(struct selector_key * key);
static void pam_write(struct selector_key * key);
static void pam_block(struct selector_key * key);
static void pam_close(struct selector_key * key);
static void pam_done(struct selector_key * key);


static const struct fd_handler pamHandler = {
    .handle_read   = pam_read,
    .handle_write  = pam_write,
    .handle_close  = pam_close,
    .handle_block  = pam_block,
};


void pam_passive_accept(struct selector_key * key) {
    struct sockaddr_storage clientAddr;
    socklen_t alientAddrLen = sizeof(clientAddr);
    struct pam * connection = NULL;

    const int clientFd = accept(key->fd, (struct sockaddr *) &clientAddr, &alientAddrLen);
    if (clientFd == -1) {
        goto fail;
    }
    if (selector_fd_set_nio(clientFd) == -1) {
        goto fail;
    }

    struct sockaddr_in * s = (struct sockaddr_in *) &clientAddr;
    char clientIp[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &s->sin_addr, clientIp, INET_ADDRSTRLEN); // TODO: manage IPv6
    LOG_DEBUG("Accepted connection on pam server from %s:%d", clientIp, ntohs(s->sin_port));

    connection = pam_new(clientFd);

    if (connection == NULL) {
        // sin un estado, nos es imposible manejaro.
        // tal vez deberiamos apagar accept() hasta que detectemos
        // que se liberó alguna conexión.
        goto fail;
    }
    memcpy(&connection->client_addr, &clientAddr, alientAddrLen);
    connection->client_addr_len = alientAddrLen;

    if (SELECTOR_SUCCESS != selector_register(key->s, clientFd, &pamHandler, OP_READ, connection)) {
        goto fail;
    }

    return ;
fail:
    if (clientFd != -1) {
        close(clientFd);
    }
    pam_destroy(connection);
}


static struct pam * pam_new(int client_fd) {
  struct pam * new = malloc(sizeof(struct pam));

  if(new != NULL) {
    new->client_fd = client_fd;
    buffer_init(&new->client_buffer, BUFFER_SIZE, new->client_buffer_data);
    new->stm = (struct state_machine){
      .initial = PAM_AUTH,
      .states = pamStates,
      .max_state = PAM_ERROR,
      .current = NULL,
    };
    stm_init(&new->stm);
    new->references = 1;
  }

  return new;
}


static void pam_read(struct selector_key * key) {

  struct state_machine * stm   = &PAM_ATTACHMENT(key)->stm;
  const enum pam_state st = stm_handler_read(stm, key);

  if(PAM_ERROR == st || PAM_DONE == st) {
      pam_done(key);
  }
}

static void pam_write(struct selector_key * key) {

  struct state_machine * stm = &PAM_ATTACHMENT(key)->stm;
  const enum pam_state st = stm_handler_write(stm, key);

  if (PAM_ERROR == st || PAM_DONE == st) {
    pam_done(key);
  }
}

static void pam_block(struct selector_key * key) {

  struct state_machine *stm   = &PAM_ATTACHMENT(key)->stm;
    const enum pam_state st = stm_handler_block(stm, key);

    if(PAM_ERROR == st || PAM_DONE == st) {
        pam_done(key);
    }
}

static void pam_close(struct selector_key * key) {
  pam_destroy(PAM_ATTACHMENT(key));
}

static void pam_done(struct selector_key * key) {
  const int fd = PAM_ATTACHMENT(key)->client_fd;

  if(SELECTOR_SUCCESS != selector_unregister_fd(key->s, fd)) {
    abort();
  }
  close(fd);

}

static void pam_destroy(struct pam * pam) {
  free(pam);
}
