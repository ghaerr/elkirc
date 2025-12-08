#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef __ELKS__
/* ELKS headers */
#include <bsd/socket.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/time.h>
#include <arpa/inet.h>
#else
/* modern POSIX headers */
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <netdb.h>
#endif

#include "elkirc.h"
#include "network.h"
#include "commands.h"
#include "ui.h"

char current_target[BUF] = {0};
int debug_mode = 0;

int main(int argc, char *argv[])
{
    const char *server;
    const char *port;
    const char *nick;
    int arg_idx = 1;

    /* check for debug flag - optimize for ELKS: check -d first (shorter) */
    if (argc > 1) {
        if (argv[arg_idx][0] == '-' && argv[arg_idx][1] == 'd' && argv[arg_idx][2] == '\0') {
            debug_mode = 1;
            arg_idx++;
        } else if (strcmp(argv[arg_idx], "--debug") == 0) {
            debug_mode = 1;
            arg_idx++;
        }
    }

    if (argc < (arg_idx + 3)) {
        printf("Usage: %s [-d|--debug] <server> <port> <nick>\n", argv[0]);
        return 1;
    }

    server = argv[arg_idx];
    port   = argv[arg_idx + 1];
    nick   = argv[arg_idx + 2];

    if (ui_init() != 0) {
        fprintf(stderr, "UI init failed\n");
        return 1;
    }

    int sock = connect_simple(server, port);
    if (sock < 0) {
        printf("Error: Failed to connect to %s:%s.\n", server, port);
        return 1;
    }

    printf("Connected to %s:%s.\n", server, port);

    printf("Registering with the server...\n");
    {
        char tmp[BUF];
        /* NICK */
        snprintf(tmp, BUF, "NICK %s\r\n", nick);
        sendl(sock, tmp);
        /* USER */
        snprintf(tmp, BUF, "USER %s 0 * :elkirc\r\n", nick);
        sendl(sock, tmp);
    }

    /* default target = nick (private) */
    ui_set_context(nick);

    /* set up select() */
    int fd_stdin = fileno(stdin);
    int maxfd = (sock > fd_stdin) ? sock : fd_stdin;

    char netbuf[BUF];
    int netpos = 0;
    char inbuf[BUF];

    /* Draw initial prompt and allow server messages to arrive */
    ui_draw_prompt();

    while (1) {
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(fd_stdin, &rfds);
        FD_SET(sock, &rfds);

        /* no timeout - wait until input on either */
        int ready = select(maxfd + 1, &rfds, NULL, NULL, NULL);
        if (ready <= 0) continue;

        /* socket ready? read and print immediately */
        if (FD_ISSET(sock, &rfds)) {
            char r[64];
            int n = recv(sock, r, sizeof(r) - 1, 0);
            if (n <= 0) {
                printf("\nError: Server closed connection.\n");
                EXIT(0);
            }
            /* process bytes and print lines as they complete */
            int i;
            for (i = 0; i < n; ++i) {
                if (netpos < (BUF - 1)) netbuf[netpos++] = r[i];
                if (r[i] == '\n') {
                    netbuf[netpos] = 0;
                    /* check if this is a PING message - ELKS-optimized: check start first */
                    int is_ping = (!strncmp(netbuf, "PING", 4));
                    /* also check for prefixed PING (e.g., ":server PING") - minimal check */
                    if (!is_ping && netbuf[0] == ':') {
                        const char *p = netbuf;
                        while (*p && *p != ' ') p++;
                        if (*p == ' ' && !strncmp(p + 1, "PING", 4)) is_ping = 1;
                    }
                    /* print the server line (unless it's a PING and debug mode is off) */
                    if (!is_ping || debug_mode) {
                        fputs(netbuf, stdout);
                        fflush(stdout);
                    }
                    netpos = 0;
                    /* let network handler inspect (PING) */
                    handle_line(sock, netbuf);
                    /* re-draw prompt for UX */
                    ui_redraw_after_output();
                }
            }
        }

        /* stdin ready? read one line and process */
        if (FD_ISSET(fd_stdin, &rfds)) {
            if (fgets(inbuf, sizeof(inbuf), stdin) == NULL) {
                /* EOF on stdin */
                printf("\nError: stdin closed.\n");
                EXIT(0);
            }
            /* trim newline */
            inbuf[strcspn(inbuf, "\r\n")] = 0;

            if (inbuf[0] == '/') {
                process_command(sock, inbuf);
            } else {
                /* send normal message to current_target */
                if (current_target[0]) {
                    char out[BUF];
                    snprintf(out, BUF, "PRIVMSG %s :%s\r\n", current_target, inbuf);
                    sendl(sock, out);
                } else {
                    printf("Error: no target set. Use /join <#channel|nick> to set a target.\n");
                }
            }
            /* after handling user input, redraw prompt */
            ui_draw_prompt();
        }
    }

    return 0;
}
