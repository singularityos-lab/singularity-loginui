#include "loginui.h"

#include <assert.h>
#include <stdio.h>

int main(void) {
    LoginUiGesture g;
    loginui_gesture_init(&g);

    assert(loginui_gesture_idle_timeout(&g, true, 100.0, 30.0) == -1);

    g.cover = g.cover_target = 0.0;
    g.last_input = 100.0;
    int left = loginui_gesture_idle_timeout(&g, true, 110.0, 30.0);
    assert(left > 19990 && left <= 20001);
    assert(loginui_gesture_idle_timeout(&g, true, 131.0, 30.0) == 0);
    assert(loginui_gesture_idle_timeout(&g, false, 110.0, 30.0) == -1);
    g.pressing = true;
    assert(loginui_gesture_idle_timeout(&g, true, 110.0, 30.0) == -1);
    g.pressing = false;

    int idle = loginui_next_wakeup(&g, false, false, 110.0, 30.0);
    assert(idle > 0 && idle <= 60005);

    int armed = loginui_next_wakeup(&g, true, true, 129.5, 30.0);
    assert(armed <= 501);

    g.cover_target = 1.0;
    assert(loginui_next_wakeup(&g, true, true, 110.0, 30.0) == 16);

    puts("ok");
    return 0;
}
