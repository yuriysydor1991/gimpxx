UTEST_GimpSignalsHandler_files = files(
  'UTEST_GimpSignalsHandler.cpp',
  join_paths(project_include_root, 'src', 'app', 'signals-handlers', 'GimpSignalsHandler.cpp'),
  join_paths(app_mocks_root_str, 'ApplicationContext', 'src', 'app', 'ApplicationContext.cpp')
)

UTEST_GimpSignalsHandler = executable(
  'UTEST_GimpSignalsHandler',
  UTEST_GimpSignalsHandler_files,
  dependencies: [gtest_deps, dependency('glib-2.0')],
  include_directories: [
    MOCK_ApplicationContext,
    MOCK_gimp_main,
    MOCK_log,
    configuration_inc
  ],
  cpp_args: ['-DNDEBUG=1']
)

test(
  'gtest',
  UTEST_GimpSignalsHandler,
  suite: ['gimpxx', 'UTEST_GimpSignalsHandler']
)
