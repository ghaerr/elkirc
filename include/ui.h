#ifndef UI_H
#define UI_H

#include "elkirc.h"

/* Initialize UI (if needed). Returns 0 on success. */
int ui_init(void);

/* Set current context (channel or nick), pass a NUL-terminated string */
/* Example: "#channel", "nick", or empty string "" for no context */
void ui_set_context(const char *ctx);

/* Draw prompt to stdout (no trailing newline). Caller should fflush(stdout). */
void ui_draw_prompt(void);

/* Called after printing server lines to re-draw prompt nicely. */
void ui_redraw_after_output(void);

/* Color functions - ELKS-optimized (only output if color_mode enabled) */
void ui_color_reset(void);
void ui_color_purple(void);
void ui_color_green(void);
void ui_color_red(void);
void ui_color_bold(void);
void ui_color_orange(void);
void ui_color_blue(void);
void ui_color_grey(void);

#endif /* UI_H */
