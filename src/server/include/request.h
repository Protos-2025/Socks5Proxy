#ifndef REQUEST_H_
#define REQUEST_H_

#include "defines.h"
#include "../../shared/include/selector.h"

enum request_state {
    VER_CMD_ATYP,
    DST_LEN,
    DST_RES
};

struct request_st {
    enum request_state state;
    uint8_t cmd, atyp;
};

void request_arrival(const unsigned state, struct selector_key * key);

unsigned request_read(struct selector_key * key);

unsigned request_block(struct selector_key * key);

#endif
