#include "loginui.h"
#include "internal.h"

#include <math.h>
#include <string.h>

#define PATTERN_SIDE_MAX 288.0
#define PATTERN_MARGIN 40.0
#define PATTERN_LINK_GAP 18.0
#define PATTERN_LINK_H 36.0

void loginui_pattern_reset(LoginUiPattern *p) {
    p->n = 0;
    p->tracking = false;
    p->error = false;
}

static double side_for(double width) {
    double side = width - 2 * PATTERN_MARGIN;
    if (side > PATTERN_SIDE_MAX) side = PATTERN_SIDE_MAX;
    if (side < 120) side = 120;
    return side;
}

double loginui_pattern_height(double width) {
    return side_for(width) + PATTERN_LINK_GAP + PATTERN_LINK_H + 12;
}

static bool visited(const LoginUiPattern *p, int node) {
    for (int i = 0; i < p->n; i++)
        if (p->path[i] == node) return true;
    return false;
}

static void add(LoginUiPattern *p, int node) {
    if (p->n >= LOGINUI_PATTERN_MAX || visited(p, node)) return;
    if (p->n > 0) {
        int last = p->path[p->n - 1];
        int r1 = last / 3, c1 = last % 3, r2 = node / 3, c2 = node % 3;
        if ((r1 + r2) % 2 == 0 && (c1 + c2) % 2 == 0) {
            int mid = ((r1 + r2) / 2) * 3 + (c1 + c2) / 2;
            if (mid != last && mid != node && !visited(p, mid)) p->path[p->n++] = mid;
        }
    }
    if (p->n < LOGINUI_PATTERN_MAX) p->path[p->n++] = node;
}

static int node_at(const LoginUiPattern *p, double x, double y) {
    if (!p->laid_out) return -1;
    double hit = p->radius * 2.2;
    for (int i = 0; i < 9; i++) {
        double dx = x - p->cx[i], dy = y - p->cy[i];
        if (dx * dx + dy * dy <= hit * hit) return i;
    }
    return -1;
}

bool loginui_pattern_press(LoginUiPattern *p, double x, double y) {
    int node = node_at(p, x, y);
    if (node < 0) return false;
    p->n = 0;
    p->error = false;
    p->tracking = true;
    p->finger_x = x;
    p->finger_y = y;
    add(p, node);
    return true;
}

bool loginui_pattern_motion(LoginUiPattern *p, double x, double y) {
    if (!p->tracking) return false;
    p->finger_x = x;
    p->finger_y = y;
    int node = node_at(p, x, y);
    if (node >= 0) add(p, node);
    return true;
}

size_t loginui_pattern_release(LoginUiPattern *p, char *out, size_t size) {
    if (!p->tracking) return 0;
    p->tracking = false;
    size_t n = 0;
    for (int i = 0; i < p->n && n + 1 < size; i++) out[n++] = (char)('1' + p->path[i]);
    if (size > 0) out[n] = '\0';
    return n;
}

bool loginui_pattern_link_at(const LoginUiPattern *p, double x, double y) {
    return p->laid_out && p->link_w > 0 && x >= p->link_x && x <= p->link_x + p->link_w
        && y >= p->link_y && y <= p->link_y + p->link_h;
}

void loginui_pattern_render(cairo_t *cr, LoginUiPattern *p, const char *fallback,
                            double x, double y, double w) {
    double side = side_for(w);
    double cell = side / 3.0;
    double left = x + (w - side) / 2.0;
    p->radius = cell * 0.13;
    for (int i = 0; i < 9; i++) {
        p->cx[i] = left + cell * (i % 3) + cell / 2.0;
        p->cy[i] = y + cell * (i / 3) + cell / 2.0;
    }
    p->laid_out = true;

    double lr = 1.0, lg = 1.0, lb = 1.0;
    if (p->error) { lr = 0.97; lg = 0.42; lb = 0.40; }

    if (p->n > 0) {
        cairo_save(cr);
        cairo_set_line_width(cr, 5);
        cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
        cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
        cairo_move_to(cr, p->cx[p->path[0]], p->cy[p->path[0]]);
        for (int i = 1; i < p->n; i++) cairo_line_to(cr, p->cx[p->path[i]], p->cy[p->path[i]]);
        if (p->tracking) cairo_line_to(cr, p->finger_x, p->finger_y);
        cairo_set_source_rgba(cr, lr, lg, lb, 0.55);
        cairo_stroke(cr);
        cairo_restore(cr);
    }

    for (int i = 0; i < 9; i++) {
        bool on = visited(p, i);
        cairo_arc(cr, p->cx[i], p->cy[i], p->radius * 2.0, 0, 2 * M_PI);
        cairo_set_source_rgba(cr, lr, lg, lb, on ? 0.22 : 0.08);
        cairo_fill(cr);
        cairo_arc(cr, p->cx[i], p->cy[i], on ? p->radius : p->radius * 0.62, 0, 2 * M_PI);
        cairo_set_source_rgba(cr, lr, lg, lb, on ? 1.0 : 0.85);
        cairo_fill(cr);
    }

    p->link_w = 0;
    if (fallback && fallback[0]) {
        int tw, th;
        lui_text_size(cr, "Sans 13", fallback, &tw, &th);
        double ly = y + side + PATTERN_LINK_GAP;
        p->link_w = tw + 32;
        p->link_h = PATTERN_LINK_H;
        p->link_x = x + (w - p->link_w) / 2.0;
        p->link_y = ly;
        lui_rounded_rect(cr, p->link_x, p->link_y, p->link_w, p->link_h, PATTERN_LINK_H / 2.0);
        cairo_set_source_rgba(cr, 1, 1, 1, 0.12);
        cairo_fill(cr);
        lui_draw_text(cr, "Sans 13", fallback, x + w / 2.0, ly + (PATTERN_LINK_H - th) / 2.0, 1, 0.96, 0.96, 0.98);
    }
}
