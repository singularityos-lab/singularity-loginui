project('singularity-loginui', 'c',
  version: '0.1.0',
  meson_version: '>= 0.59.0',
)

cc = meson.get_compiler('c')

loginui_deps = [
  dependency('cairo'),
  dependency('pangocairo'),
  dependency('gdk-pixbuf-2.0'),
  dependency('wayland-client'),
  cc.find_library('m', required: false),
]

loginui_sources = files(
  'src/image.c',
  'src/buffer.c',
  'src/draw.c',
  'src/render.c',
)

loginui_lib = static_library('loginui',
  loginui_sources,
  dependencies: loginui_deps,
  include_directories: include_directories('.', 'src'),
  install: false,
)

loginui_dep = declare_dependency(
  link_with: loginui_lib,
  include_directories: include_directories('.'),
  dependencies: loginui_deps,
)
