#include "loginui.h"

#include <stdlib.h>
#include <string.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

static GdkPixbuf *box_blur(GdkPixbuf *src, int radius) {
    int w = gdk_pixbuf_get_width(src), h = gdk_pixbuf_get_height(src);
    int nch = gdk_pixbuf_get_n_channels(src);
    int srs = gdk_pixbuf_get_rowstride(src);
    const guint8 *s = gdk_pixbuf_get_pixels(src);
    int count = 2 * radius + 1;

    GdkPixbuf *tmp = gdk_pixbuf_new(GDK_COLORSPACE_RGB, gdk_pixbuf_get_has_alpha(src), 8, w, h);
    int trs = gdk_pixbuf_get_rowstride(tmp);
    guint8 *t = gdk_pixbuf_get_pixels(tmp);
    for (int y = 0; y < h; y++) {
        const guint8 *srow = s + y * srs;
        guint8 *trow = t + y * trs;
        for (int c = 0; c < nch; c++) {
            int sum = 0;
            for (int k = -radius; k <= radius; k++) {
                int xx = k < 0 ? 0 : (k >= w ? w - 1 : k);
                sum += srow[xx * nch + c];
            }
            for (int x = 0; x < w; x++) {
                trow[x * nch + c] = (guint8)(sum / count);
                int xout = x - radius; if (xout < 0) xout = 0;
                int xin = x + radius + 1; if (xin >= w) xin = w - 1;
                sum += srow[xin * nch + c] - srow[xout * nch + c];
            }
        }
    }
    GdkPixbuf *dst = gdk_pixbuf_new(GDK_COLORSPACE_RGB, gdk_pixbuf_get_has_alpha(src), 8, w, h);
    int drs = gdk_pixbuf_get_rowstride(dst);
    guint8 *d = gdk_pixbuf_get_pixels(dst);
    for (int x = 0; x < w; x++) {
        for (int c = 0; c < nch; c++) {
            int sum = 0;
            for (int k = -radius; k <= radius; k++) {
                int yy = k < 0 ? 0 : (k >= h ? h - 1 : k);
                sum += t[yy * trs + x * nch + c];
            }
            for (int y = 0; y < h; y++) {
                d[y * drs + x * nch + c] = (guint8)(sum / count);
                int yout = y - radius; if (yout < 0) yout = 0;
                int yin = y + radius + 1; if (yin >= h) yin = h - 1;
                sum += t[yin * trs + x * nch + c] - t[yout * trs + x * nch + c];
            }
        }
    }
    g_object_unref(tmp);
    return dst;
}

static cairo_surface_t *pixbuf_to_surface(GdkPixbuf *pb) {
    int w = gdk_pixbuf_get_width(pb), h = gdk_pixbuf_get_height(pb);
    int nch = gdk_pixbuf_get_n_channels(pb);
    int prs = gdk_pixbuf_get_rowstride(pb);
    const guint8 *pix = gdk_pixbuf_get_pixels(pb);
    cairo_surface_t *s = cairo_image_surface_create(CAIRO_FORMAT_RGB24, w, h);
    unsigned char *data = cairo_image_surface_get_data(s);
    int crs = cairo_image_surface_get_stride(s);
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            const guint8 *p = pix + y * prs + x * nch;
            uint32_t *dp = (uint32_t *)(data + y * crs + x * 4);
            *dp = ((uint32_t)p[0] << 16) | ((uint32_t)p[1] << 8) | (uint32_t)p[2];
        }
    }
    cairo_surface_mark_dirty(s);
    return s;
}

cairo_surface_t *loginui_load_wallpaper(const char *path, int target_w) {
    if (!path || path[0] == '\0') return NULL;
    if (g_str_has_prefix(path, "file://")) path += 7;
    GdkPixbuf *pb = gdk_pixbuf_new_from_file_at_scale(path, target_w > 0 ? target_w : 960, -1, TRUE, NULL);
    if (!pb) return NULL;
    GdkPixbuf *b1 = box_blur(pb, 16);
    GdkPixbuf *b2 = box_blur(b1, 16);
    g_object_unref(pb);
    g_object_unref(b1);
    cairo_surface_t *s = pixbuf_to_surface(b2);
    g_object_unref(b2);
    return s;
}

cairo_surface_t *loginui_load_avatar(const char *path, int size) {
    if (!path || path[0] == '\0') return NULL;
    GdkPixbuf *pb = gdk_pixbuf_new_from_file_at_scale(path, size, size, FALSE, NULL);
    if (!pb) return NULL;
    cairo_surface_t *s = pixbuf_to_surface(pb);
    g_object_unref(pb);
    return s;
}

cairo_surface_t *loginui_load_image(const char *path, int max_w, int max_h) {
    if (!path || path[0] == '\0') return NULL;
    GdkPixbuf *pb = gdk_pixbuf_new_from_file_at_scale(path, max_w, max_h, TRUE, NULL);
    if (!pb) return NULL;
    int w = gdk_pixbuf_get_width(pb), h = gdk_pixbuf_get_height(pb);
    int nch = gdk_pixbuf_get_n_channels(pb);
    gboolean alpha = gdk_pixbuf_get_has_alpha(pb);
    int prs = gdk_pixbuf_get_rowstride(pb);
    const guint8 *pix = gdk_pixbuf_get_pixels(pb);
    cairo_surface_t *s = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h);
    unsigned char *data = cairo_image_surface_get_data(s);
    int crs = cairo_image_surface_get_stride(s);
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            const guint8 *p = pix + y * prs + x * nch;
            uint32_t a = alpha ? p[3] : 255;
            uint32_t r = p[0] * a / 255, g = p[1] * a / 255, b = p[2] * a / 255;
            uint32_t *dp = (uint32_t *)(data + y * crs + x * 4);
            *dp = (a << 24) | (r << 16) | (g << 8) | b;
        }
    }
    cairo_surface_mark_dirty(s);
    g_object_unref(pb);
    return s;
}
