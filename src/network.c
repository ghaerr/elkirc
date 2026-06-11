#include <stdio.h>
#include <string.h>

#ifdef __ELKS__
#include <unistd.h>
#include <sys/socket.h>
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

/* simple connect to host:port, return socket or -1 - ELKS-optimized */
int connect_simple(const char *host, const char *port)
{
#ifdef __ELKS__
    /* ELKS: use in_gethostbyname instead of getaddrinfo */
    ipaddr_t ipaddr = in_gethostbyname(host);
    if (!ipaddr) return -1;

    int port_num = 0;
    const char *p = port;
    while (*p >= '0' && *p <= '9') {
        port_num = port_num * 10 + (*p - '0');
        p++;
    }

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return -1;

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = 0;
    addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(sock);
        return -1;
    }

    addr.sin_port = htons(port_num);
    memcpy(&addr.sin_addr, &ipaddr, sizeof(ipaddr));
    if (in_connect(sock, (struct sockaddr *)&addr, sizeof(addr), 10) != 0) {
        close(sock);
        return -1;
    }

    return sock;
#else
    /* modern POSIX: use getaddrinfo */
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
#endif
}

/* send a null-terminated string over socket - ELKS-optimized: manual length calc */
int sendl(int sock, const char *s)
{
    const char *p = s;
    while (*p) p++;
    int len = p - s;
    return send(sock, s, len, 0);
}

/* minimal IRC input handler - ELKS-optimized: no strncmp */
void handle_line(int sock, const char *line)
{
    /* PING :server */
    if (line[0] == 'P' && line[1] == 'I' && line[2] == 'N' && line[3] == 'G') {
        char pong[BUF];
        const char *ping_arg = line + 5; /* skip "PING " */
        /* ELKS-optimized: manual string construction instead of snprintf */
        char *p = pong;
        *p++ = 'P';
        *p++ = 'O';
        *p++ = 'N';
        *p++ = 'G';
        *p++ = ' ';
        while (*ping_arg && (p - pong) < (BUF - 3)) {
            *p++ = *ping_arg++;
        }
        *p++ = '\r';
        *p++ = '\n';
        *p = '\0';
        sendl(sock, pong);
        return;
    }

    /* nothing else handled for now */
}

#ifdef __ELKS__
ssize_t send(int socket, const void *buffer, size_t length, int flags)
{
    return write(socket, buffer, length);
}

ssize_t recv(int socket, void *buffer, size_t length, int flags)
{
    return read(socket, buffer, length);
}
#endif
