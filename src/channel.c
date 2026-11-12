#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "loginui.h"

const char *loginui_update_channel(const char *path) {
    static char label[33];
    char buf[64] = "";
    FILE *f = fopen(path ? path : LOGINUI_CHANNEL_PATH, "r");
    if (!f) return NULL;
    if (!fgets(buf, sizeof buf, f)) buf[0] = '\0';
    fclose(f);
    size_t n = strcspn(buf, "\r\n");
    buf[n] = '\0';
    if (n == 0 || n >= sizeof label || strcmp(buf, "stable") == 0) return NULL;
    for (size_t i = 0; i < n; i++) {
        unsigned char c = (unsigned char)buf[i];
        if (!islower(c) && !isdigit(c) && c != '-') return NULL;
        label[i] = (char)toupper(c);
    }
    label[n] = '\0';
    return label;
}
