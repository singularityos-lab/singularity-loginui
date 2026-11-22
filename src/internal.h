#ifndef LOGINUI_INTERNAL_H
#define LOGINUI_INTERNAL_H

#include <cairo/cairo.h>
#include <gio/gio.h>

#include "loginui.h"

void lui_draw_text(cairo_t *cr, const char *desc, const char *text,
                   double x, double y, int align, double r, double g, double b);
void lui_draw_color(cairo_t *cr, const char *desc, const char *text,
                    double x, double y, int align, LoginUiColor color);
void lui_text_size(cairo_t *cr, const char *desc, const char *text, int *w, int *h);
int lui_wrapped_height(cairo_t *cr, const char *desc, const char *text, double width);
void lui_draw_wrapped(cairo_t *cr, const char *desc, const char *text,
                      double x, double y, double width, LoginUiColor color);
void lui_paint_cover(cairo_t *cr, cairo_surface_t *src, double w, double h, double alpha);
void lui_rounded_rect(cairo_t *cr, double x, double y, double w, double h, double r);
GSettingsSchema *lui_desktop_schema(void);
void lui_draw_fit(cairo_t *cr, const char *desc, const char *text,
                  double x, double middle, double width, LoginUiColor color);
void lui_caret_hide(void);
bool lui_caret_show(int dots);

#endif
