#ifndef PAMELA_H__
#define PAMELA_H__


#include "defines.h"
#include <sys/socket.h>
#include "../../shared/include/selector.h"
#include "../../shared/include/buffer.h"
#include "../include/pamAuth.h"
#include "pamRequest.h"
#include "stm.h"

#define PAM_ATTACHMENT(key) ( (struct pam *)(key)->data)

enum pam_state {
	PAM_AUTH,
    PAM_REQUEST,

	PAM_DONE,
    PAM_ERROR
};

struct pam {
  int client_fd;
  struct sockaddr_storage client_addr;
  socklen_t client_addr_len;
  Buffer client_buffer;
  uint8_t client_buffer_data[BUFFER_SIZE];
  union {
    struct pamAuth_st auth;
    struct pamRequest_st request;
    // add more
  } client;


  struct state_machine stm;

  int references;
};


void pam_passive_accept(struct selector_key * key);


#endif
