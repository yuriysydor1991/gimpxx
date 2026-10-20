**The porting to C++ try for the GIMP image editor**

Original repository taken from [https://gitlab.gnome.org/GNOME/gimp](https://gitlab.gnome.org/GNOME/gimp), at its latest stable release `GIMP_3_2_6`.

Some parts of the repository taken from the C++ template project located at [https://github.com/yuriysydor1991/cpp-app-template.git](https://github.com/yuriysydor1991/cpp-app-template.git) (the `appMeson` flavor), the way [darknetxx](https://github.com/yuriysydor1991/darknetxx.git) takes them.

**Work in progress. Only the start-up of GIMP is C++ so far**

The very same text is also available in Ukrainian under [doc/README.uk_UA.md](/doc/README.uk_UA.md).

# Repository layout

| Path | Content |
|---|---|
| [`meson.build`](/meson.build), [`meson_options.txt`](/meson_options.txt) | The root of the build. The GIMP options (lower case) come first, the template ones (upper case) follow them |
| [`meson`](/meson) | The template Meson modules (`gimpxx-project-*`) by category, as the CMake ones of the template: `compile-options` (hardening, sanitizers), `enablers` (code checks, libraries), `profilers`, `packagers`, `dockerers` |
| [`src/main.cpp`](/src/main.cpp), [`src/app`](/src/app), [`src/log`](/src/log) | The template C++ application layer, which starts GIMP |
| [`src/gimp-controller`](/src/gimp-controller) | The `gimp_controller::GIMPController` and its platform classes, the former `main.c` of GIMP |
| [`src/gimp`](/src/gimp) | The original GIMP tree with the [`gimp-data`](/src/gimp/gimp-data) submodule. Its [`README`](/src/gimp/README) is the GIMP one |
| [`doc`](/doc) | The documentation sections, the diagrams and the Doxygen configuration |
| [`misc`](/misc), [`scripts`](/scripts) | The code style and the code checks configurations, the packaging templates and the helper scripts |

The `gimp-data` link of the root points to `src/gimp/gimp-data`, since the gimp-data build files address their images relative to the top-level project.

# Names

The Meson project is `gimpxx`. The user interface names it **GIMPxx, based on GIMP**: the splash screen, the window titles, the welcome and the about dialogs, the desktop entry and the `--version` and `--help` output.

The executables (`gimp-3.2`, `gimp-console-3.2`), the install and the configuration directories (`share/gimp/3.0`, `~/.config/GIMP/3.2`) and the application ID (`org.gimp.GIMP`) stay the GIMP ones, so gimpxx shares the plug-ins, the data and the settings of GIMP.

# The start-up of GIMP

The [`src/main.cpp`](/src/main.cpp) calls the `app::ApplicationFactory::execute`. The factory creates the `gimp_controller::GIMPController` of the current platform, the former `main.c` of GIMP ([`src/gimp-controller`](/src/gimp-controller)), and runs its start-up steps in the original order of the `main()` of GIMP:

1. `GIMPController::init_platform` - the command line the OS gave, the platform environment and the console window;
1. `GimpSignalsHandler` - the GIMP backtraces and signal handlers, through the `GIMPController::init_signals`;
1. `GIMPController::init_process` - the platform process set-up, the GLib and the GIMP environment, the GIMP logging, the translations and the arguments copy, which the `ApplicationContext` owns;
1. `CommandLineParser` - the scan for the verbose, the no interface and the version options, then the parsing of the GIMP options the controller owns;
1. `ApplicationVersionPrinter` or `Application` - the version output, or the `GIMPController::run`: the remote GIMP instance, the sanity check and the run of GIMP.

The platform specifics of the former `main.c` are the virtual methods of the `GIMPController`, which the `LinuxGIMPController`, the `MacOSGIMPController` and the `WindowsGIMPController` implement; Meson builds the one of the current platform. Each GIMP executable gets its own compilation of the controller, the console one with the `GIMP_CONSOLE_COMPILATION` macro, as it got `main.c`. The controller reaches the GIMP C code through the [`gimp-c-includes.h`](/src/gimp-controller/gimp-c-includes.h) only, which the unit tests replace with the mocks of the GIMP C functions. The C++ code logs the warnings and the errors only, into the standard error stream, and creates no log file. The template `SignalsHandler` component is in place, yet not installed, since the GIMP signal handlers keep their own semantics so far.

# Building

Clone the repository together with the `gimp-data` submodule:

```
git clone --recurse-submodules https://github.com/yuriysydor1991/gimpxx.git
```

On the Ubuntu 26.04 the [`scripts/packages/install-ubuntu-26-04.sh`](/scripts/packages/install-ubuntu-26-04.sh) installs the build dependencies. The build needs a locale other than C or POSIX, the `en_US.UTF-8` one for example. Then:

```
meson setup build
meson compile -C build
./build/src/gimp/app/gimp-console-3.2 --version
```

The C++ unit and component tests are enabled with the `-DENABLE_UNIT_TESTS=true -DENABLE_COMPONENT_TESTS=true` options and run with the `meson test -C build --suite gimpxx-cpp`.

# Documentation contents

**Document is under the refinement**

1. [Cloning the C++ template project](/doc/sections/en_US/1-cloning-the-cxx-template-project.md)
1. [Forking and replacing the origin](/doc/sections/en_US/2-forking-and-replacing-the-origin.md)
1. [Requirements](/doc/sections/en_US/3-requirements.md)
    1. [Required tools for the GNU/Linux based OS](/doc/sections/en_US/3-1-required-tools-for-the-GNU-Linux-based-OS.md)
    1. [Required tools for the MS Windows based OS](/doc/sections/en_US/3-2-required-tools-for-the-MS-Windows-based-OS.md)
    1. [Optional for the tests](/doc/sections/en_US/3-3-optional-for-the-tests.md)
    1. [Optional for the documentation](/doc/sections/en_US/3-4-optional-for-the-documentation.md)
    1. [Optional for the code formatting](/doc/sections/en_US/3-5-optional-for-the-code-formatting.md)
    1. [Optional for the code analyzer (cppcheck)](/doc/sections/en_US/3-6-optional-for-the-code-analyzer-cppcheck.md)
    1. [Optional for the code analyzer with clang-tidy](/doc/sections/en_US/3-7-optional-for-the-code-analyzer-with-clang-tidy.md)
    1. [Optional for the memory check with Valgrind](/doc/sections/en_US/3-8-optional-for-the-memory-checkwith-Valgrind.md)
    1. [Optional for the flatpak packager](/doc/sections/en_US/3-9-optional-for-the-flatpak-packager.md)
    1. [Optional for the Docker container runs](/doc/sections/en_US/3-10-optional-for-docker-container-runs.md)
    1. [Optional for the libxml2 integration](/doc/sections/en_US/3-11-optional-for-the-libxml2-integration.md)
    1. [Optional for the OpenCV integration](/doc/sections/en_US/3-12-optional-for-the-OpenCV-integration.md)
    1. [Optional for the snap packager](/doc/sections/en_US/3-13-optional-for-the-snap-packager.md)
    1. [Optional for the rpm packager](/doc/sections/en_US/3-14-optional-for-the-rpm-packager.md)
    1. [Optional for the WIX packager](/doc/sections/en_US/3-15-optional-for-the-WIX-packager.md)
    1. [Optional for the libcurl integration](/doc/sections/en_US/3-16-optional-for-the-libcurl-integration.md)
    1. [Optional for the nlohmann JSON integration](/doc/sections/en_US/3-17-optional-for-the-nlohmann-json-integration.md)
    1. [Optional for the AppImage packager](/doc/sections/en_US/3-18-optional-for-the-AppImage-packager.md)
1. [Project structure](/doc/sections/en_US/4-project-structure.md)
    1. [Project diagrams](/doc/sections/en_US/4-0-project-diagrams.md)
    1. [Implement code straight away!](/doc/sections/en_US/4-1-implement-code-straight-away.md)
    1. [Changing the project and executable name](/doc/sections/en_US/4-2-changing-the-project-and-executable-name.md)
    1. [Introducing custom command line parameters](/doc/sections/en_US/4-11-introduction-of-the-custom-command-line-parameters.md)
    1. [Implement your own IApplication descendants](/doc/sections/en_US/4-7-implementing-your-onw-IApplication-descendants.md)
    1. [Handling the OS signals](/doc/sections/en_US/4-8-handling-the-OS-signals.md)
    1. [Version tracking and other project parameters](/doc/sections/en_US/4-3-version-tracking-and-other-project-parameters.md)
    1. [Minimal possible versions](/doc/sections/en_US/4-6-minimal-possible-versions.md)
    1. [Project tests](/doc/sections/en_US/4-4-project-tests.md)
        1. [Google Test](/doc/sections/en_US/4-4-1-google-test.md)
    1. [Extensions](/doc/sections/en_US/4-5-extensions.md)
1. [Project build](/doc/sections/en_US/5-project-build.md)
    1. [IDE build](/doc/sections/en_US/5-1-IDE-build.md)
    1. [Command line build](/doc/sections/en_US/5-2-command-line-build.md)
    1. Enabling testing
        1. [Enabling unit testing](/doc/sections/en_US/5-3-1-enabling-unit-testing.md)
        1. [Enabling component testing](/doc/sections/en_US/5-3-3-enabling-component-testing.md)
    1. [Documentation build](/doc/sections/en_US/5-4-documentation-build.md)
    1. [Configuring the documentation install support](/doc/sections/en_US/5-5-configuring-the-documentation-install-support.md)
    1. [Meson extra features](/doc/sections/en_US/5-19-meson-extra-features.md)
    1. Code quality & sanitizers
        1. [Enabling and performing code formatting target](/doc/sections/en_US/5-6-enabling-and-performing-code-formatting-target.md)
        1. [Enabling the static code analyzer target with cppcheck](/doc/sections/en_US/5-7-enabling-the-static-code-analyzer-target-with-cppcheck.md)
        1. [Enabling the static code analyzer with clang-tidy](/doc/sections/en_US/5-8-enabling-static-code-analyzer-with-clang-tidy.md)
        1. [Enabling the dynamic memory check target with valgrind](/doc/sections/en_US/5-9-enabling-the-dynamic-memory-check-target-with-valgrind.md)
        1. [Enabling sanitizers](/doc/sections/en_US/5-13-enabling-sanitizers.md)
    1. Profilers
        1. [Enabling gprof profiler analysis](/doc/sections/en_US/5-15-enabling-gprof-profiler-analysis.md)
        1. [Enabling vagrind's callgrind profiler analysis](/doc/sections/en_US/5-16-enabling-valgrinds-callgrind-profiler-analysis.md)
    1. Containers & CI
        1. [Enabling the Docker container build and run](/doc/sections/en_US/5-12-enabling-the-docker-container-build-and-run.md)
        1. [Running the Jenkins pipeline](/doc/sections/en_US/5-17-enabling-Jenkins-pipeline-inside-Docker-container.md)
        1. [The GitHub Actions CI pipeline](/doc/sections/en_US/5-29-the-GitHub-Actions-CI-pipeline.md)
        1. [The GitLab CI/CD pipeline](/doc/sections/en_US/5-30-the-GitLab-CI-pipeline.md)
    1. Packagers
        1. [Enabling the flatpak package generation support](/doc/sections/en_US/5-11-enabling-the-flatpak-package-generation-support.md)
        1. [Enabling the snap package generation support](/doc/sections/en_US/5-22-enabling-snap-package-generation.md)
        1. [Enabling the DEB package generation support](/doc/sections/en_US/5-23-enabling-deb-package-generation.md)
        1. [Enabling the RPM package generation support](/doc/sections/en_US/5-24-enabling-rpm-package-generation.md)
        1. [Enabling the FreeBSD pkg package generation support](/doc/sections/en_US/5-25-enabling-freebsd-pkg-package-generation.md)
        1. [Enabling the AppImage bundle generation support](/doc/sections/en_US/5-26-enabling-the-AppImage-bundle-generation.md)
        1. [Enabling the WIX MSI installer generation support](/doc/sections/en_US/5-27-enabling-the-WIX-MSI-installer-generation.md)
    1. Libraries
        1. [Enabling the libcurl](/doc/sections/en_US/5-14-enabling-libcurl.md)
        1. [Enabling the nlohmann JSON library](/doc/sections/en_US/5-18-enabling-the-nlohmann-json-library.md)
        1. [Enabling the libxml2](/doc/sections/en_US/5-20-enabling-libxml2.md)
        1. [Enabling the OpenCV](/doc/sections/en_US/5-21-enabling-OpenCV.md)
    1. [The binary hardening options](/doc/sections/en_US/5-31-the-binary-hardening-options.md)
    1. [Project maintainer options](/doc/sections/en_US/5-28-project-maintainer-options.md)
1. [Run the executable](/doc/sections/en_US/6-run-the-executable.md)
    1. [IDE run](/doc/sections/en_US/6-1-IDE-run.md)
    1. Command line run
        1. [Searching for the generated executable](/doc/sections/en_US/6-2-1-searching-for-the-generated-executable.md)
        1. [Starting the generated executable](/doc/sections/en_US/6-2-2-starting-the-generated-executable.md)
    1. Tests run
        1. [Run tests by the meson test](/doc/sections/en_US/6-3-1-run-tests-by-the-ctest.md)
        1. [Manual tests run](/doc/sections/en_US/6-3-2-manual-tests-run.md)
1. [Installing](/doc/sections/en_US/7-installing.md)
