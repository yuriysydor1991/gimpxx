configuration_inc = [
  configuration_inc,
  include_directories('.')
]

if get_option('ENABLE_UNIT_TESTS') or get_option('ENABLE_COMPONENT_TESTS')
  gtest_dep = dependency('gtest', required: true)
  gmock_dep = dependency('gmock', required: true)
  gtest_main_dep = dependency('gtest_main', required: true)

  gtest_deps = [gtest_dep, gmock_dep, gtest_main_dep] + gimpxx_sanitizers_deps
endif

# The logging subsystem goes first: it sets the project wide log macros
# arguments, which Meson accepts before the first target only.
subdir('log')
subdir('gimp-controller')
subdir('app')

# The C++ entry layer of gimpxx, which starts GIMP. The GIMP tree calls
# add_project_arguments() as well, so the layer can not be a target before
# it: src/gimp/app/meson.build builds it of these sources and of the
# gimp_controller_sources and links the GIMP executables of it.
gimpxx_sources = files('main.cpp') + app_sources + log_sources
gimpxx_includes = configuration_inc

# The original GIMP tree.
subdir('gimp')

# The binary the valgrind, callgrind and gprof targets run.
gimpxx_main_binary = enable_console_bin ? gimpconsole_exe : gimpmain_exe

if get_option('ENABLE_VALGRIND')
  valgrind_exec = find_program('valgrind', required: true)
  custom_target(
    'valgrind',
    command: [
      valgrind_exec,
      '--leak-check=full',
      '--error-exitcode=1',
      '--show-leak-kinds=all',
      gimpxx_main_binary
    ],
    depends: [gimpxx_main_binary],
    output: 'valgrind-output-stub.txt',
    build_always_stale: true,
    console: true
  )
endif

if get_option('ENABLE_CALLGRIND')
  callgrind_exec = find_program('valgrind', required: true)
  custom_target(
    'callgrind',
    command: [
      callgrind_exec,
      '--tool=callgrind',
      gimpxx_main_binary
    ],
    depends: [gimpxx_main_binary],
    output: 'callgrind-output-stub.txt',
    build_always_stale: true,
    console: true
  )
endif

if get_option('ENABLE_GPROF')
  gprof_exec = find_program('gprof', required: true)
  # The profiled binary writes the gmon.out into its working directory, so it
  # runs from the very directory the gprof reads it back from.
  custom_target(
    'gprof-analyze',
    command: [
      'sh', '-c',
      'cd ' + meson.current_build_dir() + ' && ' +
      gimpxx_main_binary.full_path() + ' && ' +
      gprof_exec.full_path() + ' ' + gimpxx_main_binary.full_path() +
      ' gmon.out > gprof-analyze.txt'
    ],
    depends: [gimpxx_main_binary],
    output: 'gprof-analyze.txt',
    build_always_stale: true,
    console: true
  )
endif

# The tests are targets too, so they come after the GIMP tree as well.
if get_option('ENABLE_COMPONENT_TESTS')
  subdir('log/tests/component/log')
  subdir('log/default-logger/tests/component/DefaultLogger')
endif

if get_option('ENABLE_UNIT_TESTS') or get_option('ENABLE_COMPONENT_TESTS')
  subdir('app/tests')
  subdir('app/applications/tests')
  subdir('app/signals-handlers/tests')
  subdir('app/signals-handlers/handlers/tests')
  subdir('app/signals-handlers/handlers/posix/tests')
  subdir('gimp-controller/tests')
endif
