#define _GNU_SOURCE
#include "loginui.h"

#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>

static void buffer_destroy(struct loginui_buffer *b) {
    wl_buffer_destroy(b->wl_buffer);
    munmap(b->data, b->size);
    free(b);
}

static void buffer_release(void *data, struct wl_buffer *wl_buffer) {
    struct loginui_buffer *b = data;
    if (b->pooled) b->busy = false;
    else buffer_destroy(b);
}
static const struct wl_buffer_listener buffer_listener = { buffer_release };

static struct loginui_buffer *buffer_new(struct wl_shm *shm, uint32_t w, uint32_t h) {
    uint32_t stride = w * 4;
    size_t size = (size_t)stride * h;
    int fd = memfd_create("singularity-loginui", MFD_CLOEXEC);
    if (fd < 0) return NULL;
    if (ftruncate(fd, (off_t)size) < 0) { close(fd); return NULL; }
    void *data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (data == MAP_FAILED) { close(fd); return NULL; }
    struct wl_shm_pool *pool = wl_shm_create_pool(shm, fd, (int32_t)size);
    struct wl_buffer *wl_buffer = wl_shm_pool_create_buffer(
        pool, 0, (int32_t)w, (int32_t)h, (int32_t)stride, WL_SHM_FORMAT_ARGB8888);
    wl_shm_pool_destroy(pool);
    close(fd);

    struct loginui_buffer *b = calloc(1, sizeof(*b));
    if (!b) { wl_buffer_destroy(wl_buffer); munmap(data, size); return NULL; }
    b->data = data; b->size = size; b->wl_buffer = wl_buffer;
    b->width = w; b->height = h;
    wl_buffer_add_listener(wl_buffer, &buffer_listener, b);
    return b;
}

static cairo_t *buffer_cairo(struct loginui_buffer *b) {
    cairo_surface_t *cs = cairo_image_surface_create_for_data(
        b->data, CAIRO_FORMAT_ARGB32, (int)b->width, (int)b->height, (int)b->width * 4);
    cairo_t *cr = cairo_create(cs);
    cairo_surface_destroy(cs);
    return cr;
}

struct loginui_buffer *loginui_create_buffer(struct wl_shm *shm,
                                             uint32_t w, uint32_t h,
                                             cairo_t **cr_out) {
    struct loginui_buffer *b = buffer_new(shm, w, h);
    if (!b) return NULL;
    *cr_out = buffer_cairo(b);
    return b;
}

struct loginui_buffer *loginui_pool_buffer(struct wl_shm *shm, struct loginui_buffer **slots, int n,
                                           uint32_t w, uint32_t h, cairo_t **cr_out) {
    int free_slot = -1;
    for (int i = 0; i < n; i++) {
        struct loginui_buffer *b = slots[i];
        if (b && !b->busy && b->width == w && b->height == h) {
            b->busy = true;
            *cr_out = buffer_cairo(b);
            return b;
        }
        if (free_slot < 0 && (!b || !b->busy)) free_slot = i;
    }
    if (free_slot < 0) return loginui_create_buffer(shm, w, h, cr_out);
    if (slots[free_slot]) buffer_destroy(slots[free_slot]);
    slots[free_slot] = buffer_new(shm, w, h);
    if (!slots[free_slot]) return NULL;
    slots[free_slot]->pooled = true;
    slots[free_slot]->busy = true;
    *cr_out = buffer_cairo(slots[free_slot]);
    return slots[free_slot];
}
