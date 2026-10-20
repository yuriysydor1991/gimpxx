UTEST_LinuxGIMPController_files = files(
  'UTEST_LinuxGIMPController.cpp',
  join_paths(project_include_root, 'src', 'gimp-controller', 'GIMPController.cpp'),
  join_paths(project_include_root, 'src', 'gimp-controller', 'linux', 'LinuxGIMPController.cpp')
)

UTEST_LinuxGIMPController = executable(
  'UTEST_LinuxGIMPController',
  UTEST_LinuxGIMPController_files,
  dependencies: [gtest_deps, dependency('gio-2.0')],
  include_directories: [
    MOCK_gimp_c_includes,
    configuration_inc
  ],
  cpp_args: ['-DNDEBUG=1', '-DGIMP_RELEASE=1']
)

test(
  'gtest',
  UTEST_LinuxGIMPController,
  suite: ['gimpxx-cpp', 'UTEST_LinuxGIMPController']
)
