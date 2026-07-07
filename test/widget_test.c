#include "widget.h"
#include "loginui.h"
#include <cairo/cairo.h>
#include <assert.h>

int main(void) {
    LoginUiItem items[] = {
        { LOGINUI_ITEM_LABEL,  "Recovery", 0, 0, 0, 0 },
        { LOGINUI_ITEM_ACTION, "Wifi",     0, 0, 0, 0 },
        { LOGINUI_ITEM_ACTION, "Reinstall",0, 0, 0, 0 },
        { LOGINUI_ITEM_ACTION, "Repair",   0, 0, 0, 0 },
        { LOGINUI_ITEM_ACTION, "Reboot",   0, 0, 0, 0 },
    };
    LoginUiMenu m;
    loginui_menu_init(&m, items, 5, "Sans 16");
    assert(m.focus == 1);

    loginui_menu_key(&m, LOGINUI_KEY_DOWN);
    assert(m.focus == 2);
    loginui_menu_key(&m, LOGINUI_KEY_UP);
    loginui_menu_key(&m, LOGINUI_KEY_UP);
    assert(m.focus == 4); /* wraps past the label */
    assert(loginui_menu_key(&m, LOGINUI_KEY_ENTER) == 4);

    cairo_surface_t *s = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 640, 480);
    cairo_t *cr = cairo_create(s);
    loginui_menu_render(cr, &m, 40, 40, 320);
    assert(items[2].w == 320 && items[2].h > 0);
    cairo_destroy(cr);
    cairo_surface_destroy(s);
    return 0;
}
