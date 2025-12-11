#ifndef PAMELA_H__
#define PAMELA_H__


#include <sys/socket.h>
#include "../../shared/include/selector.h"
#include "../../shared/include/buffer.h"
#include "../include/pamAuth.h"
#include "pamRequest.h"
#include "stm.h"
#include "defines.h"

#define PAM_ATTACHMENT(key) ( (struct pam *)(key)->data)

#define PAM_BUFFER_SIZE 65536

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
  uint8_t client_buffer_data[PAM_BUFFER_SIZE];
  struct {
    struct pamAuth_st auth;
    struct pamRequest_st request;
  } client;

  struct state_machine stm;
};


void pam_passive_accept(struct selector_key * key);


#endif
