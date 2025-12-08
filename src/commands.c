#include <string.h>
#include <stdio.h>

#include "commands.h"

/* process user command from input line - ELKS-optimized: no strncmp/strcmp */
void process_command(int sock, const char *input)
{
    /* /nick <newnick> */
    if (input[0] == '/' && input[1] == 'n' && input[2] == 'i' &&
        input[3] == 'c' && input[4] == 'k' && input[5] == ' ') {
        const char *newnick = input + 6;
        if (*newnick) {
            char tmp[BUF];
            snprintf(tmp, BUF, "NICK %s\r\n", newnick);
            sendl(sock, tmp);
        }
        return;
    }

    /* /join <channel|nick> */
    if (input[0] == '/' && input[1] == 'j' && input[2] == 'o' &&
        input[3] == 'i' && input[4] == 'n' && input[5] == ' ') {
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
    if (input[0] == '/' && input[1] == 'm' && input[2] == 's' &&
        input[3] == 'g' && input[4] == ' ') {
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
    if (input[0] == '/' && input[1] == 'r' && input[2] == 'a' &&
        input[3] == 'w' && input[4] == ' ') {
        const char *text = input + 5;
        if (*text) {
            char tmp[BUF];
            snprintf(tmp, BUF, "%s\r\n", text);
            sendl(sock, tmp);
        }
        return;
    }

    /* /quit */
    if (input[0] == '/' && input[1] == 'q' && input[2] == 'u' &&
        input[3] == 'i' && input[4] == 't' && input[5] == '\0') {
        sendl(sock, "QUIT :elkirc\r\n");
        EXIT(0);
    }

    /* /exit */
    if (input[0] == '/' && input[1] == 'e' && input[2] == 'x' &&
        input[3] == 'i' && input[4] == 't' && input[5] == '\0') {
        EXIT(0);
    }
    
    printf("Unknown command: %s\n", input);
}
