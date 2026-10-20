find_program('git', required: true)

gimpxx_extra_deps = []
gimpxx_extra_cpp_args = []
gimpxx_extra_link_args = []
# The sanitizers instrumentation every target, the tests among them, carries.
gimpxx_sanitizers_deps = []

subdir('gimpxx-project-hardening')

subdir('gimpxx-project-all-source-files')
subdir('gimpxx-project-misc-variables-declare')
subdir('gimpxx-project-configure-file')

if get_option('ENABLE_SANITIZERS') or get_option('ENABLE_SANITIZERS_THREADS')
  subdir('gimpxx-project-sanitizers')
endif

if get_option('ENABLE_GPROF')
  subdir('gimpxx-project-profiler-gprof')
endif

if get_option('ENABLE_CPPCHECK')
  subdir('gimpxx-project-cppcheck-target')
endif

if get_option('ENABLE_CLANGFORMAT')
  subdir('gimpxx-project-clang-format-target')
endif

if get_option('ENABLE_CLANG_TIDY')
  subdir('gimpxx-project-clang-tidy-target')
endif

if get_option('ENABLE_LIBCURL')
  subdir('gimpxx-project-libcurl-enabler')
endif

if get_option('ENABLE_LIBXML2')
  subdir('gimpxx-project-libxml2-enabler')
endif

if get_option('ENABLE_NLOHMANN_JSON')
  subdir('gimpxx-project-nlohmann-json-enabler')
endif

if get_option('ENABLE_OPENCV')
  subdir('gimpxx-project-OpenCV-enabler')
endif

if get_option('ENABLE_FLATPAK')
  subdir('gimpxx-project-flatpak-target')
endif

if get_option('ENABLE_SNAP')
  subdir('gimpxx-project-snap-enabler')
endif

if get_option('ENABLE_DEB')
  subdir('gimpxx-project-deb-enabler')
endif

if get_option('ENABLE_RPM')
  subdir('gimpxx-project-rpm-enabler')
endif

if get_option('ENABLE_FREEBSD_PKG')
  subdir('gimpxx-project-freebsd-pkg-enabler')
endif

if get_option('ENABLE_APPIMAGE')
  subdir('gimpxx-project-AppImage-enabler')
endif

if get_option('ENABLE_WIX')
  subdir('gimpxx-project-wix-enabler')
endif

if get_option('ENABLE_DOCKER')
  subdir('gimpxx-project-dockerers')
endif
