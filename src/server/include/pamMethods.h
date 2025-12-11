#ifndef __PAM_METHODS_H__
#define __PAM_METHODS_H__

#include "pam.h"

void handle_pam_request_method(struct pam * connection, Buffer * buffer);

#endif
