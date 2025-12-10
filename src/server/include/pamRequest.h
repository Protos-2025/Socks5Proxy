#ifndef __PAM_REEQUEST_H__
#define __PAM_REEQUEST_H__

#include <stdint.h>
#include "../../shared/include/selector.h"
#include "buffer.h"

#define PAM_REQUEST_SUCCESS 0x00
#define PAM_REQUEST_FAILURE 0x01
#define PAM_REQUEST_UNAUTHORIZED 0x02
#define PAM_REQUEST_USER_ALREADY_EXISTS 0x03
#define PAM_REQUEST_USER_WRONG_PASSWORD 0x04
#define PAM_REQUEST_USER_CREDTOOLONG 0x05
#define PAM_REQUEST_USER_BADUSERNAME 0x06


#define PAM_REQUEST_BODY_MAX_LENGTH 65536
#define RESERVED_BYTE 0x00

 // **0x00: RESERVADO**
 // **0x01: Obtener lista de usuarios conectados actualmente**
 // **0x02: Añadir usuario**
 // **0x03: Remover usuario**
 // **0x04: Cambiar contraseña**
 // **0x05: Cambiar rol**
 // **0x06: Métricas**
 // 0x07-0xF0: RESERVADO
 
#define PAM_REQUEST_METHOD_GET_CONNECTED_USERS_LIST 0x01
#define PAM_REQUEST_METHOD_ADD_USER 0x02
#define PAM_REQUEST_METHOD_REMOVE_USER 0x03
#define PAM_REQUEST_METHOD_CHANGE_PASSWORD 0x04
#define PAM_REQUEST_METHOD_CHANGE_ROLE 0x05
#define PAM_REQUEST_METHOD_GET_METRICS 0x06

enum pam_request_state {
  VER_N_RESERVED,
  METHOD,
  NBODY,
  READ_BODY,

  WRITE
};

struct pamRequest_st {
    enum pam_request_state state;
    uint8_t ver;
    uint8_t reserved;
    uint16_t method;
    uint16_t read_nbody;

    uint8_t status;
    uint16_t write_nbody;
    char write_body[PAM_REQUEST_BODY_MAX_LENGTH];
};


void pam_request_arrival(const unsigned state, struct selector_key * key);
unsigned pam_request_read(struct selector_key * key);
unsigned pam_request_write(struct selector_key * key);

#endif
