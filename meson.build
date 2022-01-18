
project(
  'triangle_tut',
  'cpp',
  default_options : [
    'buildtype=debugoptimized',
    'warning_level=3',
    'werror=true',
    'cpp_std=c++20'
  ]
)

src = [ 'src/main.cpp' ]

pch = 'src/pch.hpp'

exe = executable(
  'triangle_test',
  sources : src,
  cpp_pch : pch
)
