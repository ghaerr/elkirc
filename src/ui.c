#include <stdio.h>
#include <string.h>

#include "ui.h"

/* current_target is declared in elkirc.h and defined in main.c */
extern char current_target[BUF];

/* initialize UI - currently no-op, kept for future */
int ui_init(void) {
    return 0;
}

/* copy small string into current_target via extern */
void ui_set_context(const char *ctx) {
    if (!ctx || !*ctx) {
        current_target[0] = 0;
        return;
    }
    /* copy up to BUF-1 */
    size_t i = 0;
    while (i < (BUF - 1) && ctx[i]) { current_target[i] = ctx[i]; i++; }
    current_target[i] = 0;
}

/* draw prompt: show [context] >  or [*no-target] > */
void ui_draw_prompt(void) {
    if (current_target[0]) {
        /* show channel or nick */
        printf("[%s] > ", current_target);
    } else {
        printf("[*no-target] > ");
    }
    fflush(stdout);
}

/* after server output, reprint prompt on a new line */
void ui_redraw_after_output(void) {
    /* print a newline and re-draw */
    //putchar('\n');
    ui_draw_prompt();
}
