#define _GNU_SOURCE
#include "loginui.h"

#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>

static void buffer_release(void *data, struct wl_buffer *wl_buffer) {
    struct loginui_buffer *b = data;
    wl_buffer_destroy(b->wl_buffer);
    munmap(b->data, b->size);
    free(b);
}
static const struct wl_buffer_listener buffer_listener = { buffer_release };

struct loginui_buffer *loginui_create_buffer(struct wl_shm *shm,
                                             uint32_t w, uint32_t h,
                                             cairo_t **cr_out) {
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
    b->data = data; b->size = size; b->wl_buffer = wl_buffer;
    wl_buffer_add_listener(wl_buffer, &buffer_listener, b);

    cairo_surface_t *cs = cairo_image_surface_create_for_data(
        data, CAIRO_FORMAT_ARGB32, (int)w, (int)h, (int)stride);
    *cr_out = cairo_create(cs);
    cairo_surface_destroy(cs);
    return b;
}
