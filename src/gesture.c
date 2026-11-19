#include "loginui.h"

#define COVER_SECONDS 0.26
#define DRAG_SLOP 10.0
#define FLING 2.0

void loginui_gesture_init(LoginUiGesture *g) {
    *g = (LoginUiGesture){0};
    g->cover = g->cover_from = g->cover_target = 1.0;
    g->drag_base = 1.0;
    g->touch_id = -1;
}

bool loginui_gesture_covered(const LoginUiGesture *g, bool keypad_shown) {
    return keypad_shown && g->cover_target > 0.5;
}

bool loginui_gesture_set_cover(LoginUiGesture *g, double target, double now) {
    if (target == g->cover_target && g->cover == target) return false;
    g->cover_from = g->cover;
    g->cover_target = target;
    g->cover_t0 = now;
    if (loginui_reduced_motion()) g->cover = target;
    return true;
}

bool loginui_gesture_animating(const LoginUiGesture *g) {
    return g->cover != g->cover_target;
}

bool loginui_gesture_tick(LoginUiGesture *g, double now) {
    if (g->cover == g->cover_target) return false;
    double t = (now - g->cover_t0) / COVER_SECONDS;
    if (t >= 1.0) {
        g->cover = g->cover_target;
    } else {
        double e = 1.0 - (1.0 - t) * (1.0 - t) * (1.0 - t);
        g->cover = g->cover_from + (g->cover_target - g->cover_from) * e;
    }
    return true;
}

bool loginui_gesture_idle(const LoginUiGesture *g, bool keypad_shown, double now, double timeout) {
    return keypad_shown && g->cover_target == 0.0 && !g->pressing && now - g->last_input > timeout;
}

LoginUiGestureEvent loginui_gesture_press(LoginUiGesture *g, LoginUiKeypad *kp, bool keypad_shown,
                                          double x, double y, double now) {
    g->last_input = now;
    g->pressing = true;
    g->swiped = false;
    g->dragging = false;
    g->press_y = g->last_y = y;
    g->last_dy = 0.0;
    if (loginui_gesture_covered(g, keypad_shown)) return LOGINUI_GESTURE_NONE;
    int i = keypad_shown ? loginui_keypad_key_at(kp, x, y) : -1;
    if (i >= 0) {
        kp->pressed = i;
        return LOGINUI_GESTURE_REDRAW;
    }
    return LOGINUI_GESTURE_TAP;
}

LoginUiGestureEvent loginui_gesture_motion(LoginUiGesture *g, LoginUiKeypad *kp, bool keypad_shown,
                                           double x, double y, double height, double now) {
    if (!g->pressing) return LOGINUI_GESTURE_NONE;
    g->last_dy = y - g->last_y;
    g->last_y = y;
    if (keypad_shown && kp->pressed < 0) {
        double dy = y - g->press_y;
        bool covered = loginui_gesture_covered(g, keypad_shown);
        if (!g->dragging && (dy > DRAG_SLOP || dy < -DRAG_SLOP)) {
            if (covered && dy < 0) { g->dragging = true; g->drag_base = 1.0; }
            else if (!covered && dy > 0) { g->dragging = true; g->drag_base = 0.0; }
        }
        if (!g->dragging) return LOGINUI_GESTURE_NONE;
        double span = height > 0 ? height * 0.4 : 400.0;
        double v = g->drag_base + dy / span;
        if (v < 0.0) v = 0.0;
        if (v > 1.0) v = 1.0;
        g->cover = g->cover_target = v;
        g->swiped = true;
        g->last_input = now;
        return LOGINUI_GESTURE_REDRAW;
    }
    if (kp->pressed >= 0 && loginui_keypad_key_at(kp, x, y) != kp->pressed) {
        kp->pressed = -1;
        return LOGINUI_GESTURE_REDRAW;
    }
    return LOGINUI_GESTURE_NONE;
}

LoginUiGestureEvent loginui_gesture_release(LoginUiGesture *g, LoginUiKeypad *kp, bool keypad_shown,
                                            double x, double y, double now, int *key) {
    if (!g->pressing) return LOGINUI_GESTURE_NONE;
    g->pressing = false;
    g->last_input = now;
    if (g->dragging) {
        g->dragging = false;
        double target = g->last_dy < -FLING ? 0.0 : g->last_dy > FLING ? 1.0 : g->cover < 0.5 ? 0.0 : 1.0;
        loginui_gesture_set_cover(g, target, now);
        return LOGINUI_GESTURE_REDRAW;
    }
    if (loginui_gesture_covered(g, keypad_shown)) {
        if (!g->swiped && loginui_gesture_set_cover(g, 0.0, now)) return LOGINUI_GESTURE_REDRAW;
        return LOGINUI_GESTURE_NONE;
    }
    int i = kp->pressed;
    if (i < 0) return LOGINUI_GESTURE_NONE;
    kp->pressed = -1;
    if (loginui_keypad_key_at(kp, x, y) != i) return LOGINUI_GESTURE_REDRAW;
    if (key) *key = i;
    return LOGINUI_GESTURE_KEY;
}

bool loginui_gesture_cancel(LoginUiGesture *g, LoginUiKeypad *kp) {
    g->touch_id = -1;
    g->pressing = false;
    g->dragging = false;
    if (kp->pressed < 0) return false;
    kp->pressed = -1;
    return true;
}
