#ifndef SINGULARITY_LOGINUI_H
#define SINGULARITY_LOGINUI_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <cairo/cairo.h>
#include <wayland-client.h>

cairo_surface_t *loginui_load_wallpaper(const char *path, int target_w);
cairo_surface_t *loginui_load_avatar(const char *path, int size);
cairo_surface_t *loginui_load_image(const char *path, int max_w, int max_h);

struct loginui_buffer {
    void *data;
    size_t size;
    struct wl_buffer *wl_buffer;
};
struct loginui_buffer *loginui_create_buffer(struct wl_shm *shm,
                                             uint32_t w, uint32_t h,
                                             cairo_t **cr_out);

typedef struct {
    int width, height;
    const char *username;
    const char *time_str;
    const char *date_str;
    cairo_surface_t *avatar;
    cairo_surface_t *background;
    int password_dots;
    const char *status_text;
    bool status_error;
} LoginUiState;

/* align: 0 left, 1 centre, 2 right. */
void loginui_render(cairo_t *cr, const LoginUiState *st,
                    double *card_x, double *card_y, double *card_w, double *card_h);
void loginui_render_splash(cairo_t *cr, int width, int height,
                           cairo_surface_t *bg, cairo_surface_t *logo,
                           double phase);
void loginui_rounded_rect(cairo_t *cr, double x, double y, double w, double h, double r);
void loginui_text(cairo_t *cr, const char *font, const char *text,
                  double x, double y, int align, double r, double g, double b);
void loginui_text_size(cairo_t *cr, const char *font, const char *text, int *w, int *h);
void loginui_paint_background(cairo_t *cr, cairo_surface_t *bg, int width, int height);

#endif
