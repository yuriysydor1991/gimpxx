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
