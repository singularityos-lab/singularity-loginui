#ifndef SINGULARITY_LOGINUI_WIDGET_H
#define SINGULARITY_LOGINUI_WIDGET_H

/*
 * A minimal retained-mode widget layer over the loginui Cairo render primitives.
 * loginui itself is deliberately toolkit-less (a pixel-identical render library for
 * the fixed splashes); this sits ALONGSIDE it so interactive surfaces -- the recovery
 * menu, settings panes, the greeter action rows -- compose widgets with focus and key
 * handling instead of hand-drawing every frame. It reuses loginui_text /
 * loginui_rounded_rect / loginui_text_size as its drawing backend, so a widget menu
 * looks identical to the rest of the boot-to-session surface.
 */

#include <cairo/cairo.h>

typedef enum {
    LOGINUI_ITEM_LABEL,  /* static, non-focusable (a heading or hint) */
    LOGINUI_ITEM_ACTION, /* focusable, activatable (a menu choice) */
} LoginUiItemType;

typedef struct {
    LoginUiItemType type;
    const char *text;
    /* geometry, filled in by loginui_menu_render (usable for pointer hit-testing) */
    double x, y, w, h;
} LoginUiItem;

typedef struct {
    LoginUiItem *items;
    int count;
    int focus;         /* index of the focused ACTION item (-1 if none focusable) */
    const char *font;  /* Pango font string, e.g. "Sans 18" */
    double fr, fg, fb; /* focus-highlight colour */
    double tr, tg, tb; /* text colour */
} LoginUiMenu;

/* keys the caller maps its input to */
enum {
    LOGINUI_KEY_UP = 1,
    LOGINUI_KEY_DOWN = 2,
    LOGINUI_KEY_ENTER = 3,
};

/* Initialise the menu over `items` (borrowed, not copied) and focus the first ACTION.
   Sets sensible default colours; override the fields afterwards to theme it. */
void loginui_menu_init(LoginUiMenu *m, LoginUiItem *items, int count, const char *font);

/* Lay the items out as a vertical stack inside [x, y, w] and draw them: labels as
   plain text, actions as text with a rounded highlight behind the focused one. Returns
   the total height drawn (so the caller can centre or stack the menu). */
double loginui_menu_render(cairo_t *cr, LoginUiMenu *m, double x, double y, double w);

/* Handle a key. UP/DOWN move focus over ACTION items (labels are skipped, focus wraps)
   and return -1; ENTER returns the focused item index -- the activation the caller acts
   on. Returns -1 when there is nothing to activate. */
int loginui_menu_key(LoginUiMenu *m, int key);

#endif
