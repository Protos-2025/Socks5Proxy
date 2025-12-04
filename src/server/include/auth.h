#ifndef AUTH_H
#define AUTH_H

#include "selector.h"
#include <stdbool.h>
#include <stdint.h>

// Auth states
enum auth_state {
    AUTH_VER,
    AUTH_ULEN,
    AUTH_UNAME,
    AUTH_PLEN,
    AUTH_PASSWD
};

// Auth info
struct auth_st {
    enum auth_state state;
    uint8_t ulen;
    uint8_t plen;
    char username[256];
    char password[256];
    bool authenticated;
};

// Auth functions
void auth_arrival(const unsigned state, struct selector_key * key);
unsigned auth_read(struct selector_key * key);
unsigned auth_write(struct selector_key * key);

#endif 