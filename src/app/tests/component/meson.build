CTEST_app_files = files(
  'CTEST_app.cpp',
  join_paths(project_include_root, 'src', 'app', 'applications', 'Application.cpp'),
  join_paths(project_include_root, 'src', 'app', 'ApplicationContext.cpp'),
  join_paths(project_include_root, 'src', 'app', 'ApplicationFactory.cpp'),
  join_paths(project_include_root, 'src', 'app', 'CommandLineParser.cpp'),
  join_paths(project_include_root, 'src', 'app', 'applications', 'ApplicationVersionPrinter.cpp'),
  join_paths(project_include_root, 'src', 'gimp-controller', 'GIMPController.cpp'),
  join_paths(project_include_root, 'src', 'gimp-controller', 'linux', 'LinuxGIMPController.cpp'),
  join_paths(project_include_root, 'src', 'app', 'signals-handlers', 'GimpSignalsHandler.cpp'),
  join_paths(project_include_root, 'src', 'app', 'signals-handlers', 'SignalsHandler.cpp'),
  join_paths(project_include_root, 'src', 'app', 'signals-handlers', 'handlers', 'ASignalsHandler.cpp'),
  join_paths(project_include_root, 'src', 'app', 'signals-handlers', 'handlers', 'posix', 'ContinueSignalsHandler.cpp'),
  join_paths(project_include_root, 'src', 'app', 'signals-handlers', 'handlers', 'posix', 'IgnoredSignalsHandler.cpp'),
  join_paths(project_include_root, 'src', 'app', 'signals-handlers', 'handlers', 'posix', 'PauseSignalsHandler.cpp'),
  join_paths(project_include_root, 'src', 'app', 'signals-handlers', 'handlers', 'posix', 'PosixSignalsHandler.cpp'),
  join_paths(project_include_root, 'src', 'app', 'signals-handlers', 'handlers', 'posix', 'ReloadSignalsHandler.cpp'),
  join_paths(project_include_root, 'src', 'app', 'signals-handlers', 'handlers', 'posix', 'StopSignalsHandler.cpp'),
  join_paths(project_include_root, 'src', 'app', 'signals-handlers', 'handlers', 'posix', 'UserSignalsHandler.cpp'),
  join_paths(project_include_root, 'src', 'log', 'default-logger', 'DefaultLogger.cpp'),
  join_paths(project_include_root, 'src', 'log', 'default-logger', 'real-default-logger', 'RealDefaultLogger.cpp'),
)

CTEST_app = executable(
  'CTEST_app',
  CTEST_app_files,
  dependencies: [gtest_deps, dependency('gio-2.0')],
  # The real classes and logger, but the GIMP C functions mocks, as the GIMP
  # application code is not linkable here.
  include_directories: [
    MOCK_gimp_c_includes,
    configuration_inc
  ],
  cpp_args: ['-DGIMP_RELEASE=1']
)

test(
  'gtest',
  CTEST_app,
  suite: ['gimpxx-cpp', 'CTEST_app']
)
