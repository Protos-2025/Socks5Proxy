#ifndef __PAM_AUTH_H__
#define __PAM_AUTH_H__

#include "../../shared/include/selector.h"

#define PAM_AUTH_USERNAME_MAX_LENGHT 0xFF
#define PAM_AUTH_PASS_MAX_LENGHT 0xFF


#define AUTH_SUCCESS 0x00
#define AUTH_FAILURE 0x01
#define PAM_SERVER_ERROR 0x02
#define SOCKS_INTERNAL_ERROR 0x03
#define TOO_MANY_AUTH_TRIES 0x04


enum pamAuth_state {
  VER_N_NUSER_N_NPASS,
  USERNAME,
  PASS
};

struct pamAuth_st {
    enum pamAuth_state state;
    uint8_t status;
    uint8_t ver;
    uint8_t n_user;
    uint8_t n_pass;
    char username[PAM_AUTH_USERNAME_MAX_LENGHT];
    char pass[PAM_AUTH_PASS_MAX_LENGHT];
};

void auth_arrival(const unsigned state, struct selector_key * key);
unsigned auth_read(struct selector_key * key);
unsigned auth_write(struct selector_key * key);


#endif
