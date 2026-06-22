#include "loginui.h"
#include "internal.h"

#include <pango/pangocairo.h>

void lui_draw_text(cairo_t *cr, const char *desc, const char *text,
                   double x, double y, int align, double r, double g, double b) {
    PangoLayout *layout = pango_cairo_create_layout(cr);
    PangoFontDescription *fd = pango_font_description_from_string(desc);
    pango_layout_set_font_description(layout, fd);
    pango_font_description_free(fd);
    pango_layout_set_text(layout, text, -1);
    int tw, th;
    pango_layout_get_pixel_size(layout, &tw, &th);
    double tx = x;
    if (align == 1) tx = x - tw / 2.0;
    else if (align == 2) tx = x - tw;
    cairo_set_source_rgb(cr, r, g, b);
    cairo_move_to(cr, tx, y);
    pango_cairo_show_layout(cr, layout);
    g_object_unref(layout);
}

void lui_text_size(cairo_t *cr, const char *desc, const char *text, int *w, int *h) {
    PangoLayout *layout = pango_cairo_create_layout(cr);
    PangoFontDescription *fd = pango_font_description_from_string(desc);
    pango_layout_set_font_description(layout, fd);
    pango_font_description_free(fd);
    pango_layout_set_text(layout, text, -1);
    pango_layout_get_pixel_size(layout, w, h);
    g_object_unref(layout);
}

void lui_rounded_rect(cairo_t *cr, double x, double y, double w, double h, double r) {
    double deg = 3.14159265 / 180.0;
    cairo_new_sub_path(cr);
    cairo_arc(cr, x + w - r, y + r,     r, -90 * deg,   0);
    cairo_arc(cr, x + w - r, y + h - r, r,   0,        90 * deg);
    cairo_arc(cr, x + r,     y + h - r, r,  90 * deg,  180 * deg);
    cairo_arc(cr, x + r,     y + r,     r, 180 * deg,  270 * deg);
    cairo_close_path(cr);
}

void loginui_rounded_rect(cairo_t *cr, double x, double y, double w, double h, double r) {
    lui_rounded_rect(cr, x, y, w, h, r);
}

void loginui_text(cairo_t *cr, const char *font, const char *text,
                  double x, double y, int align, double r, double g, double b) {
    lui_draw_text(cr, font, text, x, y, align, r, g, b);
}

void loginui_text_size(cairo_t *cr, const char *font, const char *text, int *w, int *h) {
    lui_text_size(cr, font, text, w, h);
}

void loginui_paint_background(cairo_t *cr, cairo_surface_t *bg, int width, int height) {
    double w = width, h = height;
    if (bg) {
        int bw = cairo_image_surface_get_width(bg);
        int bh = cairo_image_surface_get_height(bg);
        double scale = w / bw; if (h / bh > scale) scale = h / bh;
        cairo_save(cr);
        cairo_translate(cr, (w - bw * scale) / 2.0, (h - bh * scale) / 2.0);
        cairo_scale(cr, scale, scale);
        cairo_set_source_surface(cr, bg, 0, 0);
        cairo_paint(cr);
        cairo_restore(cr);
    } else {
        cairo_set_source_rgb(cr, 0.10, 0.10, 0.11);
        cairo_paint(cr);
    }
}
