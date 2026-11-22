#include "loginui.h"
#include "internal.h"

#include <gio/gio.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#define CONTRAST_MIN 3.0
#define WINDOW_TINT 0.08

static const struct {
    const char *name;
    const char *hex;
} accents[] = {
    { "blue", "#3584e4" }, { "teal", "#2190a4" }, { "green", "#3a944a" },
    { "yellow", "#e5a50a" }, { "orange", "#e66100" }, { "red", "#e01b24" },
    { "pink", "#d56199" }, { "purple", "#9141ac" }, { "slate", "#787878" },
};

static const LoginUiColor white = { 1, 1, 1, 1 };
static const LoginUiColor black = { 0, 0, 0, 1 };
static const LoginUiColor window_bg = { 0x24 / 255.0, 0x24 / 255.0, 0x24 / 255.0, 1 };
static const LoginUiColor dark_fg = { 0x1c / 255.0, 0x1c / 255.0, 0x1c / 255.0, 1 };

static LoginUiColor mix(LoginUiColor from, LoginUiColor to, double f) {
    return (LoginUiColor){ from.r + (to.r - from.r) * f, from.g + (to.g - from.g) * f,
                           from.b + (to.b - from.b) * f, 1 };
}

static LoginUiColor alpha(LoginUiColor c, double a) {
    c.a = a;
    return c;
}

static double linear(double c) {
    return c <= 0.04045 ? c / 12.92 : pow((c + 0.055) / 1.055, 2.4);
}

static double luminance(LoginUiColor c) {
    return 0.2126 * linear(c.r) + 0.7152 * linear(c.g) + 0.0722 * linear(c.b);
}

static double contrast(LoginUiColor a, LoginUiColor b) {
    double la = luminance(a), lb = luminance(b);
    return (fmax(la, lb) + 0.05) / (fmin(la, lb) + 0.05);
}

static LoginUiColor readable(LoginUiColor c) {
    for (int step = 0; step <= 20; step++) {
        LoginUiColor shade = mix(c, white, step / 20.0);
        if (contrast(shade, window_bg) >= CONTRAST_MIN) return shade;
    }
    return white;
}

static bool parse_hex(const char *hex, LoginUiColor *out) {
    unsigned r, g, b;
    if (!hex || strlen(hex) < 7 || sscanf(hex, "#%2x%2x%2x", &r, &g, &b) != 3) return false;
    *out = (LoginUiColor){ r / 255.0, g / 255.0, b / 255.0, 1 };
    return true;
}

static GSettings *desktop_settings(void) {
    static GSettings *settings;
    static bool looked;
    if (looked) return settings;
    looked = true;
    GSettingsSchema *schema = lui_desktop_schema();
    if (!schema) return NULL;
    if (g_settings_schema_has_key(schema, "accent-color") && g_settings_schema_has_key(schema, "custom-accent-color"))
        settings = g_settings_new_full(schema, NULL, NULL);
    g_settings_schema_unref(schema);
    return settings;
}

static LoginUiColor accent_color(void) {
    LoginUiColor c = white;
    parse_hex(accents[0].hex, &c);
    GSettings *settings = desktop_settings();
    if (!settings) return c;
    char *name = g_settings_get_string(settings, "accent-color");
    if (!strcmp(name, "custom")) {
        char *hex = g_settings_get_string(settings, "custom-accent-color");
        parse_hex(hex, &c);
        g_free(hex);
    } else {
        for (size_t i = 0; i < G_N_ELEMENTS(accents); i++)
            if (!strcmp(name, accents[i].name)) parse_hex(accents[i].hex, &c);
    }
    g_free(name);
    return c;
}

static double window_tint(void) {
    GSettings *settings = desktop_settings();
    if (!settings) return WINDOW_TINT;
    GSettingsSchema *schema = NULL;
    g_object_get(settings, "settings-schema", &schema, NULL);
    double tint = WINDOW_TINT;
    if (g_settings_schema_has_key(schema, "glass-tint"))
        tint = CLAMP(g_settings_get_int(settings, "glass-tint"), 0, 40) / 100.0;
    g_settings_schema_unref(schema);
    return tint;
}

const LoginUiStyle *loginui_style(void) {
    static LoginUiStyle s;
    static LoginUiColor last;
    static double last_tint;
    static bool ready;
    LoginUiColor picked = accent_color();
    double tint = window_tint();
    if (ready && tint == last_tint && picked.r == last.r && picked.g == last.g && picked.b == last.b) return &s;
    last_tint = tint;
    ready = true;
    last = picked;

    LoginUiColor accent = readable(picked);
    LoginUiColor key = mix(white, accent, 0.3);
    s.accent = accent;
    s.accent_fg = contrast(accent, white) >= CONTRAST_MIN || contrast(accent, white) >= contrast(accent, dark_fg) ? white : dark_fg;
    s.text = white;
    s.dim = alpha(white, 0.65);
    parse_hex("#f87171", &s.error);
    s.surface = mix(window_bg, accent, tint);
    s.border = alpha(white, 0.10);
    s.card = alpha(white, 0.08);
    s.field = alpha(white, 0.09);
    s.focus = mix(accent, white, 0.3);
    s.panel = black;
    s.panel_border = alpha(accent, 0.22);
    s.key = alpha(key, 0.10);
    s.key_special = alpha(accent, 0.16);
    s.key_pressed = alpha(accent, 0.35);
    return &s;
}

void loginui_set_color(cairo_t *cr, LoginUiColor c) {
    cairo_set_source_rgba(cr, c.r, c.g, c.b, c.a);
}

void loginui_surface(cairo_t *cr, double x, double y, double w, double h, double radius) {
    const LoginUiStyle *s = loginui_style();
    lui_rounded_rect(cr, x, y, w, h, radius);
    loginui_set_color(cr, s->surface);
    cairo_fill(cr);
    lui_rounded_rect(cr, x + 0.5, y + 0.5, w - 1, h - 1, radius);
    loginui_set_color(cr, s->border);
    cairo_set_line_width(cr, 1);
    cairo_stroke(cr);
}
