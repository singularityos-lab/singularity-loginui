#include "loginui.h"
#include "internal.h"

#include <gio/gio.h>

#define BLINK_MS 1200
#define BLINK_STOP_MS 10000

static struct {
    bool shown, lit;
    int dots;
    gint64 since;
} caret;

static GSettings *interface_settings(void) {
    static GSettings *settings;
    static bool looked;
    if (looked) return settings;
    looked = true;
    GSettingsSchemaSource *source = g_settings_schema_source_get_default();
    if (!source) return NULL;
    GSettingsSchema *schema = g_settings_schema_source_lookup(source, "org.gnome.desktop.interface", TRUE);
    if (!schema) return NULL;
    settings = g_settings_new_full(schema, NULL, NULL);
    g_settings_schema_unref(schema);
    return settings;
}

static gint64 now_ms(void) {
    return g_get_monotonic_time() / 1000;
}

static gint64 half_period(void) {
    GSettings *settings = interface_settings();
    int period = BLINK_MS;
    if (settings) {
        if (!g_settings_get_boolean(settings, "cursor-blink")) return 0;
        period = g_settings_get_int(settings, "cursor-blink-time");
    }
    if (loginui_reduced_motion()) return 0;
    return period >= 100 ? period / 2 : 0;
}

static bool lit_at(gint64 elapsed, gint64 half) {
    return !half || elapsed >= BLINK_STOP_MS || (elapsed / half) % 2 == 0;
}

void lui_caret_hide(void) {
    caret.shown = false;
}

bool lui_caret_show(int dots) {
    gint64 now = now_ms();
    if (!caret.since || dots != caret.dots) caret.since = now;
    caret.dots = dots;
    caret.shown = true;
    caret.lit = lit_at(now - caret.since, half_period());
    return caret.lit;
}

bool loginui_caret_tick(void) {
    return caret.shown && caret.lit != lit_at(now_ms() - caret.since, half_period());
}

int loginui_caret_timeout(int fallback) {
    if (!caret.shown) return fallback;
    gint64 half = half_period(), elapsed = now_ms() - caret.since;
    if (!half || elapsed >= BLINK_STOP_MS) return fallback;
    gint64 left = half - elapsed % half;
    return fallback >= 0 && left >= fallback ? fallback : (int)left;
}

int loginui_password_chars(const char *text, size_t len) {
    int n = 0;
    for (size_t i = 0; i < len; i++)
        if (((unsigned char)text[i] & 0xC0) != 0x80) n++;
    return n;
}
