#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "commands.h"
#include "elkirc.h"
#include "network.h"
#include "ui.h"

/* process user command from input line - ELKS-optimized: no strncmp/strcmp */
void process_command(const char *input)
{
    /* /connect <server> [port] or /c <server> [port] */
    if ((input[0] == '/' && input[1] == 'c' && input[2] == 'o' &&
         input[3] == 'n' && input[4] == 'n' && input[5] == 'e' &&
         input[6] == 'c' && input[7] == 't' && input[8] == ' ') ||
        (input[0] == '/' && input[1] == 'c' && input[2] == ' ')) {
        const char *rest = input + (input[1] == 'c' && input[2] == 'o' ? 9 : 3);
        const char *space = strchr(rest, ' ');
        char server_buf[BUF] = {0};
        char port_buf[BUF] = {0};
        const char *port_str = "6667"; /* default port */

        if (space) {
            /* has port */
            size_t len = space - rest;
            if (len >= BUF) len = BUF - 1;
            strncpy(server_buf, rest, len);
            server_buf[len] = 0;
            strncpy(port_buf, space + 1, BUF - 1);
            port_buf[BUF - 1] = 0;
            port_str = port_buf;
        } else if (*rest) {
            /* only server, use default port */
            strncpy(server_buf, rest, BUF - 1);
            server_buf[BUF - 1] = 0;
        }

        if (server_buf[0]) {
            if (g_connected) {
                printf("Error: already connected. Disconnect first.\n");
                return;
            }

            g_sock = connect_simple(server_buf, port_str);
            if (g_sock < 0) {
                printf("Error: Failed to connect to %s:%s.\n", server_buf, port_str);
                return;
            }

            printf("Connected to %s:%s.\n", server_buf, port_str);
            printf("Registering with the server...\n");

            char tmp[BUF];
            /* NICK */
            snprintf(tmp, BUF, "NICK %s\r\n", g_nick);
            sendl(g_sock, tmp);
            /* USER */
            snprintf(tmp, BUF, "USER %s 0 * :elkirc\r\n", g_nick);
            sendl(g_sock, tmp);

            g_connected = 1;
        }
        return;
    }

    /* /nick <newnick> or /n <newnick> */
    if ((input[0] == '/' && input[1] == 'n' && input[2] == 'i' &&
         input[3] == 'c' && input[4] == 'k' && input[5] == ' ') ||
        (input[0] == '/' && input[1] == 'n' && input[2] == ' ')) {
        const char *newnick = input + (input[2] == 'i' ? 6 : 3);
        if (*newnick) {
            if (!g_connected) {
                printf("Error: not connected to any server.\n");
                return;
            }
            char tmp[BUF];
            snprintf(tmp, BUF, "NICK %s\r\n", newnick);
            sendl(g_sock, tmp);
        }
        return;
    }

    /* /join <channel|nick> or /j <channel|nick> */
    if ((input[0] == '/' && input[1] == 'j' && input[2] == 'o' &&
         input[3] == 'i' && input[4] == 'n' && input[5] == ' ') ||
        (input[0] == '/' && input[1] == 'j' && input[2] == ' ')) {
        const char *target = input + (input[2] == 'o' ? 6 : 3);
        if (*target) {
            if (!g_connected) {
                printf("Error: not connected to any server.\n");
                return;
            }
            strncpy(current_target, target, BUF - 1);
            current_target[BUF - 1] = 0;

            char tmp[BUF];
            snprintf(tmp, BUF, "JOIN %s\r\n", target);
            sendl(g_sock, tmp);
        }
        return;
    }

    /* /msg <nick> <message> */
    if (input[0] == '/' && input[1] == 'm' && input[2] == 's' &&
        input[3] == 'g' && input[4] == ' ') {
        if (!g_connected) {
            printf("Error: not connected to any server.\n");
            return;
        }
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
            sendl(g_sock, out);
        }
        return;
    }

    /* /raw <text> */
    if (input[0] == '/' && input[1] == 'r' && input[2] == 'a' &&
        input[3] == 'w' && input[4] == ' ') {
        if (!g_connected) {
            printf("Error: not connected to any server.\n");
            return;
        }
        const char *text = input + 5;
        if (*text) {
            char tmp[BUF];
            snprintf(tmp, BUF, "%s\r\n", text);
            sendl(g_sock, tmp);
        }
        return;
    }

    /* /leave [channel|nick] or /l [channel|nick] or /part [channel|nick] or /p [channel|nick] */
    if ((input[0] == '/' && input[1] == 'l' && input[2] == 'e' &&
         input[3] == 'a' && input[4] == 'v' && input[5] == 'e' &&
         (input[6] == ' ' || input[6] == '\0')) ||
        (input[0] == '/' && input[1] == 'l' && (input[2] == ' ' || input[2] == '\0')) ||
        (input[0] == '/' && input[1] == 'p' && input[2] == 'a' &&
         input[3] == 'r' && input[4] == 't' && (input[5] == ' ' || input[5] == '\0')) ||
        (input[0] == '/' && input[1] == 'p' && (input[2] == ' ' || input[2] == '\0'))) {
        const char *target = NULL;
        int has_target = 0;

        if (input[1] == 'l' && input[2] == 'e') {
            /* /leave or /leave <target> */
            if (input[6] == ' ') {
                target = input + 7;
                has_target = 1;
            }
        } else if (input[1] == 'l') {
            /* /l or /l <target> */
            if (input[2] == ' ') {
                target = input + 3;
                has_target = 1;
            }
        } else if (input[1] == 'p' && input[2] == 'a') {
            /* /part or /part <target> */
            if (input[5] == ' ') {
                target = input + 6;
                has_target = 1;
            }
        } else {
            /* /p or /p <target> */
            if (input[2] == ' ') {
                target = input + 3;
                has_target = 1;
            }
        }

        if (!g_connected) {
            printf("Error: not connected to any server.\n");
            return;
        }

        if (has_target && *target) {
            /* target specified, leave that target */
            /* if it's a channel (starts with #), send PART command */
            if (target[0] == '#') {
                char tmp[BUF];
                snprintf(tmp, BUF, "PART %s\r\n", target);
                sendl(g_sock, tmp);
            }
            /* clear current_target if it matches, set back to nick */
            if (strcmp(current_target, target) == 0) {
                ui_set_context(g_nick);
            }
        } else {
            /* no target specified, leave current target */
            if (current_target[0]) {
                if (current_target[0] == '#') {
                    /* it's a channel, send PART */
                    char tmp[BUF];
                    snprintf(tmp, BUF, "PART %s\r\n", current_target);
                    sendl(g_sock, tmp);
                }
                /* set target back to nick */
                ui_set_context(g_nick);
            } else {
                printf("Error: no target to leave.\n");
            }
        }
        return;
    }

    /* /quit or /q - disconnect from server but don't exit */
    if ((input[0] == '/' && input[1] == 'q' && input[2] == 'u' &&
         input[3] == 'i' && input[4] == 't' && input[5] == '\0') ||
        (input[0] == '/' && input[1] == 'q' && input[2] == '\0')) {
        if (g_connected) {
            sendl(g_sock, "QUIT :elkirc\r\n");
            /* close socket */
            close(g_sock);
            g_sock = -1;
            g_connected = 0;
            /* set target back to nick */
            ui_set_context(g_nick);
            printf("Disconnected from server.\n");
        } else {
            printf("Error: not connected to any server.\n");
        }
        return;
    }

    /* /exit or /e - exit the program */
    if ((input[0] == '/' && input[1] == 'e' && input[2] == 'x' &&
         input[3] == 'i' && input[4] == 't' && input[5] == '\0') ||
        (input[0] == '/' && input[1] == 'e' && input[2] == '\0')) {
        if (g_connected) {
            sendl(g_sock, "QUIT :elkirc\r\n");
            close(g_sock);
        }
        EXIT(0);
    }

    printf("Unknown command: %s\n", input);
}
