#include "loginui.h"
#include "internal.h"

#include <math.h>

static void paint_cover(cairo_t *cr, cairo_surface_t *surface, double w, double h, double alpha) {
    int bw = cairo_image_surface_get_width(surface);
    int bh = cairo_image_surface_get_height(surface);
    if (bw < 1 || bh < 1) return;
    double scale = w / bw; if (h / bh > scale) scale = h / bh;
    cairo_save(cr);
    cairo_translate(cr, (w - bw * scale) / 2.0, (h - bh * scale) / 2.0);
    cairo_scale(cr, scale, scale);
    cairo_set_source_surface(cr, surface, 0, 0);
    if (alpha >= 1.0) cairo_paint(cr);
    else cairo_paint_with_alpha(cr, alpha);
    cairo_restore(cr);
}

void loginui_render(cairo_t *cr, const LoginUiState *st,
                    double *card_x_out, double *card_y_out,
                    double *card_w_out, double *card_h_out) {
    double w = st->width, h = st->height;

    if (st->background) {
        paint_cover(cr, st->background, w, h, 1.0);
    } else {
        cairo_set_source_rgb(cr, 0.10, 0.10, 0.11);
        cairo_paint(cr);
    }
    if (st->background_sharp && st->sharpness > 0.0)
        paint_cover(cr, st->background_sharp, w, h, st->sharpness > 1.0 ? 1.0 : st->sharpness);

    bool faded = st->content_fade > 0.0;
    if (faded) {
        if (st->content_fade >= 1.0) return;
        cairo_push_group(cr);
    }

    const char *tbuf = st->time_str ? st->time_str : "";
    const char *dbuf = st->date_str ? st->date_str : "";
    const char *uname = (st->username && st->username[0]) ? st->username : "user";

    int tw, th, dw, dh;
    lui_text_size(cr, "Sans Bold 60", tbuf, &tw, &th);
    lui_text_size(cr, "Sans 18", dbuf, &dw, &dh);
    int clock_w = tw > dw ? tw : dw;
    double gap = 40;
    double card_w = 300, card_pad = 14;
    double avatar = 46, field_h = 56;
    double card_h = card_pad + avatar + 14 + field_h + card_pad;
    double status_h = 0;
    if (st->status_text && st->status_text[0]) {
        status_h = lui_wrapped_height(cr, "Sans 12", st->status_text, card_w - 2 * card_pad - 20) + 14;
        if (status_h < 30) status_h = 30;
        card_h += status_h + 8;
    }

    double group_w = clock_w + gap + card_w;
    double group_x = (w - group_w) / 2.0;
    double clock_right = group_x + clock_w;
    double card_x = group_x + clock_w + gap;
    double card_y = (h - card_h) / 2.0;

    if (card_x_out) *card_x_out = card_x;
    if (card_y_out) *card_y_out = card_y;
    if (card_w_out) *card_w_out = card_w;
    if (card_h_out) *card_h_out = card_h;

    double clock_block_h = th + 6 + dh;
    double clock_y = (h - clock_block_h) / 2.0 + st->clock_dy;
    lui_draw_text(cr, "Sans Bold 60", tbuf, clock_right, clock_y, 2, 1, 1, 1);
    lui_draw_text(cr, "Sans 18", dbuf, clock_right, clock_y + th + 6, 2, 0.85, 0.85, 0.88);

    cairo_save(cr);
    cairo_translate(cr, st->card_dx, 0);

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
        int dots = st->password_dots; if (dots > 20) dots = 20;
        cairo_set_source_rgb(cr, 0.9, 0.9, 0.92);
        for (int i = 0; i < dots; i++) {
            cairo_arc(cr, fx + 18 + i * 16, vcy, 4, 0, 2 * 3.14159265);
            cairo_fill(cr);
        }
    }

    if (st->status_text && st->status_text[0]) {
        double sy = fy + field_h + 8;
        double sh = status_h;
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
    cairo_restore(cr);

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
