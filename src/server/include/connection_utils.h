#ifndef CONNECTION_UTILS_H_
#define CONNECTION_UTILS_H_

#include "../../shared/include/selector.h"
#include "../include/socks5nio.h"

#define GENERAL_FAILURE 0
#define SELECTOR_REGISTER_FAILED 1
#define CONNECTION_IN_PROGRESS 2
#define CONNECTION_DONE 3

int try_connection(struct selector_key * key);

void get_next_resolution(struct selector_key * key);

void print_connection_access_log(struct socks5* connection);

#endif
