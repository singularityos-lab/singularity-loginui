#include "loginui.h"
#include "internal.h"

#include <math.h>

bool loginui_compact(int width, int height) {
    return height > width || width < 640;
}

typedef struct {
    double pad, avatar, field_h, status_h, w, h;
} CardMetrics;

static void measure_card(cairo_t *cr, const LoginUiState *st, double card_w, CardMetrics *m) {
    m->w = card_w;
    m->pad = 14;
    m->avatar = 46;
    m->field_h = 56;
    m->h = m->pad + m->avatar + 14 + m->field_h + m->pad;
    m->status_h = 0;
    if (st->status_text && st->status_text[0]) {
        m->status_h = lui_wrapped_height(cr, "Sans 12", st->status_text, card_w - 2 * m->pad - 20) + 14;
        if (m->status_h < 30) m->status_h = 30;
        m->h += m->status_h + 8;
    }
}

static void draw_card(cairo_t *cr, const LoginUiState *st, const CardMetrics *m, double card_x, double card_y) {
    double card_w = m->w, card_h = m->h, card_pad = m->pad, avatar = m->avatar, field_h = m->field_h;
    const char *uname = (st->username && st->username[0]) ? st->username : "user";

    lui_rounded_rect(cr, card_x, card_y, card_w, card_h, 24);
    cairo_set_source_rgba(cr, 0.176, 0.176, 0.176, 0.97);
    cairo_fill(cr);
    lui_rounded_rect(cr, card_x + 0.5, card_y + 0.5, card_w - 1, card_h - 1, 24);
    cairo_set_source_rgba(cr, 1, 1, 1, 0.08);
    cairo_set_line_width(cr, 1);
    cairo_stroke(cr);

    double row_cy = card_y + card_pad + avatar / 2.0;
    double name_x = card_x + card_pad;
    if (st->avatar) {
        double ax = card_x + card_pad + avatar / 2.0;
        cairo_save(cr);
        cairo_arc(cr, ax, row_cy, avatar / 2.0, 0, 2 * 3.14159265);
        cairo_clip(cr);
        int aw = cairo_image_surface_get_width(st->avatar);
        int ah = cairo_image_surface_get_height(st->avatar);
        double sc = avatar / (double)(aw < ah ? aw : ah);
        cairo_translate(cr, ax - avatar / 2.0 + (avatar - aw * sc) / 2.0,
                            row_cy - avatar / 2.0 + (avatar - ah * sc) / 2.0);
        cairo_scale(cr, sc, sc);
        cairo_set_source_surface(cr, st->avatar, 0, 0);
        cairo_paint(cr);
        cairo_restore(cr);
        name_x = card_x + card_pad + avatar + 12;
    }
    {
        int nw, nh;
        lui_text_size(cr, "Sans Bold 18", uname, &nw, &nh);
        lui_draw_text(cr, "Sans Bold 18", uname, name_x, row_cy - nh / 2.0, 0, 0.96, 0.96, 0.98);
    }

    double fy = card_y + card_pad + avatar + 14;
    double fx = card_x + card_pad;
    double fw = card_w - 2 * card_pad;
    lui_rounded_rect(cr, fx, fy, fw, field_h, 14);
    cairo_set_source_rgba(cr, 1, 1, 1, 0.08);
    cairo_fill(cr);
    const char *auth_label = st->auth_label ? st->auth_label : "Password";
    lui_draw_text(cr, "Sans Bold 12", auth_label, fx + 14, fy + 8, 0, 0.96, 0.96, 0.98);
    double vcy = fy + 40;
    if (st->password_dots <= 0) {
        int pw, ph;
        lui_text_size(cr, "Sans 12", auth_label, &pw, &ph);
        lui_draw_text(cr, "Sans 12", auth_label, fx + 14, vcy - ph / 2.0, 0, 0.6, 0.6, 0.62);
    } else {
        int dots = st->password_dots;
        int max_dots = (int)((fw - 28) / 16) + 1;
        if (dots > max_dots) dots = max_dots;
        cairo_set_source_rgb(cr, 0.9, 0.9, 0.92);
        for (int i = 0; i < dots; i++) {
            cairo_arc(cr, fx + 18 + i * 16, vcy, 4, 0, 2 * 3.14159265);
            cairo_fill(cr);
        }
    }

    if (st->status_text && st->status_text[0]) {
        double sy = fy + field_h + 8;
        double sh = m->status_h;
        lui_rounded_rect(cr, fx, sy, fw, sh, 9);
        if (st->status_error) cairo_set_source_rgba(cr, 0.90, 0.30, 0.28, 0.16);
        else cairo_set_source_rgba(cr, 1, 1, 1, 0.07);
        cairo_fill(cr);
        int sht = lui_wrapped_height(cr, "Sans 12", st->status_text, fw - 20);
        if (st->status_error)
            lui_draw_wrapped(cr, "Sans 12", st->status_text, fx + 10, sy + (sh - sht) / 2.0, fw - 20, 0.97, 0.52, 0.48);
        else
            lui_draw_wrapped(cr, "Sans 12", st->status_text, fx + 10, sy + (sh - sht) / 2.0, fw - 20, 0.82, 0.82, 0.85);
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
    lui_text_size(cr, "Sans 17", dbuf, &dw, &dh);
    double top = h * 0.11;
    if (top < 56) top = 56;
    double clock_y = top + st->clock_dy;
    lui_draw_text(cr, "Sans Bold 64", tbuf, w / 2.0, clock_y, 1, 1, 1, 1);
    lui_draw_text(cr, "Sans 17", dbuf, w / 2.0, clock_y + th + 4, 1, 0.85, 0.85, 0.88);
    double clock_bottom = top + th + 4 + dh;

    double card_w = w - 32;
    if (card_w > 380) card_w = 380;
    CardMetrics m;
    measure_card(cr, st, card_w, &m);

    double kp_h = st->keypad ? loginui_keypad_height(st->keypad) : 0;
    double card_x = (w - card_w) / 2.0;
    double card_y = st->keypad ? h - kp_h - 20 - m.h : h * 0.55 - m.h / 2.0;
    if (card_y < clock_bottom + 24) card_y = clock_bottom + 24;

    if (card_x_out) *card_x_out = card_x;
    if (card_y_out) *card_y_out = card_y;
    if (card_w_out) *card_w_out = card_w;
    if (card_h_out) *card_h_out = m.h;

    if (cover < 1.0) {
        cairo_save(cr);
        cairo_translate(cr, st->card_dx, cover * 48);
        cairo_rectangle(cr, card_x - 2, card_y - 2, card_w + 4, m.h + 4);
        cairo_clip(cr);
        cairo_push_group(cr);
        draw_card(cr, st, &m, card_x, card_y);
        cairo_pop_group_to_source(cr);
        cairo_paint_with_alpha(cr, 1.0 - cover);
        cairo_restore(cr);
    }
    if (st->keypad && cover < 1.0)
        loginui_keypad_render(cr, st->keypad, 0, h - kp_h + cover * (kp_h + 20), w);
    else if (st->keypad)
        st->keypad->n_keys = 0;
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
        lui_text_size(cr, "Sans 18", dbuf, &dw, &dh);
        int clock_w = tw > dw ? tw : dw;
        double gap = 40;
        CardMetrics m;
        measure_card(cr, st, 300, &m);

        double group_w = clock_w + gap + m.w;
        double group_x = (w - group_w) / 2.0;
        double clock_right = group_x + clock_w;
        double card_x = group_x + clock_w + gap;
        double card_y = (h - m.h) / 2.0;

        if (card_x_out) *card_x_out = card_x;
        if (card_y_out) *card_y_out = card_y;
        if (card_w_out) *card_w_out = m.w;
        if (card_h_out) *card_h_out = m.h;

        double clock_block_h = th + 6 + dh;
        double clock_y = (h - clock_block_h) / 2.0 + st->clock_dy;
        lui_draw_text(cr, "Sans Bold 60", tbuf, clock_right, clock_y, 2, 1, 1, 1);
        lui_draw_text(cr, "Sans 18", dbuf, clock_right, clock_y + th + 6, 2, 0.85, 0.85, 0.88);

        cairo_save(cr);
        cairo_translate(cr, st->card_dx, 0);
        draw_card(cr, st, &m, card_x, card_y);
        cairo_restore(cr);
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
