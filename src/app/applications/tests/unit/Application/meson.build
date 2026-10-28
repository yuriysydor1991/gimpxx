UTEST_Application_files = files(
  'UTEST_Application.cpp',
  join_paths(project_include_root, 'src', 'app', 'applications', 'Application.cpp'),
  join_paths(project_include_root, 'src', 'gimp-controller', 'GIMPControllerFactory.cpp'),
  join_paths(app_mocks_root_str, 'ApplicationContext', 'src', 'app', 'ApplicationContext.cpp')
)

UTEST_Application = executable(
  'UTEST_Application', 
  UTEST_Application_files, 
  dependencies: [gtest_deps, dependency('glib-2.0')],
  include_directories: [
    MOCK_ApplicationContext,
    MOCK_GIMPController,
    MOCK_LinuxGIMPController,
    MOCK_MacOSGIMPController,
    MOCK_WindowsGIMPController,
    MOCK_log,
    configuration_inc
  ],
  cpp_args: ['-DNDEBUG=1']
)

test(
  'gtest', 
  UTEST_Application,
  suite: ['gimpxx-cpp', 'UTEST_Application']
)
