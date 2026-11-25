#include "loginui.h"

#include <glib.h>

#define FRAME_MS 16

static int until_next_minute(void) {
    gint64 ms = g_get_real_time() / 1000;
    return (int)(60000 - ms % 60000) + 5;
}

static int earliest(int a, int b) {
    if (a < 0) return b;
    if (b < 0) return a;
    return a < b ? a : b;
}

int loginui_next_wakeup(const LoginUiGesture *g, bool keypad_shown, bool idle_armed, double now, double idle_timeout) {
    if (loginui_gesture_animating(g)) return FRAME_MS;
    int timeout = earliest(until_next_minute(), loginui_caret_timeout(-1));
    if (idle_armed) timeout = earliest(timeout, loginui_gesture_idle_timeout(g, keypad_shown, now, idle_timeout));
    return timeout;
}
