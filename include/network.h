#ifndef NETWORK_H
#define NETWORK_H

#include "elkirc.h"

/* Connect to server. Returns socket or -1. */
int connect_simple(const char *host, const char *port);

/* Send a null-terminated string. */
int sendl(int sock, const char *s);

/* Handle server line (PING, etc.) */
void handle_line(int sock, const char *line);

#endif /* NETWORK_H */
