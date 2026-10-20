subdir('gimpxx-project-hardening-' + gimpxx_compiler_family)

# The hardening options are the compiler, the linker and the target CPU
# specific ones, so the unsupported ones are dropped instead of breaking the
# build. They go to the gimpxx targets only, not project wide, so the
# original GIMP tree keeps its own flags.
gimpxx_extra_cpp_args += cpp.get_supported_arguments(gimpxx_hardening_cpp_args)
gimpxx_extra_link_args += cpp.get_supported_link_arguments(gimpxx_hardening_link_args)
