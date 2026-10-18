project('gimp',
  'c', 'cpp',
  version: '3.2.6',
  meson_version: '>=0.61.0',
  default_options: [
    #'c_std=none', # c11 breaks Linux/GNU, gnu11 breaks Windows/MSVC, so set to none
    'cpp_std=c++14',
    'buildtype=debugoptimized',
  ],
)

subdir('src')
