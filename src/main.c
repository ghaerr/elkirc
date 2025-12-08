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
int color_mode = 0;
/* track color state for long lines that span multiple fragments - ELKS: global instead of static */
int long_line_color_active = 0; /* 0=none, 1=blue, 2=green, 3=red */

int main(int argc, char *argv[])
{
    const char *server;
    const char *port;
    const char *nick;
    int arg_idx = 1;

    /* check for flags - ELKS-optimized: check short flags first, avoid strcmp */
    while (argc > arg_idx && argv[arg_idx][0] == '-') {
        const char *arg = argv[arg_idx];
        if (arg[1] == 'd' && arg[2] == '\0') {
            debug_mode = 1;
            arg_idx++;
        } else if (arg[1] == 'c' && arg[2] == '\0') {
            color_mode = 1;
            arg_idx++;
        } else if (arg[1] == '-' && arg[2] == 'd' && arg[3] == 'e' &&
                   arg[4] == 'b' && arg[5] == 'u' && arg[6] == 'g' && arg[7] == '\0') {
            debug_mode = 1;
            arg_idx++;
        } else if (arg[1] == '-' && arg[2] == 'c' && arg[3] == 'o' &&
                   arg[4] == 'l' && arg[5] == 'o' && arg[6] == 'r' && arg[7] == '\0') {
            color_mode = 1;
            arg_idx++;
        } else {
            break; /* unknown flag, treat as server */
        }
    }

    if (argc < (arg_idx + 3)) {
        printf("Usage: %s [-d|--debug] [-c|--color] <server> <port> <nick>\n", argv[0]);
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
                /* handle buffer overflow for long lines: print fragment and continue */
                if (netpos >= (BUF - 1)) {
                    /* buffer full but no \n yet - detect and apply server prefix pattern if present */
                    netbuf[netpos] = 0;
                    if (netbuf[0] == ':') {
                        /* try to detect ":server.domain XXX nick " pattern in current fragment */
                        const char *p = netbuf;
                        const char *srv_end = p;
                        while (*srv_end && *srv_end != ' ') srv_end++;
                        if (*srv_end == ' ') {
                            const char *code_start = srv_end + 1;
                            if (code_start[0] >= '0' && code_start[0] <= '9' &&
                                code_start[1] >= '0' && code_start[1] <= '9' &&
                                code_start[2] >= '0' && code_start[2] <= '9' &&
                                code_start[3] == ' ') {
                                const char *nick_start = code_start + 4;
                                const char *nick_end = nick_start;
                                while (*nick_end && *nick_end != ' ' && (nick_end - netbuf) < (BUF - 1)) nick_end++;
                                if (*nick_end == ' ') {
                                    /* found pattern ":server.domain XXX nick " - apply formatting */
                                    int prefix_len = nick_end - netbuf;
                                    const char *content_start = nick_end + 1;
                                    /* determine code type */
                                    char c0 = code_start[0], c1 = code_start[1], c2 = code_start[2];
                                    int frag_code_type = -1;
                                    if ((c0 == '0' || c0 == '1' || c0 == '2') &&
                                        c1 >= '0' && c1 <= '9' && c2 >= '0' && c2 <= '9') {
                                        frag_code_type = 0;
                                    } else if (c0 == '3' && c1 >= '0' && c1 <= '9' && c2 >= '0' && c2 <= '9') {
                                        frag_code_type = 1;
                                    } else if ((c0 == '4' || c0 == '5') &&
                                               c1 >= '0' && c1 <= '9' && c2 >= '0' && c2 <= '9') {
                                        frag_code_type = 2;
                                    }
                                    /* print prefix only if debug enabled */
                                    if (debug_mode) {
                                        if (color_mode) {
                                            ui_color_grey();
                                            fwrite(netbuf, 1, prefix_len, stdout);
                                            ui_color_reset();
                                        } else {
                                            fwrite(netbuf, 1, prefix_len, stdout);
                                        }
                                    }
                                    /* apply color to content and remember for next fragments */
                                    if (color_mode && frag_code_type >= 0) {
                                        if (frag_code_type == 0) {
                                            ui_color_blue();
                                            long_line_color_active = 1;
                                        } else if (frag_code_type == 1) {
                                            ui_color_green();
                                            long_line_color_active = 2;
                                        } else if (frag_code_type == 2) {
                                            ui_color_red();
                                            long_line_color_active = 3;
                                        }
                                    }
                                    /* print content part */
                                    if (content_start < (netbuf + netpos)) {
                                        fputs(content_start, stdout);
                                    }
                                    /* DON'T reset color here - keep it active for next fragments */
                                } else {
                                    /* pattern not complete in fragment */
                                    if (long_line_color_active > 0) {
                                        /* continue with active color from previous fragment */
                                        if (color_mode) {
                                            if (long_line_color_active == 1) {
                                                ui_color_blue();
                                            } else if (long_line_color_active == 2) {
                                                ui_color_green();
                                            } else if (long_line_color_active == 3) {
                                                ui_color_red();
                                            }
                                        }
                                    }
                                    fputs(netbuf, stdout);
                                }
                            } else {
                                /* no numeric code */
                                if (long_line_color_active > 0) {
                                    /* continue with active color from previous fragment */
                                    if (color_mode) {
                                        if (long_line_color_active == 1) {
                                            ui_color_blue();
                                        } else if (long_line_color_active == 2) {
                                            ui_color_green();
                                        } else if (long_line_color_active == 3) {
                                            ui_color_red();
                                        }
                                    }
                                }
                                fputs(netbuf, stdout);
                            }
                        } else {
                            /* no space after server name */
                            if (long_line_color_active > 0) {
                                /* continue with active color from previous fragment */
                                if (color_mode) {
                                    if (long_line_color_active == 1) {
                                        ui_color_blue();
                                    } else if (long_line_color_active == 2) {
                                        ui_color_green();
                                    } else if (long_line_color_active == 3) {
                                        ui_color_red();
                                    }
                                }
                            }
                            fputs(netbuf, stdout);
                        }
                    } else {
                        /* doesn't start with ':' */
                        if (long_line_color_active > 0) {
                            /* continue with active color from previous fragment */
                            if (color_mode) {
                                if (long_line_color_active == 1) {
                                    ui_color_blue();
                                } else if (long_line_color_active == 2) {
                                    ui_color_green();
                                } else if (long_line_color_active == 3) {
                                    ui_color_red();
                                }
                            }
                        }
                        fputs(netbuf, stdout);
                    }
                    fflush(stdout);
                    netpos = 0;
                }
                netbuf[netpos++] = r[i];
                if (r[i] == '\n') {
                    /* strip \r if present before \n, keep \n (ELKS-optimized) */
                    /* netbuf[netpos-1] is \n we just added, check if netbuf[netpos-2] is \r */
                    if (netpos >= 2 && netbuf[netpos - 2] == '\r' && netbuf[netpos - 1] == '\n') {
                        /* remove \r: shift \n left, null-terminate after \n */
                        netbuf[netpos - 2] = '\n';
                        netbuf[netpos - 1] = 0; /* null-terminate (netpos-1 had \n, now it's the terminator) */
                        netpos--; /* adjust since we removed one char */
                    } else {
                        /* \n is already in buffer at netpos-1, just null-terminate after it */
                        netbuf[netpos] = 0;
                    }
                    /* check if this is a PING message - ELKS-optimized: no strncmp */
                    int is_ping = 0;
                    if (netbuf[0] == 'P' && netbuf[1] == 'I' && netbuf[2] == 'N' && netbuf[3] == 'G') {
                        is_ping = 1;
                    } else if (netbuf[0] == ':') {
                        /* check for prefixed PING (e.g., ":server PING") */
                        const char *p = netbuf;
                        while (*p && *p != ' ') p++;
                        if (*p == ' ' && p[1] == 'P' && p[2] == 'I' && p[3] == 'N' && p[4] == 'G') {
                            is_ping = 1;
                        }
                    }
                    /* print the server line (unless it's a PING and debug mode is off) */
                    if (!is_ping || debug_mode) {
                        /* ELKS-optimized: single pass detection of all patterns */
                        int has_notice = 0, has_join = 0, has_privmsg = 0;
                        const char *privmsg_pos = NULL;
                        const char *server_prefix_start = NULL;
                        const char *server_prefix_end = NULL;
                        const char *content_after_nick = NULL;
                        int code_type = -1; /* 0=0XX/1XX/2XX, 1=3XX, 2=4XX/5XX, -1=other */
                        int is_long_line = 0;
                        int is_own_msg = 0;
                        const char *p = netbuf;
                        
                        /* detect ":server.domain XXX nick " pattern first (most common) */
                        if (*p == ':') {
                            const char *srv_end = p;
                            while (*srv_end && *srv_end != ' ') srv_end++;
                            if (*srv_end == ' ') {
                                const char *code_start = srv_end + 1;
                                /* check if numeric code XXX */
                                if (code_start[0] >= '0' && code_start[0] <= '9' &&
                                    code_start[1] >= '0' && code_start[1] <= '9' &&
                                    code_start[2] >= '0' && code_start[2] <= '9' &&
                                    code_start[3] == ' ') {
                                    const char *nick_start = code_start + 4;
                                    const char *nick_end = nick_start;
                                    while (*nick_end && *nick_end != ' ') nick_end++;
                                    if (*nick_end == ' ') {
                                        /* found pattern ":server.domain XXX nick " */
                                        server_prefix_start = netbuf;
                                        server_prefix_end = nick_end;
                                        content_after_nick = nick_end + 1;
                                        /* determine code type - ELKS-optimized: no strncmp */
                                        char c0 = code_start[0], c1 = code_start[1], c2 = code_start[2];
                                        if ((c0 == '0' || c0 == '1' || c0 == '2') &&
                                            c1 >= '0' && c1 <= '9' && c2 >= '0' && c2 <= '9') {
                                            code_type = 0;
                                        } else if (c0 == '3' && c1 >= '0' && c1 <= '9' && c2 >= '0' && c2 <= '9') {
                                            code_type = 1;
                                            if ((c1 == '3' && c2 == '2') || (c1 == '5' && c2 == '3')) {
                                                is_long_line = 1;
                                            }
                                        } else if ((c0 == '4' || c0 == '5') &&
                                                   c1 >= '0' && c1 <= '9' && c2 >= '0' && c2 <= '9') {
                                            code_type = 2;
                                        }
                                    }
                                }
                            }
                        }
                        
                        /* scan for IRC command patterns - ELKS-optimized: single pass */
                        p = netbuf;
                        while (*p) {
                            if (*p == ' ') {
                                /* check NOTICE */
                                if (!has_notice && p[1] == 'N' && p[2] == 'O' && p[3] == 'T' &&
                                    p[4] == 'I' && p[5] == 'C' && p[6] == 'E' && p[7] == ' ') {
                                    has_notice = 1;
                                }
                                /* check JOIN */
                                if (!has_join && p[1] == 'J' && p[2] == 'O' && p[3] == 'I' &&
                                    p[4] == 'N' && p[5] == ' ') {
                                    has_join = 1;
                                }
                                /* check PRIVMSG */
                                if (!has_privmsg && p[1] == 'P' && p[2] == 'R' && p[3] == 'I' &&
                                    p[4] == 'V' && p[5] == 'M' && p[6] == 'S' && p[7] == 'G' && p[8] == ' ') {
                                    has_privmsg = 1;
                                    privmsg_pos = p + 1;
                                }
                            }
                            p++;
                        }
                        
                        /* check if PRIVMSG is from ourselves - ELKS-optimized: no strlen */
                        if (has_privmsg && netbuf[0] == ':') {
                            p = netbuf + 1;
                            const char *excl = strchr(p, '!');
                            if (excl) {
                                int len = excl - p;
                                const char *n = nick;
                                int match = 1;
                                while (len-- > 0 && *n) {
                                    if (*p++ != *n++) {
                                        match = 0;
                                        break;
                                    }
                                }
                                if (match && len < 0 && *n == '\0') {
                                    is_own_msg = 1;
                                }
                            }
                        }
                        /* format PRIVMSG to current channel as <nick> message - ELKS-optimized */
                        int formatted_privmsg = 0;
                        if (has_privmsg && privmsg_pos && netbuf[0] == ':' && current_target[0]) {
                            /* parse PRIVMSG: :nick!user@host PRIVMSG #channel :message */
                            const char *nick_start = netbuf + 1;
                            const char *excl = strchr(nick_start, '!');
                            if (excl) {
                                const char *target_start = privmsg_pos + 8; /* after "PRIVMSG " */
                                const char *target_end = strchr(target_start, ' ');
                                if (target_end) {
                                    const char *msg_start = strchr(target_end, ':');
                                    if (msg_start) {
                                        /* check if target matches current_target - ELKS-optimized: no strlen/strncmp */
                                        int target_len = target_end - target_start;
                                        const char *ct = current_target;
                                        int match = 1;
                                        const char *ts = target_start;
                                        while (target_len-- > 0 && *ct) {
                                            if (*ts++ != *ct++) {
                                                match = 0;
                                                break;
                                            }
                                        }
                                        if (match && target_len < 0 && *ct == '\0') {
                                            /* format as <nick> message with colors */
                                            int nick_len = excl - nick_start;
                                            if (color_mode) {
                                                ui_color_bold();
                                                putchar('<');
                                                ui_color_reset();
                                                ui_color_blue();
                                                fwrite(nick_start, 1, nick_len, stdout);
                                                ui_color_reset();
                                                ui_color_bold();
                                                putchar('>');
                                                ui_color_reset();
                                            } else {
                                                putchar('<');
                                                fwrite(nick_start, 1, nick_len, stdout);
                                                putchar('>');
                                            }
                                            putchar(' ');
                                            /* apply bold if own message */
                                            if (is_own_msg && color_mode) ui_color_bold();
                                            fputs(msg_start + 1, stdout); /* skip the ':' - already includes \n */
                                            if (is_own_msg && color_mode) ui_color_reset();
                                            formatted_privmsg = 1;
                                        }
                                    }
                                }
                            }
                        }

                        /* print message with selective coloring - ELKS-optimized */
                        /* Flow: 1) NOTICE -> purple, 2) ":server.domain XXX nick " pattern, 3) else normal */
                        if (formatted_privmsg) {
                            /* already printed in formatted form above */
                            /* reset long line color state at end of line */
                            long_line_color_active = 0;
                        } else if (has_notice) {
                            /* NOTICE: find ":server.domain NOTICE * :" and show only content after in purple */
                            /* ELKS-optimized: we already detected NOTICE, now find content */
                            const char *notice_content = NULL;
                            if (netbuf[0] == ':') {
                                const char *p = netbuf;
                                while (*p && *p != ' ') p++;
                                if (*p == ' ') {
                                    p++;
                                    /* check NOTICE - we know it's there, but verify position */
                                    if (p[0] == 'N' && p[1] == 'O' && p[2] == 'T' &&
                                        p[3] == 'I' && p[4] == 'C' && p[5] == 'E' && p[6] == ' ') {
                                        p += 7;
                                        if (*p == '*' && p[1] == ' ' && p[2] == ':') {
                                            notice_content = p + 3;
                                        } else {
                                            while (*p && *p != ':') p++;
                                            if (*p == ':' && (p[1] == ' ' || p[1] != '\0')) {
                                                notice_content = p + (p[1] == ' ' ? 2 : 1);
                                            }
                                        }
                                    }
                                }
                            }
                            if (notice_content && color_mode) {
                                ui_color_purple();
                                fputs(notice_content, stdout);
                                ui_color_reset();
                            } else if (notice_content) {
                                fputs(notice_content, stdout);
                            } else {
                                fputs(netbuf, stdout);
                            }
                            /* reset long line color state at end of line */
                            long_line_color_active = 0;
                        } else if (server_prefix_start && server_prefix_end && content_after_nick) {
                            /* found ":server.domain XXX nick " pattern */
                            int prefix_len = server_prefix_end - server_prefix_start;
                            /* show prefix only if debug enabled */
                            if (debug_mode) {
                                if (color_mode) {
                                    ui_color_grey();
                                    fwrite(server_prefix_start, 1, prefix_len+1, stdout);
                                    ui_color_reset();
                                } else {
                                    fwrite(server_prefix_start, 1, prefix_len, stdout);
                                }
                            }
                            /* apply color to content based on code type */
                            if (color_mode) {
                                if (code_type == 0) {
                                    /* 0XX/1XX/2XX: blue */
                                    ui_color_blue();
                                } else if (code_type == 1) {
                                    /* 3XX: green */
                                    ui_color_green();
                                } else if (code_type == 2) {
                                    /* 4XX/5XX: red */
                                    ui_color_red();
                                }
                            }
                            fputs(content_after_nick, stdout);
                            if (color_mode && code_type >= 0) {
                                ui_color_reset();
                            }
                            /* reset long line color state at end of line */
                            long_line_color_active = 0;
                        } else {
                            /* no color or other message types: print normally or with full color */
                            /* check if we have active color from long line fragments */
                            if (long_line_color_active > 0) {
                                /* continue with active color from previous fragments */
                                if (color_mode) {
                                    if (long_line_color_active == 1) {
                                        ui_color_blue();
                                    } else if (long_line_color_active == 2) {
                                        ui_color_green();
                                    } else if (long_line_color_active == 3) {
                                        ui_color_red();
                                    }
                                }
                            }
                            /* grey for server messages (start with ':') that aren't special types or PRIVMSG to current_target */
                            int is_server_msg = (netbuf[0] == ':' && !has_notice && !server_prefix_start &&
                                                !has_join && (!has_privmsg || !formatted_privmsg));
                            if (!long_line_color_active) {
                                /* only apply other colors if we don't have long line color active */
                                if (is_ping && color_mode) {
                                    /* PING messages in grey when printed (debug mode) */
                                    ui_color_grey();
                                } else if (is_own_msg && color_mode) ui_color_bold();
                                else if (has_join && color_mode) ui_color_green();
                                else if (is_server_msg && color_mode) ui_color_grey();
                            }
                            fputs(netbuf, stdout);
                            if (color_mode && (long_line_color_active > 0 || is_ping || is_own_msg || has_join || is_server_msg)) {
                                ui_color_reset();
                            }
                            /* reset long line color state at end of line */
                            long_line_color_active = 0;
                        }
                        fflush(stdout);
                        /* re-draw prompt for UX: only for new server messages or non-long-line messages */
                        if (server_prefix_start || !is_long_line) {
                            ui_redraw_after_output();
                        }
                        /* long lines don't get prompt redraw to avoid clutter */
                    } else {
                        /* PING message not printed - don't redraw prompt */
                    }
                    netpos = 0;
                    /* let network handler inspect (PING) */
                    handle_line(sock, netbuf);
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
                    /* don't echo here - server will send it back as PRIVMSG and we'll show it in bold */
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
