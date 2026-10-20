# The platform implementation the ApplicationFactory creates.
if host_machine.system() == 'windows'
  subdir('windows')
elif host_machine.system() == 'darwin'
  subdir('macos')
else
  subdir('linux')
endif

# The GIMPController calls the GIMP C code, so src/gimp/app/meson.build
# compiles these sources for each GIMP executable, the way it compiled the
# former main.c: the console one with the GIMP_CONSOLE_COMPILATION macro.
gimp_controller_sources = files(
  'GIMPController.cpp'
) + gimp_controller_platform_sources

if get_option('ENABLE_UNIT_TESTS') or get_option('ENABLE_COMPONENT_TESTS')
  # Declared before the src/app tests are entered because their unit tests
  # consume these mocks.
  subdir('tests/mocks')
endif
