#ifndef PAMELA_H__
#define PAMELA_H__


#include "defines.h"
#include <sys/socket.h>
#include "../../shared/include/selector.h"
#include "../../shared/include/buffer.h"
#include "stm.h"


enum pamela_state {
  AUTH,
  // add more
};


struct pam {
  int client_fd;
  struct sockaddr_storage client_addr;
  socklen_t client_addr_len;
    buffer client_buffer;
    uint8_t client_buffer_data[BUFFER_SIZE];


  struct state_machine stm;

  int references;
};


void pam_passive_accept(struct selector_key * key);


#endif
