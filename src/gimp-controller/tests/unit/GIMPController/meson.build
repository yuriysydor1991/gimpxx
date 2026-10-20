UTEST_GIMPController_files = files(
  'UTEST_GIMPController.cpp',
  join_paths(project_include_root, 'src', 'gimp-controller', 'GIMPController.cpp')
)

# The GUI binary of a release build and the console binary of a development
# build, so the two cover both the binaries and the build kinds.
foreach variant : [
    ['UTEST_GIMPController', ['-DGIMP_RELEASE=1']],
    ['UTEST_GIMPController_console', ['-DGIMP_CONSOLE_COMPILATION']],
  ]
  UTEST_GIMPController = executable(
    variant[0],
    UTEST_GIMPController_files,
    dependencies: [gtest_deps, dependency('gio-2.0')],
    include_directories: [
      MOCK_gimp_c_includes,
      configuration_inc
    ],
    cpp_args: ['-DNDEBUG=1'] + variant[1]
  )

  test(
    'gtest',
    UTEST_GIMPController,
    suite: ['gimpxx-cpp', variant[0]]
  )
endforeach
