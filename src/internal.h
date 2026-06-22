#ifndef LOGINUI_INTERNAL_H
#define LOGINUI_INTERNAL_H

#include <cairo/cairo.h>

void lui_draw_text(cairo_t *cr, const char *desc, const char *text,
                   double x, double y, int align, double r, double g, double b);
void lui_text_size(cairo_t *cr, const char *desc, const char *text, int *w, int *h);
void lui_rounded_rect(cairo_t *cr, double x, double y, double w, double h, double r);

#endif
