#include <string.h>
#include <stdio.h>

#include "commands.h"

/* process user command from input line */
void process_command(int sock, const char *input)
{
    /* /nick <newnick> */
    if (!strncmp(input, "/nick ", 6)) {
        const char *newnick = input + 6;
        if (*newnick) {
            char tmp[BUF];
            snprintf(tmp, BUF, "NICK %s\r\n", newnick);
            sendl(sock, tmp);
        }
        return;
    }

    /* /join <channel|nick> */
    if (!strncmp(input, "/join ", 6)) {
        const char *target = input + 6;
        if (*target) {
            strncpy(current_target, target, BUF - 1);
            current_target[BUF - 1] = 0;

            char tmp[BUF];
            snprintf(tmp, BUF, "JOIN %s\r\n", target);
            sendl(sock, tmp);
        }
        return;
    }

    /* /msg <nick> <message> */
    if (!strncmp(input, "/msg ", 5)) {
        const char *rest = input + 5;
        const char *space = strchr(rest, ' ');
        if (space) {
            char user[BUF];
            size_t len = space - rest;
            if (len >= BUF) len = BUF - 1;

            strncpy(user, rest, len);
            user[len] = 0;

            const char *msg = space + 1;
            char out[BUF];
            snprintf(out, BUF, "PRIVMSG %s :%s\r\n", user, msg);
            sendl(sock, out);
        }
        return;
    }

    /* /raw <text> */
    if (!strncmp(input, "/raw ", 5)) {
        const char *text = input + 5;
        if (*text) {
            char tmp[BUF];
            snprintf(tmp, BUF, "%s\r\n", text);
            sendl(sock, tmp);
        }
        return;
    }

    /* /quit */
    if (!strcmp(input, "/quit")) {
        sendl(sock, "QUIT :elkirc\r\n");
        EXIT(0);
    }

    /* /exit */
    if (!strcmp(input, "/exit")) {
        EXIT(0);
    }
    
    printf("Unknown command: %s\n", input);
}
