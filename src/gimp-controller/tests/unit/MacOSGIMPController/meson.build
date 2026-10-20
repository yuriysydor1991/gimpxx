UTEST_MacOSGIMPController_files = files(
  'UTEST_MacOSGIMPController.cpp',
  join_paths(project_include_root, 'src', 'gimp-controller', 'GIMPController.cpp'),
  join_paths(project_include_root, 'src', 'gimp-controller', 'macos', 'MacOSGIMPController.cpp')
)

# The macOS specifics use the POSIX and the GLib calls only, so they are tested
# on every POSIX platform, the ones of the relocatable bundle included.
UTEST_MacOSGIMPController = executable(
  'UTEST_MacOSGIMPController',
  UTEST_MacOSGIMPController_files,
  dependencies: [gtest_deps, dependency('gio-2.0')],
  include_directories: [
    MOCK_gimp_c_includes,
    configuration_inc
  ],
  cpp_args: [
    '-DNDEBUG=1',
    '-DGIMP_RELEASE=1',
    '-DENABLE_RELOCATABLE_RESOURCES=1'
  ]
)

test(
  'gtest',
  UTEST_MacOSGIMPController,
  suite: ['gimpxx-cpp', 'UTEST_MacOSGIMPController']
)
