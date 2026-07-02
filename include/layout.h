#ifndef NEX_LAYOUT_H
#define NEX_LAYOUT_H

#include "types.h"

/* Each layout owns its own geometry math and, where relevant, its own
 * leaf-ordering logic. None of them share state with each other. */

void layout_binary_arrange(monitor_t *m, desktop_t *d, xcb_rectangle_t rect);
void layout_monocle_arrange(monitor_t *m, desktop_t *d, xcb_rectangle_t rect);
void layout_tall_arrange(monitor_t *m, desktop_t *d, xcb_rectangle_t rect);
void layout_wide_arrange(monitor_t *m, desktop_t *d, xcb_rectangle_t rect);

/* String/char representations, used by query.c (JSON), desktop.c (status
 * events) and subscribe.c (status bar feed). Kept here instead of a macro
 * in helpers.h since they need layout_t, which isn't visible that early. */
const char *layout_str(layout_t l);
char layout_chr(layout_t l);
const char *layout_variant_str(layout_variant_t v);

#endif
