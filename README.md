# singularity-loginui

> [!IMPORTANT]
> Report bugs and request features in the
> [Singularity Desktop tracker](https://github.com/singularityos-lab/singularity-desktop/issues/new/choose).

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

## Use of Generative AI

Maintainers may use generative AI tools as assistants while working on singularity-loginui. Non-trivial assisted commits disclose the tool, model, and scope of the work.

AI tools may assist with code comments, documentation, repetitive code, and issue triage. Maintainers make project decisions and review every assisted change before it is merged.

Use these trailers for non-trivial assisted commits:

```plain
Assisted-by: <tool>:<model-version>
AI-Scope: <what the tool generated and the prompt or a short prompt summary>
```

Single-line completions, renames, and formatting changes do not need trailers.

Coding agents must also follow [AGENTS.md](AGENTS.md) before changing files,
creating commits, or opening pull requests.
