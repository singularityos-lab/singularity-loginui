#include "widget.h"
#include "loginui.h"
#include <stddef.h>

/* next/prev ACTION index, wrapping and skipping labels. Returns `from` if no other
   action exists. */
static int next_action(const LoginUiMenu *m, int from, int dir) {
    if (m->count == 0) return -1;
    int i = from;
    for (int step = 0; step < m->count; step++) {
        i += dir;
        if (i < 0) i = m->count - 1;
        if (i >= m->count) i = 0;
        if (m->items[i].type == LOGINUI_ITEM_ACTION) return i;
    }
    return from;
}

void loginui_menu_init(LoginUiMenu *m, LoginUiItem *items, int count, const char *font) {
    m->items = items;
    m->count = count;
    m->font = font ? font : "Sans 18";
    m->fr = 0.20; m->fg = 0.45; m->fb = 0.95;  /* accent highlight (overridable) */
    m->tr = 0.93; m->tg = 0.94; m->tb = 0.96;  /* near-white text */
    m->focus = -1;
    for (int i = 0; i < count; i++) {
        if (items[i].type == LOGINUI_ITEM_ACTION) { m->focus = i; break; }
    }
}

double loginui_menu_render(cairo_t *cr, LoginUiMenu *m, double x, double y, double w) {
    const double pad_x = 18.0, pad_y = 10.0, gap = 6.0, radius = 10.0;
    double cy = y;
    for (int i = 0; i < m->count; i++) {
        LoginUiItem *it = &m->items[i];
        const char *txt = it->text ? it->text : "";
        int tw = 0, th = 0;
        loginui_text_size(cr, m->font, txt, &tw, &th);
        double rowH = (double)th + 2 * pad_y;
        it->x = x; it->y = cy; it->w = w; it->h = rowH;

        if (it->type == LOGINUI_ITEM_ACTION && i == m->focus) {
            loginui_rounded_rect(cr, x, cy, w, rowH, radius);
            cairo_set_source_rgba(cr, m->fr, m->fg, m->fb, 0.85);
            cairo_fill(cr);
        }
        double tr = m->tr, tg = m->tg, tb = m->tb;
        if (it->type == LOGINUI_ITEM_LABEL) { tr *= 0.6; tg *= 0.6; tb *= 0.6; } /* dim headings */
        loginui_text(cr, m->font, txt, x + pad_x, cy + pad_y, 0, tr, tg, tb);
        cy += rowH + gap;
    }
    return cy - y;
}

int loginui_menu_key(LoginUiMenu *m, int key) {
    if (m->focus < 0) return -1;
    switch (key) {
        case LOGINUI_KEY_UP:    m->focus = next_action(m, m->focus, -1); return -1;
        case LOGINUI_KEY_DOWN:  m->focus = next_action(m, m->focus, +1); return -1;
        case LOGINUI_KEY_ENTER: return m->focus;
        default:                return -1;
    }
}
