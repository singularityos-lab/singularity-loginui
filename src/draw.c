#include "loginui.h"
#include "internal.h"

#include <pango/pangocairo.h>
#include <stdlib.h>

static PangoFontDescription *lui_font(const char *desc) {
    PangoFontDescription *fd = pango_font_description_from_string(desc);
    if (!pango_font_description_get_family(fd))
        pango_font_description_set_family(fd, "Sans");
    return fd;
}

void lui_draw_text(cairo_t *cr, const char *desc, const char *text,
                   double x, double y, int align, double r, double g, double b) {
    PangoLayout *layout = pango_cairo_create_layout(cr);
    PangoFontDescription *fd = lui_font(desc);
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

static PangoLayout *lui_wrapped_layout(cairo_t *cr, const char *desc, const char *text, double width) {
    PangoLayout *layout = pango_cairo_create_layout(cr);
    PangoFontDescription *fd = lui_font(desc);
    pango_layout_set_font_description(layout, fd);
    pango_font_description_free(fd);
    pango_layout_set_width(layout, (int)(width * PANGO_SCALE));
    pango_layout_set_wrap(layout, PANGO_WRAP_WORD_CHAR);
    pango_layout_set_alignment(layout, PANGO_ALIGN_CENTER);
    pango_layout_set_text(layout, text, -1);
    return layout;
}

int lui_wrapped_height(cairo_t *cr, const char *desc, const char *text, double width) {
    PangoLayout *layout = lui_wrapped_layout(cr, desc, text, width);
    int tw, th;
    pango_layout_get_pixel_size(layout, &tw, &th);
    g_object_unref(layout);
    return th;
}

void lui_draw_wrapped(cairo_t *cr, const char *desc, const char *text,
                      double x, double y, double width, double r, double g, double b) {
    PangoLayout *layout = lui_wrapped_layout(cr, desc, text, width);
    cairo_set_source_rgb(cr, r, g, b);
    cairo_move_to(cr, x, y);
    pango_cairo_show_layout(cr, layout);
    g_object_unref(layout);
}

void lui_text_size(cairo_t *cr, const char *desc, const char *text, int *w, int *h) {
    PangoLayout *layout = pango_cairo_create_layout(cr);
    PangoFontDescription *fd = lui_font(desc);
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

typedef struct {
    cairo_surface_t *scaled;
    int w, h;
} CoverCache;

static const cairo_user_data_key_t cover_key;

static void cover_cache_free(void *data) {
    CoverCache *c = data;
    if (c->scaled) cairo_surface_destroy(c->scaled);
    free(c);
}

static cairo_surface_t *cover_scaled(cairo_surface_t *src, int dw, int dh) {
    CoverCache *c = cairo_surface_get_user_data(src, &cover_key);
    if (c && c->scaled && c->w == dw && c->h == dh) return c->scaled;
    if (!c) {
        c = calloc(1, sizeof *c);
        if (!c) return NULL;
        if (cairo_surface_set_user_data(src, &cover_key, c, cover_cache_free) != CAIRO_STATUS_SUCCESS) {
            free(c);
            return NULL;
        }
    }
    if (c->scaled) cairo_surface_destroy(c->scaled);
    c->scaled = cairo_image_surface_create(CAIRO_FORMAT_RGB24, dw, dh);
    c->w = dw;
    c->h = dh;
    int bw = cairo_image_surface_get_width(src);
    int bh = cairo_image_surface_get_height(src);
    double scale = (double)dw / bw;
    if ((double)dh / bh > scale) scale = (double)dh / bh;
    cairo_t *cr = cairo_create(c->scaled);
    cairo_translate(cr, (dw - bw * scale) / 2.0, (dh - bh * scale) / 2.0);
    cairo_scale(cr, scale, scale);
    cairo_set_source_surface(cr, src, 0, 0);
    cairo_pattern_set_filter(cairo_get_source(cr), CAIRO_FILTER_GOOD);
    cairo_paint(cr);
    cairo_destroy(cr);
    return c->scaled;
}

void lui_paint_cover(cairo_t *cr, cairo_surface_t *src, double w, double h, double alpha) {
    int bw = cairo_image_surface_get_width(src);
    int bh = cairo_image_surface_get_height(src);
    if (bw < 1 || bh < 1) return;
    double x0 = 0, y0 = 0, dw = w, dh = h;
    cairo_user_to_device(cr, &x0, &y0);
    cairo_user_to_device_distance(cr, &dw, &dh);
    cairo_matrix_t m;
    cairo_get_matrix(cr, &m);
    cairo_surface_t *scaled = NULL;
    if (m.xy == 0 && m.yx == 0 && dw >= 1 && dh >= 1)
        scaled = cover_scaled(src, (int)(dw + 0.5), (int)(dh + 0.5));
    cairo_save(cr);
    if (scaled) {
        cairo_identity_matrix(cr);
        cairo_set_source_surface(cr, scaled, x0, y0);
    } else {
        double scale = w / bw; if (h / bh > scale) scale = h / bh;
        cairo_translate(cr, (w - bw * scale) / 2.0, (h - bh * scale) / 2.0);
        cairo_scale(cr, scale, scale);
        cairo_set_source_surface(cr, src, 0, 0);
    }
    if (alpha >= 1.0) cairo_paint(cr);
    else cairo_paint_with_alpha(cr, alpha);
    cairo_restore(cr);
}

void loginui_paint_background(cairo_t *cr, cairo_surface_t *bg, int width, int height) {
    if (bg) {
        lui_paint_cover(cr, bg, width, height, 1.0);
    } else {
        cairo_set_source_rgb(cr, 0.10, 0.10, 0.11);
        cairo_paint(cr);
    }
}
