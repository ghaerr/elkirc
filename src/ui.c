#include <stdio.h>
#include <string.h>

#include "ui.h"

/* current_target is declared in elkirc.h and defined in main.c */
extern char current_target[BUF];
extern int color_mode;

/* initialize UI - currently no-op, kept for future */
int ui_init(void) {
    return 0;
}

/* display welcome banner - ELKS-optimized: simple fputs */
void ui_show_welcome(void) {
    fputs("\n", stdout);
    fputs(".,::::::   :::      :::  .   ::::::::::..     .,-:::::  \n", stdout);
    fputs(";;;;''''   ;;;      ;;; .;;,.;;;;;;;``;;;;  ,;;;'````'  \n", stdout);
    fputs(" [[cccc    [[[      [[[[[/'  [[[ [[[,/[[['  [[[         \n", stdout);
    fputs(" $$\"\"\"\"    $$'     _$$$$,    $$$ $$$$$$c    $$$         \n", stdout);
    fputs(" 888oo,__ o88oo,.__\"888\"88o, 888 888b \"88bo,`88bo,__,o, \n", stdout);
    fputs(" \"\"\"\"YUMMM\"\"\"\"YUMMM MMM \"MMP\"MMM MMMM   \"W\"   \"YUMMMMMP\"\n", stdout);
    fputs("\n", stdout);
    fputs("ELKIRC v0.1 - https://github.com/sepen/elkirc\n", stdout);
    fputs("\n", stdout);
    fflush(stdout);
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
        /* show channel or nick with colors if enabled */
        if (color_mode) {
            ui_color_bold();
            putchar('[');
            ui_color_reset();
            ui_color_orange();
            fputs(current_target, stdout);
            ui_color_reset();
            ui_color_bold();
            putchar(']');
            ui_color_reset();
            putchar(' ');
        } else {
            printf("[%s] ", current_target);
        }
    } else {
        if (color_mode) {
            ui_color_bold();
            printf("[*no-target] ");
            ui_color_reset();
        } else {
            printf("[*no-target] ");
        }
    }
    fflush(stdout);
}

/* after server output, reprint prompt on a new line */
void ui_redraw_after_output(void) {
    /* print a newline and re-draw */
    //putchar('\n');
    ui_draw_prompt();
}

/* Color functions - ELKS-optimized: simple ANSI codes */
void ui_color_reset(void) {
    if (color_mode) fputs("\033[0m", stdout);
}

void ui_color_purple(void) {
    if (color_mode) fputs("\033[35m", stdout);
}

void ui_color_green(void) {
    if (color_mode) fputs("\033[32m", stdout);
}

void ui_color_red(void) {
    if (color_mode) fputs("\033[31m", stdout);
}

void ui_color_bold(void) {
    if (color_mode) fputs("\033[1m", stdout);
}

void ui_color_orange(void) {
    if (color_mode) fputs("\033[33m", stdout); /* yellow/orange */
}

void ui_color_blue(void) {
    if (color_mode) fputs("\033[34m", stdout);
}

void ui_color_grey(void) {
    if (color_mode) fputs("\033[90m", stdout); /* bright black / grey */
}
