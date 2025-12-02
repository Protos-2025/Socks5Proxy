#ifndef PAMELA_H__
#define PAMELA_H__


#include "../../shared/include/selector.h"


enum pamela_state {
  AUTH,
  // add more
};


struct pamela {
};


void pam_passive_accept(struct selector_key * key);


#endif
