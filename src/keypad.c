#include "loginui.h"
#include "internal.h"

#include <stdio.h>
#include <string.h>
#include <xkbcommon/xkbcommon.h>

enum {
    KEY_CHAR,
    KEY_SPACER,
    KEY_SHIFT,
    KEY_BACKSPACE,
    KEY_ENTER,
    KEY_SPACE,
    KEY_LETTERS,
    KEY_SYMBOLS,
    KEY_MORE,
};

#define PAD_TOP 10.0
#define PAD_SIDE 12.0
#define PAD_BOTTOM 14.0
#define GAP 6.0
#define LETTER_KEY_H 46.0
#define DIGIT_KEY_H 54.0
#define SYMBOLS_PER_PAGE 17

static const int letter_codes[LOGINUI_KEYPAD_LETTERS_MAX] = {
    16, 17, 18, 19, 20, 21, 22, 23, 24, 25,
    30, 31, 32, 33, 34, 35, 36, 37, 38,
    44, 45, 46, 47, 48, 49, 50,
};
static const int letter_row_len[3] = { 10, 9, 7 };

static const int symbol_codes[] = {
    2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13,
    26, 27, 39, 40, 41, 43, 51, 52, 53, 86,
    16, 17, 18, 19, 20, 21, 22, 23, 24, 25,
    30, 31, 32, 33, 34, 35, 36, 37, 38,
    44, 45, 46, 47, 48, 49, 50,
};

static bool key_text(struct xkb_keymap *km, int code, int level, char *out, size_t size) {
    const xkb_keysym_t *syms;
    out[0] = '\0';
    if (xkb_keymap_key_get_syms_by_level(km, (xkb_keycode_t)(code + 8), 0, (xkb_level_index_t)level, &syms) != 1)
        return false;
    char buf[8];
    int n = xkb_keysym_to_utf8(syms[0], buf, sizeof buf);
    if (n < 2 || (unsigned char)buf[0] < 0x20 || buf[0] == 0x7f) return false;
    snprintf(out, size, "%s", buf);
    return true;
}

static bool is_letter(const LoginUiKeypad *k, const char *t) {
    for (int i = 0; i < LOGINUI_KEYPAD_LETTERS_MAX; i++)
        if (!strcmp(k->letters[i][0], t) || !strcmp(k->letters[i][1], t)) return true;
    return false;
}

static void add_symbol(LoginUiKeypad *k, const char *t) {
    if (!t[0] || !strcmp(t, " ") || (t[0] >= '0' && t[0] <= '9' && !t[1])) return;
    if (is_letter(k, t)) return;
    for (int i = 0; i < k->n_symbols; i++)
        if (!strcmp(k->symbols[i], t)) return;
    if (k->n_symbols < LOGINUI_KEYPAD_SYMBOLS_MAX)
        snprintf(k->symbols[k->n_symbols++], sizeof k->symbols[0], "%s", t);
}

static void load_keymap(LoginUiKeypad *k, struct xkb_keymap *keymap) {
    struct xkb_context *ctx = NULL;
    struct xkb_keymap *km = keymap;
    if (!km) {
        ctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
        if (ctx) km = xkb_keymap_new_from_names(ctx, NULL, XKB_KEYMAP_COMPILE_NO_FLAGS);
    }
    memset(k->letters, 0, sizeof k->letters);
    k->n_symbols = 0;
    if (km) {
        for (int i = 0; i < LOGINUI_KEYPAD_LETTERS_MAX; i++) {
            key_text(km, letter_codes[i], 0, k->letters[i][0], sizeof k->letters[i][0]);
            if (!key_text(km, letter_codes[i], 1, k->letters[i][1], sizeof k->letters[i][1]))
                snprintf(k->letters[i][1], sizeof k->letters[i][1], "%s", k->letters[i][0]);
        }
        for (int ascii = 1; ascii >= 0; ascii--) {
            for (size_t c = 0; c < sizeof symbol_codes / sizeof symbol_codes[0]; c++) {
                int levels = (int)xkb_keymap_num_levels_for_key(km, (xkb_keycode_t)(symbol_codes[c] + 8), 0);
                if (levels > 8) levels = 8;
                for (int l = 0; l < levels; l++) {
                    char t[8];
                    if (!key_text(km, symbol_codes[c], l, t, sizeof t)) continue;
                    if (((unsigned char)t[0] < 0x80) == (ascii == 1)) add_symbol(k, t);
                }
            }
        }
        static const char common[] = "@#!?.,-_*&%$+=/:;()'\"";
        int front = 0;
        for (const char *c = common; *c; c++) {
            for (int i = front; i < k->n_symbols; i++) {
                if (k->symbols[i][0] != *c || k->symbols[i][1]) continue;
                char tmp[8];
                memcpy(tmp, k->symbols[i], sizeof tmp);
                memmove(k->symbols[front + 1], k->symbols[front], (size_t)(i - front) * sizeof k->symbols[0]);
                memcpy(k->symbols[front], tmp, sizeof tmp);
                front++;
                break;
            }
        }
    }
    if (km && km != keymap) xkb_keymap_unref(km);
    if (ctx) xkb_context_unref(ctx);
}

void loginui_keypad_init(LoginUiKeypad *k, struct xkb_keymap *keymap, LoginUiKeypadPage page) {
    memset(k, 0, sizeof *k);
    k->pressed = -1;
    k->page = page;
    load_keymap(k, keymap);
}

void loginui_keypad_set_keymap(LoginUiKeypad *k, struct xkb_keymap *keymap) {
    load_keymap(k, keymap);
    k->symbol_page = 0;
    k->pressed = -1;
}

void loginui_keypad_set_page(LoginUiKeypad *k, LoginUiKeypadPage page) {
    k->page = page;
    k->shift = false;
    k->caps = false;
    k->symbol_page = 0;
    k->pressed = -1;
}

static int rows_for(const LoginUiKeypad *k) {
    return k->page == LOGINUI_KEYPAD_DIGITS ? 5 : 4;
}

static double key_h_for(const LoginUiKeypad *k) {
    return k->page == LOGINUI_KEYPAD_DIGITS ? DIGIT_KEY_H : LETTER_KEY_H;
}

double loginui_keypad_height(const LoginUiKeypad *k) {
    int rows = rows_for(k);
    return PAD_TOP + rows * key_h_for(k) + (rows - 1) * GAP + PAD_BOTTOM;
}

static void push(LoginUiKeypad *k, int kind, const char *text, double weight) {
    if (k->n_keys >= LOGINUI_KEYPAD_KEYS_MAX) return;
    LoginUiKey *key = &k->keys[k->n_keys++];
    memset(key, 0, sizeof *key);
    key->kind = kind;
    key->weight = weight;
    if (text) snprintf(key->text, sizeof key->text, "%s", text);
}

static void place_row(LoginUiKeypad *k, int first, double unit, double x, double w, double y, double h) {
    double total = 0;
    for (int i = first; i < k->n_keys; i++) total += k->keys[i].weight;
    double kx = x + (w - total * unit) / 2.0;
    for (int i = first; i < k->n_keys; i++) {
        LoginUiKey *key = &k->keys[i];
        double kw = key->weight * unit;
        key->x = kx + GAP / 2.0;
        key->y = y;
        key->w = kw - GAP;
        key->h = h;
        kx += kw;
    }
}

static int symbol_pages(const LoginUiKeypad *k) {
    int pages = (k->n_symbols + SYMBOLS_PER_PAGE - 1) / SYMBOLS_PER_PAGE;
    return pages < 1 ? 1 : pages;
}

static void layout(LoginUiKeypad *k, double x, double y, double w) {
    k->n_keys = 0;
    double inner_x = x + PAD_SIDE - GAP / 2.0;
    double inner_w = w - 2 * PAD_SIDE + GAP;
    double kh = key_h_for(k);
    double ry = y + PAD_TOP;
    int first;

    if (k->page == LOGINUI_KEYPAD_DIGITS) {
        double unit = inner_w / 3.0;
        static const char *digits = "123456789";
        for (int r = 0; r < 3; r++) {
            first = k->n_keys;
            for (int c = 0; c < 3; c++) {
                char t[2] = { digits[r * 3 + c], '\0' };
                push(k, KEY_CHAR, t, 1);
            }
            place_row(k, first, unit, inner_x, inner_w, ry, kh);
            ry += kh + GAP;
        }
        first = k->n_keys;
        push(k, KEY_LETTERS, "ABC", 1);
        push(k, KEY_CHAR, "0", 1);
        push(k, KEY_BACKSPACE, NULL, 1);
        place_row(k, first, unit, inner_x, inner_w, ry, kh);
        ry += kh + GAP;
        first = k->n_keys;
        push(k, KEY_ENTER, "Enter", 3);
        place_row(k, first, unit, inner_x, inner_w, ry, kh);
        return;
    }

    double unit = inner_w / 10.0;
    if (k->page == LOGINUI_KEYPAD_LETTERS) {
        bool upper = k->shift || k->caps;
        int li = 0;
        for (int r = 0; r < 3; r++) {
            first = k->n_keys;
            if (r == 2) push(k, KEY_SHIFT, "Shift", 1.5);
            for (int c = 0; c < letter_row_len[r]; c++, li++) {
                const char *t = k->letters[li][upper ? 1 : 0];
                if (t[0]) push(k, KEY_CHAR, t, 1);
            }
            if (r == 2) push(k, KEY_BACKSPACE, NULL, 1.5);
            place_row(k, first, unit, inner_x, inner_w, ry, kh);
            ry += kh + GAP;
        }
        first = k->n_keys;
        push(k, KEY_SYMBOLS, "?123", 1.5);
        push(k, KEY_SPACE, NULL, 7);
        push(k, KEY_ENTER, "Enter", 1.5);
        place_row(k, first, unit, inner_x, inner_w, ry, kh);
        return;
    }

    first = k->n_keys;
    for (int c = 1; c <= 10; c++) {
        char t[2] = { (char)('0' + c % 10), '\0' };
        push(k, KEY_CHAR, t, 1);
    }
    place_row(k, first, unit, inner_x, inner_w, ry, kh);
    ry += kh + GAP;

    int pages = symbol_pages(k);
    if (k->symbol_page >= pages) k->symbol_page = 0;
    int si = k->symbol_page * SYMBOLS_PER_PAGE;
    int end = si + SYMBOLS_PER_PAGE;
    if (end > k->n_symbols) end = k->n_symbols;

    first = k->n_keys;
    for (int c = 0; c < 10; c++, si++) {
        if (si < end) push(k, KEY_CHAR, k->symbols[si], 1);
        else push(k, KEY_SPACER, NULL, 1);
    }
    place_row(k, first, unit, inner_x, inner_w, ry, kh);
    ry += kh + GAP;

    first = k->n_keys;
    if (pages > 1) {
        char label[4] = { (char)('1' + k->symbol_page), '/', (char)('0' + pages), '\0' };
        push(k, KEY_MORE, label, 1.5);
    } else {
        push(k, KEY_SPACER, NULL, 1.5);
    }
    for (int c = 0; c < 7; c++, si++) {
        if (si < end) push(k, KEY_CHAR, k->symbols[si], 1);
        else push(k, KEY_SPACER, NULL, 1);
    }
    push(k, KEY_BACKSPACE, NULL, 1.5);
    place_row(k, first, unit, inner_x, inner_w, ry, kh);
    ry += kh + GAP;

    first = k->n_keys;
    push(k, KEY_LETTERS, "ABC", 1.5);
    push(k, KEY_SPACE, NULL, 7);
    push(k, KEY_ENTER, "Enter", 1.5);
    place_row(k, first, unit, inner_x, inner_w, ry, kh);
}

static void draw_backspace(cairo_t *cr, double cx, double cy, double r, double g, double b) {
    double w = 22, h = 16;
    double x = cx - w / 2.0 + 2, y = cy - h / 2.0;
    cairo_new_path(cr);
    cairo_move_to(cr, x, cy);
    cairo_line_to(cr, x + 6, y);
    cairo_line_to(cr, x + w, y);
    cairo_line_to(cr, x + w, y + h);
    cairo_line_to(cr, x + 6, y + h);
    cairo_close_path(cr);
    double mx = x + 6 + (w - 6) / 2.0;
    cairo_move_to(cr, mx - 3.5, cy - 3.5);
    cairo_line_to(cr, mx + 3.5, cy + 3.5);
    cairo_move_to(cr, mx + 3.5, cy - 3.5);
    cairo_line_to(cr, mx - 3.5, cy + 3.5);
    cairo_set_source_rgb(cr, r, g, b);
    cairo_set_line_width(cr, 1.6);
    cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
    cairo_stroke(cr);
}

void loginui_keypad_render(cairo_t *cr, LoginUiKeypad *k, double x, double y, double w) {
    layout(k, x, y, w);
    double h = loginui_keypad_height(k);

    lui_rounded_rect(cr, x, y, w, h + 18, 18);
    cairo_set_source_rgba(cr, 0.11, 0.11, 0.12, 0.97);
    cairo_fill(cr);
    lui_rounded_rect(cr, x + 0.5, y + 0.5, w - 1, h + 18, 18);
    cairo_set_source_rgba(cr, 1, 1, 1, 0.08);
    cairo_set_line_width(cr, 1);
    cairo_stroke(cr);

    for (int i = 0; i < k->n_keys; i++) {
        LoginUiKey *key = &k->keys[i];
        if (key->kind == KEY_SPACER) continue;
        bool special = key->kind != KEY_CHAR && key->kind != KEY_SPACE;
        bool lit = key->kind == KEY_SHIFT && (k->shift || k->caps);
        lui_rounded_rect(cr, key->x, key->y, key->w, key->h, 10);
        if (lit) cairo_set_source_rgb(cr, 0.92, 0.92, 0.94);
        else if (i == k->pressed) cairo_set_source_rgba(cr, 1, 1, 1, 0.30);
        else cairo_set_source_rgba(cr, 1, 1, 1, special ? 0.16 : 0.10);
        cairo_fill(cr);
        if (lit && k->caps) {
            cairo_rectangle(cr, key->x + key->w / 2.0 - 8, key->y + key->h - 9, 16, 2);
            cairo_set_source_rgb(cr, 0.10, 0.10, 0.11);
            cairo_fill(cr);
        }

        double fr = lit ? 0.10 : 0.96, fg = lit ? 0.10 : 0.96, fb = lit ? 0.11 : 0.98;
        double cx = key->x + key->w / 2.0, cy = key->y + key->h / 2.0;
        if (key->kind == KEY_BACKSPACE) {
            draw_backspace(cr, cx, cy, fr, fg, fb);
            continue;
        }
        if (!key->text[0]) continue;
        const char *font = special ? "Sans 13" : (k->page == LOGINUI_KEYPAD_DIGITS ? "Sans 22" : "Sans 18");
        int tw, th;
        lui_text_size(cr, font, key->text, &tw, &th);
        lui_draw_text(cr, font, key->text, cx, cy - th / 2.0, 1, fr, fg, fb);
    }
}

int loginui_keypad_key_at(const LoginUiKeypad *k, double x, double y) {
    for (int i = 0; i < k->n_keys; i++) {
        const LoginUiKey *key = &k->keys[i];
        if (key->kind == KEY_SPACER) continue;
        if (x >= key->x - GAP / 2.0 && x <= key->x + key->w + GAP / 2.0 &&
            y >= key->y - GAP / 2.0 && y <= key->y + key->h + GAP / 2.0)
            return i;
    }
    return -1;
}

LoginUiKeypadAction loginui_keypad_activate(LoginUiKeypad *k, int index, char *text, size_t size) {
    if (size) text[0] = '\0';
    if (index < 0 || index >= k->n_keys) return LOGINUI_KEYPAD_NONE;
    LoginUiKey *key = &k->keys[index];
    switch (key->kind) {
    case KEY_CHAR:
        snprintf(text, size, "%s", key->text);
        if (k->shift && !k->caps) k->shift = false;
        return LOGINUI_KEYPAD_TEXT;
    case KEY_SPACE:
        snprintf(text, size, " ");
        return LOGINUI_KEYPAD_TEXT;
    case KEY_BACKSPACE:
        return LOGINUI_KEYPAD_BACKSPACE;
    case KEY_ENTER:
        return LOGINUI_KEYPAD_ENTER;
    case KEY_SHIFT:
        if (k->caps) { k->caps = false; k->shift = false; }
        else if (k->shift) k->caps = true;
        else k->shift = true;
        return LOGINUI_KEYPAD_NONE;
    case KEY_LETTERS:
        loginui_keypad_set_page(k, LOGINUI_KEYPAD_LETTERS);
        return LOGINUI_KEYPAD_NONE;
    case KEY_SYMBOLS:
        loginui_keypad_set_page(k, LOGINUI_KEYPAD_SYMBOLS);
        return LOGINUI_KEYPAD_NONE;
    case KEY_MORE:
        k->symbol_page = (k->symbol_page + 1) % symbol_pages(k);
        return LOGINUI_KEYPAD_NONE;
    default:
        return LOGINUI_KEYPAD_NONE;
    }
}
