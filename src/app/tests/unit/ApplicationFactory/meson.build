UTEST_ApplicationFactory_files = files(
  'UTEST_ApplicationFactory.cpp',
  join_paths(project_include_root, 'src', 'app', 'ApplicationFactory.cpp'),
  join_paths(app_mocks_root_str, 'ApplicationContext', 'src', 'app', 'ApplicationContext.cpp')
)

UTEST_ApplicationFactory = executable(
  'UTEST_ApplicationFactory', 
  UTEST_ApplicationFactory_files, 
  dependencies: [gtest_deps, dependency('glib-2.0')],
  include_directories: [
    MOCK_ApplicationContext,
    MOCK_Application,
    MOCK_ApplicationVersionPrinter,
    MOCK_CommandLineParser,
    MOCK_GimpSignalsHandler,
    MOCK_gimp_main,
    MOCK_log,
    configuration_inc
  ],
  cpp_args: ['-DNDEBUG=1']
)

test(
  'gtest', 
  UTEST_ApplicationFactory,
  suite: ['gimpxx', 'UTEST_ApplicationFactory']
)
