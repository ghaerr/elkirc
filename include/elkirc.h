#ifndef ELKIRC_H
#define ELKIRC_H

#define BUF 256

/* ELKS-first exit wrapper */
#ifdef __ELKS__
#include <unistd.h>
#define EXIT(x) _exit(x)
#else
#include <stdlib.h>
#define EXIT(x) exit(x)
#endif

extern char current_target[BUF];

#endif
