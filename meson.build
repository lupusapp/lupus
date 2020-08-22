project('Lupus', 'c', 'cpp', default_options : ['c_std=c18', 'cpp_std=c++17', 'warning_level=3'])

gtk = dependency('gtkmm-3.0', version: '>=3.24', required: true)
libsodium = dependency('libsodium', required: true)
toxcore = dependency('toxcore', required: true)

subdir('src')
subdir('include')
subdir('toxpp')

executable(
  meson.project_name(),
  sources: [
    'main.cpp',
    srcs,
    hdrs,
    toxpp,
    ],
  dependencies: [
    gtk,
    libsodium,
    toxcore
    ],
  cpp_args: ['-Wall', '-Werror', '-pedantic']
  )
