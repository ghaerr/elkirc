#include <stdio.h>
#include <string.h>

#ifdef __ELKS__
#include <bsd/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#endif

#include "network.h"
#include "elkirc.h"

/* simple connect to host:port, return socket or -1 */
int connect_simple(const char *host, const char *port)
{
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(host, port, &hints, &res) != 0)
        return -1;

    int sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock < 0) {
        freeaddrinfo(res);
        return -1;
    }

    if (connect(sock, res->ai_addr, res->ai_addrlen) != 0) {
        close(sock);
        freeaddrinfo(res);
        return -1;
    }

    freeaddrinfo(res);
    return sock;
}

/* send a null-terminated string over socket */
int sendl(int sock, const char *s)
{
    int len = strlen(s);
    return send(sock, s, len, 0);
}

/* minimal IRC input handler */
void handle_line(int sock, const char *line)
{
    /* PING :server */
    if (!strncmp(line, "PING", 4)) {
        char pong[BUF];
        snprintf(pong, BUF, "PONG %s\r\n", line + 5);
        sendl(sock, pong);
        return;
    }

    /* nothing else handled for now */
}
