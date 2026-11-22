#include "loginui.h"
#include "internal.h"

#include <math.h>

bool loginui_compact(int width, int height) {
    return height > width || width < 640;
}

#define CARD_RADIUS 20.0
#define CARD_GAP 16.0
#define CARD_W 360.0
#define STATUS_GAP 10.0
#define FIELD_RADIUS 8.0
#define FIELD_INSET 12.0
#define FIELD_H 44.0
#define HINT_H 22.0
#define DOT_R 4.0
#define DOT_STEP 14.0
#define CARET_W 2.0
#define CARET_H 20.0
#define SCRIM 0.55
#define SHADOW_STEPS 8
#define SHADOW_SPREAD 24.0
#define SHADOW_DROP 8.0
#define SHADOW_ROOM (SHADOW_SPREAD + SHADOW_DROP)
#define NAME_FONT "Sans Semi-Bold 13.5"

typedef struct {
    double pad, avatar, field_h, status_h, w, h;
} CardMetrics;

static void measure_card(cairo_t *cr, const LoginUiState *st, double card_w, CardMetrics *m) {
    m->w = card_w;
    m->pad = 24;
    m->avatar = 46;
    m->field_h = st->pattern ? HINT_H : FIELD_H;
    m->h = m->pad + m->avatar + CARD_GAP + m->field_h + m->pad;
    m->status_h = 0;
    if (st->status_text && st->status_text[0]) {
        m->status_h = lui_wrapped_height(cr, "Sans 10", st->status_text, card_w - 2 * m->pad);
        m->h += m->status_h + STATUS_GAP;
    }
}

static void draw_avatar(cairo_t *cr, cairo_surface_t *img, double x, double y, double size) {
    int iw = cairo_image_surface_get_width(img);
    int ih = cairo_image_surface_get_height(img);
    double sc = size / (double)(iw < ih ? iw : ih);
    cairo_save(cr);
    cairo_arc(cr, x + size / 2.0, y + size / 2.0, size / 2.0, 0, 2 * M_PI);
    cairo_clip(cr);
    cairo_translate(cr, x + (size - iw * sc) / 2.0, y + (size - ih * sc) / 2.0);
    cairo_scale(cr, sc, sc);
    cairo_set_source_surface(cr, img, 0, 0);
    cairo_paint(cr);
    cairo_restore(cr);
}

static const char *field_hint(const LoginUiState *st) {
    if (st->auth_hint) return st->auth_hint;
    return st->auth_label ? st->auth_label : "Password";
}

static void draw_field(cairo_t *cr, const LoginUiState *st, double x, double y, double w, double h) {
    const LoginUiStyle *s = loginui_style();
    lui_rounded_rect(cr, x, y, w, h, FIELD_RADIUS);
    loginui_set_color(cr, s->field);
    cairo_fill(cr);
    lui_rounded_rect(cr, x + 1, y + 1, w - 2, h - 2, FIELD_RADIUS - 1);
    loginui_set_color(cr, st->status_error ? s->error : s->focus);
    cairo_set_line_width(cr, 2);
    cairo_stroke(cr);

    int dots = st->password_dots > 0 ? st->password_dots : 0;
    double left = x + FIELD_INSET, room = w - 2 * FIELD_INSET;
    double used = dots * DOT_STEP + CARET_W;
    double start = used > room ? left + room - used : left;
    double middle = y + h / 2.0;

    cairo_save(cr);
    cairo_rectangle(cr, left, y, room, h);
    cairo_clip(cr);
    if (!dots)
        lui_draw_fit(cr, "Sans 11", field_hint(st), left + CARET_W + 4, middle, room - CARET_W - 4, s->dim);
    loginui_set_color(cr, s->text);
    for (int i = 0; i < dots; i++) {
        double dx = start + i * DOT_STEP + DOT_R;
        if (dx + DOT_R < left) continue;
        cairo_new_sub_path(cr);
        cairo_arc(cr, dx, middle, DOT_R, 0, 2 * M_PI);
    }
    cairo_fill(cr);
    if (lui_caret_show(dots)) {
        cairo_rectangle(cr, start + dots * DOT_STEP, middle - CARET_H / 2.0, CARET_W, CARET_H);
        loginui_set_color(cr, s->accent);
        cairo_fill(cr);
    }
    cairo_restore(cr);
}

static void draw_shadow(cairo_t *cr, double x, double y, double w, double h) {
    for (int i = SHADOW_STEPS; i > 0; i--) {
        double spread = SHADOW_SPREAD * i / SHADOW_STEPS;
        lui_rounded_rect(cr, x - spread, y - spread + SHADOW_DROP, w + 2 * spread, h + 2 * spread, CARD_RADIUS + spread);
        cairo_set_source_rgba(cr, 0, 0, 0, 0.55 / SHADOW_STEPS / 2);
        cairo_fill(cr);
    }
}

static void draw_card(cairo_t *cr, const LoginUiState *st, const CardMetrics *m, double x, double y) {
    const LoginUiStyle *s = loginui_style();
    const char *uname = (st->username && st->username[0]) ? st->username : "user";
    double inner_x = x + m->pad, inner_w = m->w - 2 * m->pad;

    draw_shadow(cr, x, y, m->w, m->h);
    loginui_surface(cr, x, y, m->w, m->h, CARD_RADIUS);

    double name_x = inner_x;
    if (st->avatar) {
        draw_avatar(cr, st->avatar, inner_x, y + m->pad, m->avatar);
        name_x += m->avatar + 12;
    }
    lui_draw_fit(cr, NAME_FONT, uname, name_x, y + m->pad + m->avatar / 2.0, inner_x + inner_w - name_x, s->text);

    double fy = y + m->pad + m->avatar + CARD_GAP;
    if (st->pattern)
        lui_draw_fit(cr, "Sans 11", field_hint(st), inner_x, fy + m->field_h / 2.0, inner_w, s->dim);
    else
        draw_field(cr, st, inner_x, fy, inner_w, m->field_h);

    if (m->status_h > 0) {
        LoginUiColor c = st->status_error ? s->error : s->dim;
        lui_draw_wrapped(cr, "Sans 10", st->status_text, inner_x, fy + m->field_h + STATUS_GAP, inner_w, c);
    }
}

static void draw_swipe_hint(cairo_t *cr, double w, double h, double alpha) {
    if (alpha <= 0.0) return;
    double cx = w / 2.0, cy = h - 78;
    cairo_save(cr);
    cairo_rectangle(cr, 0, cy - 12, w, 60);
    cairo_clip(cr);
    cairo_push_group(cr);
    cairo_move_to(cr, cx - 12, cy + 5);
    cairo_line_to(cr, cx, cy - 5);
    cairo_line_to(cr, cx + 12, cy + 5);
    cairo_set_source_rgba(cr, 1, 1, 1, 0.85);
    cairo_set_line_width(cr, 2.4);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
    cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
    cairo_stroke(cr);
    lui_draw_text(cr, "Sans 13", "Swipe up to unlock", cx, cy + 16, 1, 0.92, 0.92, 0.94);
    cairo_pop_group_to_source(cr);
    cairo_paint_with_alpha(cr, alpha > 1.0 ? 1.0 : alpha);
    cairo_restore(cr);
}

static void render_compact(cairo_t *cr, const LoginUiState *st,
                           double *card_x_out, double *card_y_out,
                           double *card_w_out, double *card_h_out) {
    double w = st->width, h = st->height;
    double cover = st->cover < 0.0 ? 0.0 : st->cover > 1.0 ? 1.0 : st->cover;
    const char *tbuf = st->time_str ? st->time_str : "";
    const char *dbuf = st->date_str ? st->date_str : "";

    int tw, th, dw, dh;
    lui_text_size(cr, "Sans Bold 64", tbuf, &tw, &th);
    lui_text_size(cr, "Sans Semi-Bold 17", dbuf, &dw, &dh);
    double top = h * 0.11;
    if (top < 56) top = 56;
    double clock_y = top + st->clock_dy;
    lui_draw_text(cr, "Sans Bold 64", tbuf, w / 2.0, clock_y, 1, 1, 1, 1);
    lui_draw_text(cr, "Sans Semi-Bold 17", dbuf, w / 2.0, clock_y + th + 4, 1, 1, 1, 1);
    double clock_bottom = top + th + 4 + dh;

    double card_w = w - 32;
    if (card_w > 380) card_w = 380;
    CardMetrics m;
    measure_card(cr, st, card_w, &m);

    double kp_h = st->pattern ? loginui_pattern_height(w)
        : st->keypad ? loginui_keypad_height(st->keypad) : 0;
    double card_x = (w - card_w) / 2.0;
    double card_y = kp_h > 0 ? h - kp_h - 20 - m.h : h * 0.55 - m.h / 2.0;
    if (card_y < clock_bottom + 24) card_y = clock_bottom + 24;

    if (card_x_out) *card_x_out = card_x;
    if (card_y_out) *card_y_out = card_y;
    if (card_w_out) *card_w_out = card_w;
    if (card_h_out) *card_h_out = m.h;

    if (cover < 1.0) {
        cairo_save(cr);
        cairo_translate(cr, st->card_dx, cover * 48);
        cairo_rectangle(cr, card_x - SHADOW_ROOM, card_y - SHADOW_ROOM, card_w + 2 * SHADOW_ROOM, m.h + 2 * SHADOW_ROOM);
        cairo_clip(cr);
        cairo_push_group(cr);
        draw_card(cr, st, &m, card_x, card_y);
        cairo_pop_group_to_source(cr);
        cairo_paint_with_alpha(cr, 1.0 - cover);
        cairo_restore(cr);
    }
    if (st->pattern && cover < 1.0) {
        cairo_save(cr);
        cairo_push_group(cr);
        loginui_pattern_render(cr, st->pattern, st->pattern_fallback, 0, h - kp_h + cover * (kp_h + 20), w);
        cairo_pop_group_to_source(cr);
        cairo_paint_with_alpha(cr, 1.0 - cover);
        cairo_restore(cr);
    } else if (st->pattern) {
        st->pattern->laid_out = false;
    } else if (st->keypad && cover < 1.0) {
        loginui_keypad_render(cr, st->keypad, 0, h - kp_h + cover * (kp_h + 20), w);
    } else if (st->keypad) {
        st->keypad->n_keys = 0;
    }
    draw_swipe_hint(cr, w, h, cover);
}

void loginui_render(cairo_t *cr, const LoginUiState *st,
                    double *card_x_out, double *card_y_out,
                    double *card_w_out, double *card_h_out) {
    double w = st->width, h = st->height;

    if (st->background) {
        lui_paint_cover(cr, st->background, w, h, 1.0);
    } else {
        cairo_set_source_rgb(cr, 0.10, 0.10, 0.11);
        cairo_paint(cr);
    }
    if (st->background_sharp && st->sharpness > 0.0)
        lui_paint_cover(cr, st->background_sharp, w, h, st->sharpness > 1.0 ? 1.0 : st->sharpness);

    lui_caret_hide();
    double veil = SCRIM * (1.0 - CLAMP(st->sharpness, 0.0, 1.0)) * (1.0 - CLAMP(st->content_fade, 0.0, 1.0));
    if (loginui_compact(st->width, st->height)) veil *= 1.0 - CLAMP(st->cover, 0.0, 1.0);
    if (veil > 0.0) {
        cairo_set_source_rgba(cr, 0, 0, 0, veil);
        cairo_paint(cr);
    }

    bool faded = st->content_fade > 0.0;
    if (faded) {
        if (st->content_fade >= 1.0) return;
        cairo_push_group(cr);
    }

    if (loginui_compact(st->width, st->height)) {
        render_compact(cr, st, card_x_out, card_y_out, card_w_out, card_h_out);
    } else {
        const char *tbuf = st->time_str ? st->time_str : "";
        const char *dbuf = st->date_str ? st->date_str : "";
        int tw, th, dw, dh;
        lui_text_size(cr, "Sans Bold 60", tbuf, &tw, &th);
        lui_text_size(cr, "Sans Semi-Bold 18", dbuf, &dw, &dh);
        int clock_w = tw > dw ? tw : dw;
        double gap = 40;
        CardMetrics m;
        measure_card(cr, st, CARD_W, &m);

        double group_w = clock_w + gap + m.w;
        double group_x = (w - group_w) / 2.0;
        double clock_right = group_x + clock_w;
        double card_x = group_x + clock_w + gap;
        double pattern_h = st->pattern ? loginui_pattern_height(m.w) + 16 : 0;
        double grown = st->pattern && m.status_h > 0 ? m.status_h + STATUS_GAP : 0;
        double card_y = (h - (m.h - grown) - pattern_h) / 2.0 - grown;

        if (card_x_out) *card_x_out = card_x;
        if (card_y_out) *card_y_out = card_y;
        if (card_w_out) *card_w_out = m.w;
        if (card_h_out) *card_h_out = m.h + pattern_h;

        double clock_block_h = th + 6 + dh;
        double clock_y = (h - clock_block_h) / 2.0 + st->clock_dy;
        lui_draw_text(cr, "Sans Bold 60", tbuf, clock_right, clock_y, 2, 1, 1, 1);
        lui_draw_text(cr, "Sans Semi-Bold 18", dbuf, clock_right, clock_y + th + 6, 2, 1, 1, 1);

        cairo_save(cr);
        cairo_translate(cr, st->card_dx, 0);
        draw_card(cr, st, &m, card_x, card_y);
        cairo_restore(cr);
        if (st->pattern)
            loginui_pattern_render(cr, st->pattern, st->pattern_fallback, card_x, card_y + m.h + 16, m.w);
    }

    if (faded) {
        cairo_pop_group_to_source(cr);
        cairo_paint_with_alpha(cr, 1.0 - st->content_fade);
    }
}

void loginui_render_splash(cairo_t *cr, int width, int height,
                           cairo_surface_t *bg, cairo_surface_t *logo,
                           double phase) {
    double w = width, h = height;
    if (bg) {
        loginui_paint_background(cr, bg, width, height);
    } else {
        cairo_set_source_rgb(cr, 0, 0, 0);
        cairo_paint(cr);
    }

    double cx = w / 2.0;

    if (logo) {
        int lw = cairo_image_surface_get_width(logo);
        int lh = cairo_image_surface_get_height(logo);
        double target = h * 0.16;
        double scale = lh > 0 ? target / lh : 1.0;
        double dw = lw * scale, dh = lh * scale;
        cairo_save(cr);
        cairo_translate(cr, cx - dw / 2.0, h * 0.46 - dh / 2.0);
        cairo_scale(cr, scale, scale);
        cairo_set_source_surface(cr, logo, 0, 0);
        cairo_paint(cr);
        cairo_restore(cr);
    }

    double track_w = 220, track_h = 4;
    double track_x = cx - track_w / 2.0;
    double track_y = h * 0.62;
    double r = track_h / 2.0;

    lui_rounded_rect(cr, track_x, track_y, track_w, track_h, r);
    cairo_set_source_rgba(cr, 1, 1, 1, 0.12);
    cairo_fill(cr);

    double seg_w = track_w * 0.30;
    double t = 0.5 + 0.5 * sin(phase * 1.6);
    double seg_x = track_x + t * (track_w - seg_w);
    lui_rounded_rect(cr, seg_x, track_y, seg_w, track_h, r);
    cairo_set_source_rgba(cr, 1, 1, 1, 0.85);
    cairo_fill(cr);
}
