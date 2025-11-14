#ifndef __LOGS_H__
#define __LOGS_H__

#ifdef DEBUG
#include <stdio.h>
#define LOG(...) fprintf(stderr, __VA_ARGS__)
#else
#define LOG(...)
#endif

#endif
