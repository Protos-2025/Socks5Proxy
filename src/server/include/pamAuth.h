#ifndef __PAM_AUTH_H__
#define __PAM_AUTH_H__

#include "../../shared/include/selector.h"


void auth_arrival(const unsigned state, struct selector_key * key);
unsigned auth_read(struct selector_key * key);
unsigned auth_write(struct selector_key * key);


#endif
