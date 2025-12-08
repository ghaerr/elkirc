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

int main(int argc, char *argv[])
{
    if (argc < 4) {
        printf("Usage: %s <server> <port> <nick>\n", argv[0]);
        return 1;
    }

    const char *server = argv[1];
    const char *port   = argv[2];
    const char *nick   = argv[3];

    if (ui_init() != 0) {
        fprintf(stderr, "UI init failed\n");
        return 1;
    }

    int sock = connect_simple(server, port);
    if (sock < 0) {
        printf("Connection failed.\n");
        return 1;
    }

    /* register */
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
                printf("\n** server closed connection **\n");
                EXIT(0);
            }
            /* process bytes and print lines as they complete */
            int i;
            for (i = 0; i < n; ++i) {
                if (netpos < (BUF - 1)) netbuf[netpos++] = r[i];
                if (r[i] == '\n') {
                    netbuf[netpos] = 0;
                    /* print the server line */
                    fputs(netbuf, stdout);
                    fflush(stdout);
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
                printf("\n** stdin closed **\n");
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
                    printf("No target set. Use /join <channel|nick>\n");
                }
            }
            /* after handling user input, redraw prompt */
            ui_draw_prompt();
        }
    }

    return 0;
}
