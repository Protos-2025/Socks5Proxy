#ifndef REPLY_H_
#define REPLY_H_

#include "defines.h"
#include "../../shared/include/selector.h"

#define SUCCEDED 0x00
#define SERVER_FAILURE 0x01
#define CONNECTION_NOT_ALLOWED 0x02
#define NETWORK_UNREACHABLE 0x03
#define HOST_UNREACHABLE 0x04
#define CONNECTION_REFUSED 0x05
#define TTL_EXPIRED 0x06
#define COMMAND_NOT_SUPPORTED 0x07
#define ADDRESS_TYPE_NOT_SUPPORTED 0x08
#define INVALID_SOCKS5_VERSION 0x09
#define INVALID_RSV 0x10
#define INVALID_FQDN_LENGHT 0x11

struct reply_st {
    uint8_t rep;
    bool found_bnd_info;
};

void reply_arrival(const unsigned state, struct selector_key * key);

unsigned reply_write(struct selector_key * key);

#endif
