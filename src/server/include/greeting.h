#ifndef GREETING_H_
#define GREETING_H_

#include "defines.h"
#include <stdint.h>
#include "../../shared/include/selector.h"

enum greeting_state {
    VER_N_NMETHODS,
    METHODS
};

struct greeting_st {
    enum greeting_state state;
    uint8_t n_methods;
    uint8_t method;
};

void greeting_arrival(const unsigned state, struct selector_key * key);

unsigned greeting_read(struct selector_key * key);

unsigned greeting_write(struct selector_key * key);

#endif
