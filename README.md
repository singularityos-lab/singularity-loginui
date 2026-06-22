# singularity-loginui

Shared toolkit-less Cairo render library for the Singularity login surfaces:
the greeter, the lock screen, the session splash and the boot splash. It draws
to a `cairo_t` so each consumer can present it on any backend (Wayland
layer-shell or raw KMS), to keep the visual pixels identical across the boot to
session process.

## Build

```sh
meson setup build
ninja -C build
```
