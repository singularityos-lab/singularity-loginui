#include "loginui.h"
#include <assert.h>
#include <cairo/cairo.h>
#include <xkbcommon/xkbcommon.h>

static void settle(LoginUiGesture *g, double now) {
    while (loginui_gesture_tick(g, now)) now += 1.0;
}

int main(void) {
    struct xkb_context *ctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    struct xkb_rule_names names = { .layout = "us" };
    struct xkb_keymap *us = xkb_keymap_new_from_names(ctx, &names, XKB_KEYMAP_COMPILE_NO_FLAGS);
    assert(us);
    cairo_surface_t *s = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 480, 400);
    cairo_t *cr = cairo_create(s);
    LoginUiKeypad kp;
    loginui_keypad_init(&kp, us, LOGINUI_KEYPAD_DIGITS);
    loginui_keypad_render(cr, &kp, 0, 0, 480);
    assert(kp.n_keys > 0);
    const LoginUiKey *k0 = &kp.keys[0];
    double kx = k0->x + k0->w / 2, ky = k0->y + k0->h / 2;
    double fy = loginui_keypad_height(&kp) + 40;

    LoginUiGesture g;
    loginui_gesture_init(&g);
    assert(g.cover == 1.0 && g.touch_id == -1);
    assert(loginui_gesture_covered(&g, true));
    assert(!loginui_gesture_covered(&g, false));

    assert(loginui_gesture_press(&g, &kp, true, kx, ky, 1.0) == LOGINUI_GESTURE_NONE);
    assert(kp.pressed < 0);
    assert(loginui_gesture_motion(&g, &kp, true, kx, ky - 5, 800, 1.01) == LOGINUI_GESTURE_NONE);
    assert(loginui_gesture_motion(&g, &kp, true, kx, ky - 160, 800, 1.02) == LOGINUI_GESTURE_REDRAW);
    assert(g.dragging && g.cover < 1.0 && g.cover > 0.0);
    assert(loginui_gesture_motion(&g, &kp, true, kx, ky - 200, 800, 1.03) == LOGINUI_GESTURE_REDRAW);
    assert(loginui_gesture_release(&g, &kp, true, kx, ky - 200, 1.04, NULL) == LOGINUI_GESTURE_REDRAW);
    assert(g.cover_target == 0.0);
    settle(&g, 1.05);
    assert(g.cover == 0.0 && !loginui_gesture_animating(&g));

    int key = -1;
    assert(loginui_gesture_press(&g, &kp, true, kx, ky, 2.0) == LOGINUI_GESTURE_REDRAW);
    assert(kp.pressed == 0);
    assert(loginui_gesture_release(&g, &kp, true, kx, ky, 2.1, &key) == LOGINUI_GESTURE_KEY);
    assert(key == 0 && kp.pressed == -1);

    assert(loginui_gesture_press(&g, &kp, true, kx, ky, 3.0) == LOGINUI_GESTURE_REDRAW);
    assert(loginui_gesture_motion(&g, &kp, true, -50, -50, 800, 3.1) == LOGINUI_GESTURE_REDRAW);
    assert(kp.pressed == -1);
    assert(loginui_gesture_release(&g, &kp, true, -50, -50, 3.2, &key) == LOGINUI_GESTURE_NONE);

    assert(loginui_gesture_press(&g, &kp, false, 10, 10, 4.0) == LOGINUI_GESTURE_TAP);
    assert(loginui_gesture_release(&g, &kp, false, 10, 10, 4.1, &key) == LOGINUI_GESTURE_NONE);

    assert(loginui_gesture_press(&g, &kp, true, 240, fy, 5.0) == LOGINUI_GESTURE_TAP);
    assert(loginui_gesture_motion(&g, &kp, true, 240, fy + 180, 800, 5.1) == LOGINUI_GESTURE_REDRAW);
    assert(loginui_gesture_release(&g, &kp, true, 240, fy + 180, 5.2, NULL) == LOGINUI_GESTURE_REDRAW);
    assert(g.cover_target == 1.0);
    settle(&g, 5.3);

    assert(loginui_gesture_press(&g, &kp, true, 240, fy, 6.0) == LOGINUI_GESTURE_NONE);
    assert(loginui_gesture_release(&g, &kp, true, 240, fy, 6.1, NULL) == LOGINUI_GESTURE_REDRAW);
    assert(g.cover_target == 0.0);
    settle(&g, 6.2);

    assert(!loginui_gesture_idle(&g, true, 10.0, 30.0));
    assert(loginui_gesture_idle(&g, true, 40.0, 30.0));
    assert(!loginui_gesture_idle(&g, false, 40.0, 30.0));

    assert(loginui_gesture_press(&g, &kp, true, kx, ky, 50.0) == LOGINUI_GESTURE_REDRAW);
    g.touch_id = 3;
    assert(loginui_gesture_cancel(&g, &kp));
    assert(g.touch_id == -1 && !g.pressing && kp.pressed == -1);
    assert(!loginui_gesture_cancel(&g, &kp));

    cairo_destroy(cr);
    cairo_surface_destroy(s);
    xkb_keymap_unref(us);
    xkb_context_unref(ctx);
    return 0;
}
