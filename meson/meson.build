find_program('git', required: true)

gimpxx_extra_deps = []
gimpxx_extra_cpp_args = []
gimpxx_extra_link_args = []
# The sanitizers instrumentation every target, the tests among them, carries.
gimpxx_sanitizers_deps = []

subdir('gimpxx-hardening')

subdir('gimpxx-all-source-files')
subdir('gimpxx-misc-variables-declare')
subdir('gimpxx-configure-file')

if get_option('ENABLE_SANITIZERS') or get_option('ENABLE_SANITIZERS_THREADS')
  subdir('gimpxx-sanitizers')
endif

if get_option('ENABLE_GPROF')
  subdir('gimpxx-profiler-gprof')
endif

if get_option('ENABLE_CPPCHECK')
  subdir('gimpxx-cppcheck-target')
endif

if get_option('ENABLE_CLANGFORMAT')
  subdir('gimpxx-clang-format-target')
endif

if get_option('ENABLE_CLANG_TIDY')
  subdir('gimpxx-clang-tidy-target')
endif

if get_option('ENABLE_LIBCURL')
  subdir('gimpxx-libcurl-enabler')
endif

if get_option('ENABLE_LIBXML2')
  subdir('gimpxx-libxml2-enabler')
endif

if get_option('ENABLE_NLOHMANN_JSON')
  subdir('gimpxx-nlohmann-json-enabler')
endif

if get_option('ENABLE_OPENCV')
  subdir('gimpxx-OpenCV-enabler')
endif

if get_option('ENABLE_FLATPAK')
  subdir('gimpxx-flatpak-target')
endif

if get_option('ENABLE_SNAP')
  subdir('gimpxx-snap-enabler')
endif

if get_option('ENABLE_DEB')
  subdir('gimpxx-deb-enabler')
endif

if get_option('ENABLE_RPM')
  subdir('gimpxx-rpm-enabler')
endif

if get_option('ENABLE_FREEBSD_PKG')
  subdir('gimpxx-freebsd-pkg-enabler')
endif

if get_option('ENABLE_APPIMAGE')
  subdir('gimpxx-AppImage-enabler')
endif

if get_option('ENABLE_WIX')
  subdir('gimpxx-wix-enabler')
endif

if get_option('ENABLE_DOCKER')
  subdir('gimpxx-dockerers')
endif
