#ifndef __COPY_H__
#define __COPY_H__

#include "buffer.h"
#include "selector.h"

typedef struct copy_st {
    buffer* buffer;
    int fd;
    size_t interests;
    struct copy_st* otherCopySt;
} copy_st;

void socksv5_copy_arrival(fd_selector s, struct selector_key * key);
unsigned socksv5_copy_read(struct selector_key * key);
unsigned socksv5_copy_write(struct selector_key * key);
void socksv5_copy_close(fd_selector s, struct selector_key * key);

#endif