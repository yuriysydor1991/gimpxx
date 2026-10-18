project('gimpxx',
  'c', 'cpp',
  version: '3.2.6',
  meson_version: '>=0.61.2',
  default_options: [
    #'c_std=none', # c11 breaks Linux/GNU, gnu11 breaks Windows/MSVC, so set to none
    'cpp_std=c++17',
    'buildtype=debugoptimized',
  ],
)

subdir('meson')
subdir('src')

if get_option('ENABLE_DOC_DOXYGEN')
  subdir('doc')
endif
