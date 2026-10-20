subdir('GIMPController')
subdir('LinuxGIMPController')

# The macOS specifics use the POSIX and the GLib calls only.
if host_machine.system() != 'windows'
  subdir('MacOSGIMPController')
endif
