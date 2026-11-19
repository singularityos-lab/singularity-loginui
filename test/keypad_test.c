#include "loginui.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <cairo/cairo.h>
#include <xkbcommon/xkbcommon.h>

static struct xkb_keymap *keymap(struct xkb_context *ctx, const char *layout) {
    struct xkb_rule_names names = { .layout = layout };
    return xkb_keymap_new_from_names(ctx, &names, XKB_KEYMAP_COMPILE_NO_FLAGS);
}

static int find(LoginUiKeypad *k, const char *text) {
    for (int i = 0; i < k->n_keys; i++)
        if (!strcmp(k->keys[i].text, text)) return i;
    return -1;
}

static bool has_symbol(LoginUiKeypad *k, const char *t) {
    for (int i = 0; i < k->n_symbols; i++)
        if (!strcmp(k->symbols[i], t)) return true;
    return false;
}

static void tap(LoginUiKeypad *k, cairo_t *cr, int index, char *out, size_t size, LoginUiKeypadAction want) {
    const LoginUiKey *key = &k->keys[index];
    assert(loginui_keypad_key_at(k, key->x + key->w / 2, key->y + key->h / 2) == index);
    assert(loginui_keypad_activate(k, index, out, size) == want);
    loginui_keypad_render(cr, k, 0, 0, 480);
}

int main(void) {
    struct xkb_context *ctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    assert(ctx);
    cairo_surface_t *s = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 480, 400);
    cairo_t *cr = cairo_create(s);
    char out[8];

    struct xkb_keymap *us = keymap(ctx, "us");
    assert(us);
    LoginUiKeypad k;
    loginui_keypad_init(&k, us, LOGINUI_KEYPAD_LETTERS);
    loginui_keypad_render(cr, &k, 0, 0, 480);
    assert(!strcmp(k.letters[0][0], "q") && !strcmp(k.letters[0][1], "Q"));
    assert(find(&k, "q") >= 0 && find(&k, "m") >= 0);
    assert(has_symbol(&k, "@") && has_symbol(&k, "#") && has_symbol(&k, "!"));
    assert(!has_symbol(&k, "a") && !has_symbol(&k, "1"));

    tap(&k, cr, find(&k, "Shift"), out, sizeof out, LOGINUI_KEYPAD_NONE);
    assert(k.shift && find(&k, "Q") >= 0);
    tap(&k, cr, find(&k, "Q"), out, sizeof out, LOGINUI_KEYPAD_TEXT);
    assert(!strcmp(out, "Q") && !k.shift && find(&k, "q") >= 0);
    tap(&k, cr, find(&k, "Enter"), out, sizeof out, LOGINUI_KEYPAD_ENTER);
    tap(&k, cr, find(&k, "?123"), out, sizeof out, LOGINUI_KEYPAD_NONE);
    assert(k.page == LOGINUI_KEYPAD_SYMBOLS && find(&k, "7") >= 0);
    tap(&k, cr, find(&k, "7"), out, sizeof out, LOGINUI_KEYPAD_TEXT);
    assert(!strcmp(out, "7"));

    struct xkb_keymap *it = keymap(ctx, "it");
    assert(it);
    loginui_keypad_set_keymap(&k, it);
    assert(has_symbol(&k, "@") && has_symbol(&k, "#"));
    assert(has_symbol(&k, "\xc3\xa8") && has_symbol(&k, "\xc3\xb2"));
    loginui_keypad_set_page(&k, LOGINUI_KEYPAD_SYMBOLS);
    loginui_keypad_render(cr, &k, 0, 0, 480);
    assert(find(&k, "@") >= 0 && find(&k, "#") >= 0 && find(&k, "!") >= 0);
    assert(!strcmp(k.symbols[0], "@") && !strcmp(k.symbols[1], "#"));
    bool found_at = false;
    int pages = 0;
    do {
        loginui_keypad_render(cr, &k, 0, 0, 480);
        if (find(&k, "@") >= 0) found_at = true;
        int more = -1;
        for (int i = 0; i < k.n_keys; i++) if (strlen(k.keys[i].text) == 3 && k.keys[i].text[1] == '/') { more = i; break; }
        if (more < 0) break;
        loginui_keypad_activate(&k, more, out, sizeof out);
        pages++;
    } while (k.symbol_page != 0 && pages < 10);
    assert(found_at);

    loginui_keypad_set_page(&k, LOGINUI_KEYPAD_DIGITS);
    loginui_keypad_render(cr, &k, 0, 0, 480);
    assert(k.n_keys == 13);
    tap(&k, cr, find(&k, "0"), out, sizeof out, LOGINUI_KEYPAD_TEXT);
    assert(!strcmp(out, "0"));
    int back = -1;
    for (int i = 0; i < k.n_keys; i++) if (!k.keys[i].text[0]) back = i;
    tap(&k, cr, back, out, sizeof out, LOGINUI_KEYPAD_BACKSPACE);
    tap(&k, cr, find(&k, "ABC"), out, sizeof out, LOGINUI_KEYPAD_NONE);
    assert(k.page == LOGINUI_KEYPAD_LETTERS);

    assert(loginui_compact(480, 1040) && loginui_compact(600, 400) && !loginui_compact(1920, 1080));

    xkb_keymap_unref(us);
    xkb_keymap_unref(it);
    xkb_context_unref(ctx);
    cairo_destroy(cr);
    cairo_surface_destroy(s);
    return 0;
}
