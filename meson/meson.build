find_program('git', required: true)

gimpxx_extra_deps = []
gimpxx_extra_cpp_args = []
gimpxx_extra_link_args = []
# The sanitizers instrumentation every target, the tests among them, carries.
gimpxx_sanitizers_deps = []

subdir('compile-options')

subdir('gimpxx-project-all-source-files')
subdir('gimpxx-project-misc-variables-declare')
subdir('gimpxx-project-configure-file')

subdir('profilers')
subdir('enablers')
subdir('packagers')
subdir('dockerers')
