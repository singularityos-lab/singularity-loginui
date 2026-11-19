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

struct xkb_keymap;

typedef enum {
    LOGINUI_KEYPAD_LETTERS,
    LOGINUI_KEYPAD_SYMBOLS,
    LOGINUI_KEYPAD_DIGITS,
} LoginUiKeypadPage;

typedef enum {
    LOGINUI_KEYPAD_NONE,
    LOGINUI_KEYPAD_TEXT,
    LOGINUI_KEYPAD_BACKSPACE,
    LOGINUI_KEYPAD_ENTER,
} LoginUiKeypadAction;

#define LOGINUI_KEYPAD_LETTERS_MAX 26
#define LOGINUI_KEYPAD_SYMBOLS_MAX 96
#define LOGINUI_KEYPAD_KEYS_MAX 48

typedef struct {
    int kind;
    char text[8];
    double weight;
    double x, y, w, h;
} LoginUiKey;

typedef struct {
    LoginUiKeypadPage page;
    bool shift;
    bool caps;
    int symbol_page;
    int pressed;
    char letters[LOGINUI_KEYPAD_LETTERS_MAX][2][8];
    char symbols[LOGINUI_KEYPAD_SYMBOLS_MAX][8];
    int n_symbols;
    LoginUiKey keys[LOGINUI_KEYPAD_KEYS_MAX];
    int n_keys;
} LoginUiKeypad;

void loginui_keypad_init(LoginUiKeypad *k, struct xkb_keymap *keymap, LoginUiKeypadPage page);
void loginui_keypad_set_keymap(LoginUiKeypad *k, struct xkb_keymap *keymap);
void loginui_keypad_set_page(LoginUiKeypad *k, LoginUiKeypadPage page);
double loginui_keypad_height(const LoginUiKeypad *k);
void loginui_keypad_render(cairo_t *cr, LoginUiKeypad *k, double x, double y, double w);
int loginui_keypad_key_at(const LoginUiKeypad *k, double x, double y);
LoginUiKeypadAction loginui_keypad_activate(LoginUiKeypad *k, int key, char *text, size_t size);

bool loginui_compact(int width, int height);

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
    const char *auth_label; /* field label + placeholder; NULL -> "Password" */
    cairo_surface_t *background_sharp;
    double sharpness;
    double clock_dy;
    double card_dx;
    double content_fade;
    double cover;
    LoginUiKeypad *keypad;
} LoginUiState;

/* align: 0 left, 1 centre, 2 right. */
void loginui_render(cairo_t *cr, const LoginUiState *st,
                    double *card_x, double *card_y, double *card_w, double *card_h);
void loginui_render_splash(cairo_t *cr, int width, int height,
                           cairo_surface_t *bg, cairo_surface_t *logo,
                           double phase);
#define LOGINUI_BRAND_RED 0.0
#define LOGINUI_BRAND_GREEN 0.0
#define LOGINUI_BRAND_BLUE 0.0
#define LOGINUI_BRAND_LOGO_SIZE 256
#define LOGINUI_BRAND_LOGO_PATH "/usr/share/singularity/splash-logo.png"
#define LOGINUI_MOTION_SMALL_MS 140
#define LOGINUI_MOTION_SCENE_MS 600

cairo_surface_t *loginui_load_brand_logo(void);
void loginui_render_brand(cairo_t *cr, int width, int height,
                          cairo_surface_t *logo, double track_alpha,
                          double phase);
bool loginui_reduced_motion(void);

typedef struct {
    double logo_x, logo_y, logo_size;
    double bar_x, bar_y, bar_w, bar_h;
} LoginUiBrandLayout;

#define LOGINUI_PROGRESS_PATH "/run/boot-progress"

void loginui_brand_layout(double width, double height, double scale,
                          LoginUiBrandLayout *out);
void loginui_render_brand_progress(cairo_t *cr, int width, int height,
                                   double scale, cairo_surface_t *logo,
                                   double progress, double bar_alpha);
double loginui_progress_curve(double from, double to, double elapsed,
                              double tau);
bool loginui_progress_read(const char *path, double *value);
void loginui_progress_write(const char *path, double value);
const char *loginui_gpu_notice(const char *sysfs);
void loginui_brand_notice_box(cairo_t *cr, double width, double height, double scale,
                              const char *text, double *x, double *y, double *w, double *h);
void loginui_render_brand_notice(cairo_t *cr, int width, int height, double scale,
                                 const char *text, double alpha);
#define LOGINUI_CHANNEL_PATH "/var/lib/sinty/update-channel"

const char *loginui_update_channel(const char *path);
void loginui_brand_channel_box(cairo_t *cr, double width, double height, double scale,
                               const char *label, double *x, double *y, double *w, double *h);
void loginui_render_brand_channel(cairo_t *cr, int width, int height, double scale,
                                  const char *label, double alpha);
double loginui_scene_seconds(void);
void loginui_rounded_rect(cairo_t *cr, double x, double y, double w, double h, double r);
void loginui_text(cairo_t *cr, const char *font, const char *text,
                  double x, double y, int align, double r, double g, double b);
void loginui_text_size(cairo_t *cr, const char *font, const char *text, int *w, int *h);
void loginui_paint_background(cairo_t *cr, cairo_surface_t *bg, int width, int height);

#endif
