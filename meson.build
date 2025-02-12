
project(
  'triangle',
  'cpp',
  default_options : [
    'buildtype=release',
    'warning_level=3',
    'cpp_std=c++20'
  ]
)

vulkan = dependency( 'vulkan', method : 'pkg-config' )
fmt = dependency( 'fmt', method : 'pkg-config' )
# x = dependency( 'x11', method : 'pkg-config' )
# wayland = dependency( 'wayland-client++', method : 'pkg-config' )
# wayland_cursor = dependency( 'wayland-cursor++', method : 'pkg-config' )
# wayland_extra = dependency( 'wayland-client-extra++', method : 'pkg-config' )
glfw = dependency( 'glfw3', method : 'pkg-config' )

engine_src = [
  'engine/impl.cpp',
  'engine/component.cpp',
  'engine/utility.cpp',
  'engine/object.cpp',
  'engine/window.cpp'
]

custom_target(
  'vertex_shader',
  output: [ 'vert.spv' ],
  input: [ 'shader/vertex.hlsl' ],
  install: true,
  install_dir: 'shader',
  command: [
    'glslc',
    '-o', 'vert.spv',
    '-fshader-stage=vertex',
    '--target-env=vulkan1.3',
    '../shader/vertex.hlsl'
  ]
)

custom_target(
  'fragment_shader',
  output: [ 'frag.spv' ],
  input: [ 'shader/fragment.hlsl' ],
  install: true,
  install_dir: 'shader',
  command: [
    'glslc',
    '-o', 'frag.spv',
    '-fshader-stage=fragment',
    '--target-env=vulkan1.3',
    '../shader/fragment.hlsl'
  ]
)

engine = library(
  'engine-xorg',
  sources : engine_src,
  cpp_args : [ '-D', 'NDEBUG', '-g', '-fno-exceptions' ],
  cpp_pch : 'engine/pch.hpp',
  dependencies : [ vulkan, fmt, glfw ]
)

engine_d = library(
  'engine-xorg-debug',
  sources : engine_src,
  cpp_args : [ '-g', '-fno-exceptions' ],
  cpp_pch : 'engine/pch.hpp',
  dependencies : [ vulkan, fmt, glfw ],
  override_options : [ 'buildtype=debug' ]
)

# wl_engine = library(
#   'engine-wayland',
#   sources : engine_src,
#   cpp_args : [ '-D', 'NDEBUG', '-g', '-fno-exceptions',
#     '-D', 'WAYLAND' ],
#   cpp_pch : 'engine/pch.hpp',
#   dependencies : [ vulkan, wayland, wayland_extra, fmt ]
# )

# wl_engine_d = library(
#   'engine-wayland-debug',
#   sources : engine_src,
#   cpp_args : [ '-g', '-fno-exceptions',
#     '-D', 'WAYLAND' ],
#   cpp_pch : 'engine/pch.hpp',
#   dependencies : [ vulkan, wayland, wayland_extra, fmt ],
#   override_options : [ 'buildtype=debug' ]
# )

src = [ 'src/main.cpp' ]

pch = 'src/pch.hpp'

exe = executable(
  'triangle.xorg',
  sources : src,
  include_directories : 'engine',
  cpp_args : [ '-D', 'NDEBUG', '-g' ],
  cpp_pch : pch,
  dependencies : [ fmt ],
  link_with : engine
)

exe = executable(
  'triangle-debug.xorg',
  sources : src,
  include_directories : 'engine',
  cpp_pch : pch,
  cpp_args : [ '-g' ],
  link_with : engine_d,
  dependencies : [ fmt ],
  override_options : [ 'buildtype=debug' ]
)

# exe = executable(
#   'triangle.wayland',
#   sources : src,
#   include_directories : 'engine',
#   cpp_args : [ '-D', 'NDEBUG', '-g',
#     '-D', 'WAYLAND' ],
#   cpp_pch : pch,
#   link_with : wl_engine
# )

# exe = executable(
#   'triangle-debug.wayland',
#   sources : src,
#   include_directories : 'engine',
#   cpp_pch : pch,
#   cpp_args : [ '-g',
#     '-D', 'WAYLAND' ],
#   link_with : wl_engine_d,
#   override_options : [ 'buildtype=debug' ]
# )
