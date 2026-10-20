cpp = meson.get_compiler('cpp')

# Each compile options module keeps the options of a compiler family in its
# own subdirectory named after it, as the cmake/compile-options of the
# template: MSVC for the MSVC like compilers (cl, clang-cl), GCC for any other.
gimpxx_compiler_family = {'msvc': 'MSVC'}.get(cpp.get_argument_syntax(), 'GCC')

subdir('gimpxx-project-hardening')

if get_option('ENABLE_SANITIZERS') or get_option('ENABLE_SANITIZERS_THREADS')
  subdir('gimpxx-project-sanitizers')
endif
