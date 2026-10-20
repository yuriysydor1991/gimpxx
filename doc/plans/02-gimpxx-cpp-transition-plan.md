# gimpxx: the C to C++ transition plan

**Status:** plan for a future implementation. Nothing of it is implemented yet.

**Analyzed on 2026-10-09**, against:

- gimpxx `master` at `b7f8cac066` (2026-10-07), which is GIMP 3.3.1 (the 3.3 development branch), with the `gimp-data` submodule at `fe4ecc0`;
- [cpp-app-template](https://github.com/yuriysydor1991/cpp-app-template) `main` `1d445dbd`, `app` `804f3ff9`, `appMeson` `913961ad`, `applibMeson` `5e35f9e3` and `appGtkmm3` `2c6cffcb`;
- [darknetxx](https://github.com/yuriysydor1991/darknetxx) `f37cf317`, the precedent this plan follows.

The line numbers below are from those revisions. Once they drift, search by the function names.

The plan set:

| Document | Content |
|---|---|
| [01 - Architecture analysis](01-gimpxx-architecture-analysis.md) | What GIMP is made of, from `main()` down to the public libraries. |
| **02 - Transition plan** (this one) | Goal, decisions, target layout, phases, tests, risks. |
| [03 - Top-level layer design](03-gimpxx-top-level-layer-design.md) | The C++ classes of the top-level organization layer and the mapping of every C function onto them. |
| [04 - Lower layers roadmap](04-gimpxx-lower-layers-roadmap.md) | What comes after the top-level layer, layer by layer, down to the plug-ins. |

## Goal

Turn gimpxx into a C++ project the way darknetxx was turned into one: a [cpp-app-template](https://github.com/yuriysydor1991/cpp-app-template) shaped C++ application owns the process from `main()` down, and the inherited GIMP C code sits below it as a component that is replaced layer by layer, from the top to the bottom.

The first deliverable, which this plan covers in full, is the **top-level organization layer**:

1. the repository and build organization of the template (its Meson flavor): the root build, the `meson/` helper modules, the `src/` C++ tree, `doc/`, `misc/`, `scripts/`, `CHANGELOG.md` and `README.md`;
2. the C++ classes which replace GIMP's application glue, the files directly in `app/` (`main.c`, `app.c`, `signals.c`, `errors.c`, `sanity.c`, `language.c`, `unique.c`) and wrap the rest of it (`gimp-version.c`, `gimp-update.c`);
3. the tests, CI and documentation for both.

GIMP's behavior stays the same. Everything below `app/*.c` (from `app/gui` down) is the subject of the [roadmap](04-gimpxx-lower-layers-roadmap.md).

## Verdict

Feasible, with low risk if it is done as a strangler: a C++ `main()` and the template application classes first call the unchanged C code, and then take its responsibilities over one per commit, each commit checked by a CLI parity harness against the original binaries.

The evidence:

- **The glue reaches C++-clean headers.** `main.c` reaches 45 project headers and `app.c` reaches 58, and none of them uses a C++ keyword as an identifier. In the whole top-level include closure the only clash is `GimpDrawablePrivate *private;` (`app/core/gimpdrawable.h:38`), which `errors.c` includes without using it.
- **The interop pattern already exists.** None of the 1,144 `app/` headers has `G_BEGIN_DECLS`, but the existing C++ files show the working pattern of including them inside `extern "C" { ... }` (`app/core/gimp-parallel.cc:36`).
- **The build already mixes the languages.** It is C plus C++ today: `project('gimp', 'c', 'cpp')` with `cpp_std=c++14`, 10 `.cc` files (9,517 lines), and `gimp-3.3` already links with the C++ linker.
- **The top-level API is almost private.**
  - `app.h`, `signals.h`, `errors.h`, `sanity.h`, `language.h` and `unique.h` have no callers outside `app/*.c` (five of them stop the compilation when included from an `app/` subdirectory).
  - Only `gimp_version*()`, `gimp_update_check/refresh()`, `gimp_debug_*()`, `gimp_log*()` and the `GimpCoreApp` interface are called from below. Those keep a C ABI.

The hard parts, each handled by a step below:

1. **Option parsing has side effects.**
   - GLib's `GOptionContext` runs GIMP's own early-configuration hook (`gimp_options_group_parse_hook()`, `app/main.c:649`) and GTK's display initialization (the post-parse hook of `gtk_get_option_group()`, added in `gui_libs_init()`, `app/gui/gui.c:178`), in this order.
   - `--version` exits from a pre-scan, and the callbacks of `--license`, `--dump-gimprc*` and `--dump-pdb-procedures-deprecated` call `exit()` from inside the parser.
2. **The build runs the GIMP it has just built.** `tools/in-build-gimp.py` runs the fresh `gimp-console-3.3` (or `gimp-3.3`) to render the `gimp-data` images and to run the `libgimp` tests (`meson.build:2028-2110`), so every commit must produce a working binary.
3. **Moving the GIMP tree under `src/` trips on root-relative Meson calls.** 97 such calls (`meson.project_source_root()`, `meson.global_*_root()`) in 36 `meson.build` files need edits, and 27 more sit in the `gimp-data` submodule, which is a separate GNOME repository.

## Hard constraints

| # | Constraint | Where it comes from |
|---|---|---|
| C1 | **Behavior parity at every commit** of `gimp-3.3` and `gimp-console-3.3`: options, help text, exit codes, stdout content, config directories. | Scripts parse `--version` and the `--dump-*` output, and the build itself runs the binary. |
| C2 | **The public C ABI and API stay unchanged.** This covers the nine `lib*-3.0` libraries (their `.def` files are checked by `tools/defcheck.py`), `Gimp-3.0.gir` / `GimpUi-3.0.gir`, the plug-in wire protocol `GIMP_PROTOCOL_VERSION 0x0119` and the module ABI `0x0005`. No C++ symbol may leak into them. | Plug-ins in C, Python, JavaScript, Lua, Vala and Script-Fu consume them. |
| C3 | **The GIMP licenses stay:** GPLv3+ for the application and LGPLv3+ for the libraries. The template's `LICENSE` ("Free use software. Replace license with your own.") is replaced as it asks. New files carry the GPLv3+ header. | `COPYING`, `LICENSE` |
| C4 | **`meson.project_version()` stays GIMP's version** (3.3.1). It drives the executable names (`gimp-3.3`), `so_version` 0.300.1, the API name `gimp-3.0`, the gettext domain and the install directories, and the configure step rejects other formats (`meson.build:26-132`). gimpxx gets a version of its own. | root `meson.build` |
| C5 | **`meson.project_name()` stays `gimp`.** It drives `GIMP_PACKAGE`/`PACKAGE_NAME` (`meson.build:1672-1673`), the `gimp/3.0` install subdirectory (1698), the test suite names and the dist tarball name. | root `meson.build` |
| C6 | **Cross-platform behavior is preserved.** Linux has D-Bus single-instance handling. Windows has `WinMain`, `.rc` resources, Dr. MinGW and console attaching. On macOS every C file is compiled as Objective-C (`-ObjC`, `libgimpbase/meson.build:2`), and `language.c`, `gimp-update.c` and `app/gui/*.c` contain Objective-C code. | `app/main.c`, `meson.build:211-224` |
| C7 | **The executables are defined before `gimp_run_env`** (`meson.build:2028-2034`), and the new C++ targets come after the project arguments (`meson.build:1887`) and after `subdir('libgimpbase')`. | Meson ordering rules |

## Decisions

### D1. Build system: Meson, through the template's Meson flavor

**Recommendation:** keep Meson. Take the layout and helper modules from the template's `appMeson` branch, which has the same `src/app`, `src/log` and `src/main.cpp` structure as the CMake `app` branch.

**Why:**
- GIMP's build is Meson end to end: a 2,323-line root file and 182 `meson.build` files (40 of them in `gimp-data`).
- It also carries a lot of Meson-specific machinery:
  - generators: gdbus-codegen, gimp-mkenums, genmarshal, GIR, VAPI, and the PDB perl generator, which writes into the source tree;
  - per-platform packaging under `build/`;
  - the self-hosting image rendering.
- darknetxx went to CMake because Darknet already had a small CMake build. For GIMP a CMake port would be a project of its own with no C++ benefit, and the template supports Meson as a first-class flavor.

**Alternative:** CMake (the template `main`/`app` branches). Rejected for this plan; it can be revisited when no Meson-only generator is left.

### D2. Template flavor: `appMeson` first, `applibMeson` later

**Recommendation:** adopt the application flavor (`appMeson`): `src/main.cpp`, `src/app` (`ApplicationFactory`, `ApplicationContext`, `CommandLineParser`, `CMDParamNames`, `applications/`, `signals-handlers/`) and `src/log`.

**Why:** the top-level layer needs no installable C++ library.

**Later:** the library facade of `applibMeson` (`src/lib/facade`, `src/lib/libmain`, `src/converters`) becomes interesting once a C++ API over GIMP's core is wanted (doc 04). darknetxx went the same way: it first switched to the plain application ("Switching cpp-app-template to regular app version instead of applib"), and only later grew `src/lib`.

### D3. Where the GIMP C tree lives

**Recommendation:** the end state is the template layout, with the whole upstream tree under `src/gimp/` (the darknetxx "hide the original repository under `src/`" step). Do the relocation as its own milestone (Phase 4), **after** the C++ entry layer reaches parity, not before it.

**Why after:**
- The C++ layer's Meson files only use variables GIMP exports (`rootInclude`, `appInclude`, `libapp*`, `gimpconsole_deps`, ...), and its sources include GIMP headers through include directories (`"core/gimp.h"`, `"libgimpbase/gimpbase.h"`). The relocation therefore does not touch them, so the two pieces of work are independent.
- Doing the risky behavior work and the risky path work apart keeps any failure bisectable.
- The relocation has a prerequisite outside this repository: `gimp-data` uses `meson.project_source_root() / 'gimp-data/...'` in 26 places and `meson.global_source_root()` once (`gimp-data/images/logo/meson.build:2`). Under `src/gimp/` these point to the wrong directory, so `gimp-data` needs a fork or an upstream patch first (Phase 4.1).

**Alternative:** relocate first, exactly in the darknetxx order. This is possible, because Phase 4 does not depend on Phases 2 and 3. It costs the `gimp-data` work up front, and makes upstream merges harder while the most upstream-sensitive files (`app/main.c`, `app/app.c`) are being ported.

### D4. C++ standard: C++17 for the whole project

**Recommendation:** change `cpp_std=c++14` to `cpp_std=c++17` in `project()`.

**Why:**
- The template code needs C++17: `inline static` members, `std::filesystem` and `std::atomic<...>::is_always_lock_free`.
- The existing `.cc` files use nothing that C++17 removed (no `register`, dynamic exception specifications, `auto_ptr` or `bind1st`). They use templates and lambdas; only `plug-ins/file-exr/openexr-wrapper.cc` uses a few `std::` containers.

**Fallback:** keep C++14 for the project and set `override_options: ['cpp_std=c++17']` on the new targets only.

### D5. GLib, GObject and GTK from C++: the C API with RAII helpers

**Recommendation:** in the top-level layer, call the GLib/GIO/GTK C API directly, wrapped in small RAII helpers (`src/helpers/glib`):
- `GObjectPtr<T>` (ref/unref);
- `GCharPtr` (`g_free`);
- `GStrvPtr` (`g_strfreev`);
- `GErrorPtr`;
- `GFilePtr`.

No glibmm, giomm or gtkmm here.

**Why:**
- GIMP's own types (`GimpApp`, `GimpCoreApp`, ...) are C GObjects with no C++ bindings.
- The layer is small.
- It keeps the dependency set equal to upstream's.

**Later:** gtkmm-3.0 (the template's `appGtkmm3` branch) is evaluated for the GUI layers in doc 04.

### D6. Command line parsing: the template interface over `GOptionContext`

**Recommendation:** keep the template's `CommandLineParser::parse_args(ctx)` / `CMDParamNames` contract, but implement the parsing with `GOptionContext`.

**Why:**
- Only GLib can parse the GEGL and GTK option groups (`--gegl-*`, `--display`, `--g-fatal-warnings`, ...).
- It produces the translated `--help` and `--help-all` text.
- It runs the post-parse hooks in the order GIMP depends on.

**How (details in doc 03):**
- The "terminal" options are recognized by a pre-scan, without running the hooks and so without GTK initialization: `--version`/`-v`, `--license`, `--help*`, `--dump-gimprc*` and `--dump-pdb-procedures-deprecated`. This is what `main.c` already does for `--version` and `--no-interface` (`app/main.c:840-878`).
- Each terminal option becomes its own `IApplication` instead of an `exit()` inside a parser callback.

### D7. Logging: the template's `DefaultLogger`, with GIMP-safe defaults

**Recommendation:** the C++ layer logs through the template's `LOGE/LOGW/LOGI/LOGD/LOGT` macros. The template defaults change, since the template's `LOG_INIT_DEFAULTS()` writes `<name>-<version>.log` into the current directory and prints INFO to stdout:

- **No log file** unless `--log-file`/`-l` is given. This is the template's own flag.
- **Messages go to stderr only.** stdout carries machine-read output: `--version`, `--dump-gimprc`, batch output.
- **Printed level:** WARNING by default, INFO with `--verbose`.
- **A GLib bridge** (after the template's `appGtkmm3` `GtkLogBridge`) tees the structured GLib log into the log file when one is requested. It never takes over `g_print`/`g_printerr`, which GIMP uses for real output, and it never replaces GIMP's per-domain handlers (`errors.c` routes them to message dialogs).
- **A C shim** (`cpplog4c`, as in darknetxx `src/log/cpplog4c.h`) lets C code log into the same logger later.

### D8. OS signals: the template's component, GIMP's semantics

**Recommendation:** use the template's `src/app/signals-handlers` design (`ISignalsHandler`, the `SignalsHandler` facade, per-category handlers), but populate it with GIMP's categories:

| Category | Signals | Action |
|---|---|---|
| Terminate | SIGHUP, SIGINT, SIGQUIT, SIGTERM | `gimp_terminate()` |
| Fatal | SIGABRT, SIGBUS, SIGSEGV, SIGFPE (not macOS) | `gimp_fatal_error()` |
| Ignored | SIGPIPE | ignored (the template's own `IgnoredSignalsHandler`) |
| Child | SIGCHLD | default action with `SA_RESTART` |
| Win32 | — | the Dr. MinGW exception filter |

**How:** install the handlers with `sigaction` flags (`gimp_signal_private()`), not with `std::signal`, which cannot express `SA_RESTART`.

**Not installed:** the template's Pause, Continue, Reload and User categories, because GIMP has no such semantics.

**Later, opt-in:** graceful stop. SIGINT/SIGTERM raise the context's `stop` flag, and a GLib source turns it into `gimp_exit(gimp, TRUE)`. This is what console background mode already does for SIGINT (`app_quit_on_ctrl_c()`, `app/app.c:613`).

### D9. Binary names, application ID and config directories

**Recommendation:** keep all of them for now:
- executables: `gimp-3.3` and `gimp-console-3.3`;
- application ID: `org.gimp.GIMP`;
- D-Bus name: `org.gimp.GIMP.UI`;
- config directory: `GIMP/3.3`.

**Why:** gimpxx stays a drop-in replacement, and the existing wrapper scripts (`docker-gimpxx.sh run` starts `gimp-<version>`) keep working.

**Later:** a gimpxx-branded, side-by-side installable variant is an open decision (see below). It touches C5, the desktop file, the D-Bus names and `gimpdir`.

### D10. Versions

**Recommendation:**
- The Meson project version stays GIMP's (C4).
- gimpxx gets its own semantic version, `gimpxx_version = '0.1.0'`, declared in `meson/gimpxx-misc-variables-declare/meson.build`. It feeds `project_decls::PROJECT_BUILD_VERSION` (`src/project-global-decls.h.meson.in`), `CHANGELOG.md` and the release tags.

### D11. Code style

**Recommendation:**
- New C++ follows the template's Google-based `misc/.clang-format`, also installed as `src/.clang-format` so that editors pick it up.
- GIMP's C keeps GIMP's GNU-based `.clang-format`, which moves to `src/gimp/.clang-format` on relocation.
- The template's `all-source-files` glob (`meson/template-project-all-source-files`) is restricted to exclude `src/gimp/**`, so the clang-format, clang-tidy and cppcheck targets only see the new code.

### D12. Template defaults that must not go project-wide

**Recommendation:** set the template's `warning_level=3`, `b_pie=true`, hardening and sanitizer options **per new target** (`override_options`), not in `project()`.

**Why:** GIMP builds at the default warning level. `-Wpedantic -Wextra` on 1.27 million lines of C would bury every real warning.

**Also:** the C-only project arguments (`GLIB_VERSION_MIN_REQUIRED/MAX_ALLOWED`, `meson.build:485,491`; `GDK_VERSION_*`, `meson.build:506-507`) are added for `cpp` too, so the C++ code cannot use newer GLib/GTK API than the C code may.

## Target layout

The end state, after the relocation (Phase 4):

```
gimpxx/
├── meson.build                    project('gimp', 'c', 'cpp', version: '3.3.1', ...): the template root
├── meson_options.txt              GIMP's options (lower-case) + the template's options (UPPER_CASE)
├── meson/                         gimpxx-* helper modules of the template's Meson flavor:
│                                  misc-variables-declare, configure-file, hardening, sanitizers,
│                                  clang-format/clang-tidy/cppcheck targets, profilers, dockerers, packagers
├── src/
│   ├── meson.build                subdir('gimp') + the C++ libraries + the gimp-3.3 / gimp-console-3.3 executables
│   ├── main.cpp                   return app::ApplicationFactory::execute(argc, argv);
│   ├── project-global-decls.h.meson.in
│   ├── app/                       namespace app: the template application layer and GIMP's applications
│   │   ├── ApplicationContext, ApplicationFactory, CommandLineParser, CMDParamNames
│   │   ├── applications/          IApplication, Application, ApplicationHelpPrinter, ApplicationVersionPrinter,
│   │   │                          ApplicationLicensePrinter, GimprcDumpApplication, PdbDeprecatedDumpApplication,
│   │   │                          RemoteInstanceApplication
│   │   ├── platform/              IPlatformEnvironment + posix/, win32/, macos/
│   │   └── signals-handlers/      the template component with GIMP's categories
│   ├── gimp-adaptor/              namespace gimp_adaptor: the only C++ code that includes GIMP app/ headers
│   ├── converters/                namespace converters: ApplicationContext -> GimpRunContext
│   ├── log/                       the template logger, the GLib bridge, the cpplog4c C shim
│   ├── helpers/glib/              RAII for GLib/GObject
│   ├── tests/mocks/               the shared mocks (template convention)
│   └── gimp/                      the upstream GIMP tree: app/, libgimp*/, plug-ins/, modules/, pdb/, data/,
│                                  po*/, gimp-data/ (submodule), build/, tools/, docs/, devel-docs/, ...
├── doc/                           plans/ (these documents), diagrams/, sections/en_US|uk_UA, Doxyfile.meson.in
├── misc/                          .clang-format, .clang-tidy, .cppcheck-suppress, Dockerfiles/, packagers/
├── scripts/                       build/, docker/, packages/, parity/
├── .github/workflows/ci.yml
└── README.md, CHANGELOG.md, COPYING, LICENSE
```

Before the relocation, the same tree exists with the GIMP directories still at the repository root. The root `meson.build` is GIMP's, extended with `subdir('meson')` and `subdir('src')`.

## How the pieces fit at run time (end state)

```
main()                                                src/main.cpp
└─ app::ApplicationFactory::execute(argc, argv)
   ├─ IPlatformEnvironment::preinit()                 Win32 env before GLib starts, -psn_ args, mingw argv, console
   ├─ logger init (+ GLib bridge)                     quiet defaults, --log-file
   ├─ ApplicationContext(argc, argv)
   ├─ ProcessBootstrap::prepare_crash_reporting()     gimp_backtrace_init, the crash log path
   ├─ ISignalsHandler::install(ctx)                   terminate / fatal / ignored / child (+ Win32 filter)
   ├─ IPlatformEnvironment::process_setup()           Win32 mutex, DLL directory, DEP, AppUserModelID
   ├─ ProcessBootstrap::early_init()                  gimp_env_init, gimp_log_init, i18n, names, argv copy
   ├─ CommandLineParser::parse_args(ctx)              pre-scan; GOptionContext: main entries, "gimp" group hook
   │                                                  (EarlyConfiguration -> language), GEGL group, GTK group
   ├─ create_application(ctx)                         help | version | license | gimprc dump | PDB dump |
   │                                                  RemoteInstanceApplication(Application) | Application
   └─ IApplication::run(ctx)
      └─ Application::run(ctx)
         ├─ gimp_adaptor::SanityChecker::check_early()
         ├─ converters::ApplicationContext2GimpRunContext
         └─ gimp_adaptor::GimpController::perform(runCtx)          the old app_run()
            ├─ gimp_new(); runner->create_core_app()               GimpApp | GimpConsoleApp
            ├─ user install, gimp_load_config(), ErrorsHandler::init(), check_late(), gimp_gegl_init()
            ├─ g_application_run()  ->  activate:                  the old app_activate_callback()
            │    runner->init_gui(), gimp_initialize(), gimp_restore(), recovery, update check,
            │    open files, gimp_batch_run(), quit or hold
            └─ teardown: gimp_gegl_exit(), ErrorsHandler::exit(), unref, gimp_debug_instances(), gegl_exit()
```

## Phases

Each step below is one commit. The commit subjects follow the darknetxx history style. **Every commit must:**
- build in the Docker environment (Debian trixie, babl and GEGL from source, as `/home/ubuntu/gimpxx/Dockerfile` does today);
- pass the 21 existing Meson tests;
- pass the CLI parity harness from Phase 0.

### Phase 0 — Baseline and safety net (no product change)

| Step | Commit | Content |
|---|---|---|
| 0.1 | `chore: record the upstream base and the upstream remote` | Tag `gimp-upstream/3.3.1-b7f8cac066` and add the `upstream` remote (gitlab.gnome.org/GNOME/gimp, or its GitHub mirror). |
| 0.2 | `chore: introduce the .gitignore` | GIMP has no `.gitignore` at all. Start from the template's, plus `_build*`, `_install*` and `.GIMP3-build-config-*`. |
| 0.3 | `test: add the command line parity harness` | `scripts/parity/gimp-cli-parity.py` runs two builds (the reference built from the upstream tag, and the candidate) over a matrix of command lines, then compares exit codes and stdout/stderr after normalization: library versions, paths, CPU counts, memory sizes. Also normalized golden files under `scripts/parity/golden/` for CI. The matrix is in doc 03, "Parity matrix". |
| 0.4 | `test: add the startup smoke runs` | A console batch run (`gimp-console-3.3 -idf --quit --batch-interpreter=plug-in-script-fu-eval -b '(gimp-message "ok")'`), plus the GUI binary in no-interface mode and, when `xvfb-run` exists, with an interface. |

**Acceptance:** the harness runs green, reference against reference.

### Phase 1 — Template infrastructure, grafted in place (no product change)

| Step | Commit | Content |
|---|---|---|
| 1.1 | `build: switch the C++ standard to C++17` | `cpp_std=c++17`; the `GLIB_VERSION_*` / `GDK_VERSION_*` macros added for `cpp` too. |
| 1.2 | `build: copy the cpp-app-template Meson infrastructure` | From `appMeson`/`applibMeson`: `meson/` (renamed `gimpxx-*`), `misc/` (`.clang-format`, `.clang-tidy`, `.cppcheck-suppress`), `src/.clang-format`, `scripts/` adapted to Meson and GIMP's dependencies, the `doc/` skeleton (`Doxyfile.meson.in`, `diagrams/`), and `CHANGELOG.md`. |
| 1.3 | `build: merge the template options into meson_options.txt` | The UPPER_CASE template options: `ENABLE_UNIT_TESTS`, `ENABLE_COMPONENT_TESTS`, `ENABLE_CLANGFORMAT`, `ENABLE_CLANG_TIDY`, `ENABLE_CPPCHECK`, `ENABLE_SANITIZERS`, `MAX_LOG_LEVEL`, ... They cannot collide with GIMP's lower-case ones. `subdir('meson')` goes after `meson.build:1888` and after `subdir('libgimpbase')`. |
| 1.4 | `feat: add the src skeleton, the logger and the GLib RAII helpers` | `src/meson.build`, `src/project-global-decls.h.meson.in` (the `gimpxx_version`), `src/log` (template, with the gimpxx defaults of D7), `src/helpers/glib`, and their GTest unit tests (the template's `UTEST_*`/`CTEST_*` layout). |
| 1.5 | `docs: add the gimpxx README.md and keep the GIMP README` | GitHub shows `README.md` first; `README` stays as GIMP's. Uses the template README structure and links this plan. |

**Acceptance:** the installed file list (`meson introspect --installed`) is identical to the baseline; the template targets (`clang-format`, `cppcheck`, unit tests) see only `src/`.

### Phase 2 — The C++ entry point (thin bridge, no product change)

| Step | Commit | Content |
|---|---|---|
| 2.1 | `refactor: rename the C main into gimp_c_main` | `app/main.c`: `main()` becomes `int gimp_c_main (int argc, char **argv)`, declared in a new `app/gimp-c-main.h` with `G_BEGIN_DECLS`. `WinMain` (`app/main.c:1088`) calls it. |
| 2.2 | `feat: introduce the C++ main and the template application classes` | `src/main.cpp`, `src/app` (template `ApplicationFactory`, `ApplicationContext`, `IApplication`, `Application`), and `gimp_adaptor::LegacyEntry::run(argc, argv)`, which calls `gimp_c_main()`. The factory only initializes the logger. Signals and parsing stay in C for now. |
| 2.3 | `build: build gimp-3.3 and gimp-console-3.3 of src/main.cpp` | `main.c` is compiled into two static libraries instead, mirroring today's compile units:<br>• the GUI one, with `libapp_c_args`;<br>• the console one, with `main.c` + `libapp_sources` + `-DGIMP_CONSOLE_COMPILATION`.<br>The `executable()` calls move from `app/meson.build:188-250` into `src/meson.build` with the same names, `.rc` files, `win_subsystem` values and install symlinks. `subdir('src')` goes between `subdir('app')` and `gimp_run_env`. |

**Acceptance:** parity is 100%, and the in-build image rendering and the `libgimp` tests pass. From this commit on, `main()` is C++.

### Phase 3 — Strangling the C glue (one responsibility per commit)

The order follows the run-time order of `main()`. Each step moves code from `gimp_c_main()` (later `gimp_c_run()`) into the classes of doc 03, and is verified by the parity harness plus new unit tests.

| Step | Commit | Moves | Into |
|---|---|---|---|
| 3.1 | `feat: print the version and the license from C++` | the `--version`/`-v`/`--license` pre-scan, `gimp_show_version_and_exit()`, `gimp_show_license_and_exit()` (`app/main.c:1282-1300`) | `ApplicationVersionPrinter`, `ApplicationLicensePrinter`, `CommandLineParser` pre-scan |
| 3.2 | `feat: parse the command line in the CommandLineParser` | the option statics and `main_entries` (`app/main.c:129-328`), the `GOptionContext` setup and parse (884-920), the option callbacks (1153-1203) | `CMDParamNames`, `CommandLineParser` (over `GOptionContext`), `ApplicationContext`. The C side receives a filled `GimpStartupOptions` struct through `gimp_c_run()`. |
| 3.3 | `feat: move the early configuration into the adaptor` | `gimp_early_configuration()` and its hook (`app/main.c:578-659`); `app_libs_init()` (`app/app.c:116-151`) | `gimp_adaptor::EarlyConfiguration`, `gimp_adaptor::LibraryOptionGroups` |
| 3.4 | `feat: move the platform and process bootstrap into C++` | `gimp_windows_setenv()`, `gimp_macos_setenv()`, the `-psn_` filter, mingw argv, console attach, the Win32 mutex/DLL dir/DEP/AUMID block, malloc/env/log/i18n init, names, argv duplication (`app/main.c:330-530, 682-838, 1080-1148, 1302-1375`) | `app::platform::*PlatformEnvironment`, `gimp_adaptor::ProcessBootstrap`. The macOS Objective-C (`setlocale_lc_numeric()`, `app/main.c:532-572`) stays in a `.m`/`.c` helper behind a C function. |
| 3.5 | `feat: handle the OS signals in the signals-handlers component` | `signals.c` | `TerminateSignalsHandler`, `FatalSignalsHandler`, `IgnoredSignalsHandler`, `ChildSignalsHandler`, `win32/Win32ExceptionHandler` |
| 3.6 | `feat: the help printer and the dump applications` | GOption help; `--dump-gimprc*`, `--dump-pdb-procedures-deprecated` (`app/main.c:1205-1280`) | `ApplicationHelpPrinter`, `GimprcDumpApplication`, `PdbDeprecatedDumpApplication`, `gimp_adaptor::GimprcDumper`/`PdbDeprecatedDumper` |
| 3.7 | `feat: the remote instance application` | `unique.c`; the single-instance block (`app/main.c:976-1029`) | `RemoteInstanceApplication` (a decorator over `Application`), `gimp_adaptor::RemoteInstance` |
| 3.8 | `feat: the sanity checker` | `sanity.c` | `gimp_adaptor::SanityChecker` |
| 3.9 | `feat: the language initializer` | `language.c` (the Win32 LANGID table becomes a constexpr table; the macOS part stays Objective-C) | `gimp_adaptor::LanguageInitializer` |
| 3.10 | `feat: run GIMP through the GimpController` | `app_run()`, `app_activate_callback()`, `app_exit_after_callback()`, `app_quit_on_ctrl_c()`, `app_abort()`, `app_exit()` (`app/app.c:153-623`). `gimp_c_run()` and `GimpStartupOptions` disappear. | `gimp_adaptor::GimpController`, `GimpRunContext`, `runners::ConsoleCoreAppRunner` / `GuiCoreAppRunner`, `converters::ApplicationContext2GimpRunContext` |
| 3.11 | `feat: the errors handler` | `errors.c` | `gimp_adaptor::ErrorsHandler` |
| 3.12 | `feat: the C++ facades of the version and update checks` | `gimp-version.c` and `gimp-update.c` keep their C implementation and their C API (callers in `app/widgets` and `app/dialogs`); C++ calls go through the facades | `gimp_adaptor::VersionInfo`, `gimp_adaptor::UpdateChecker` |
| 3.13 | `feat: the GLib log bridge and the --log-file parameter` | none (new) | `src/log/glib-bridge`, `cpplog4c` |
| 3.14 | `refactor: erase the ported C glue` | deletes `main.c`, `app.c`, `signals.c`, `errors.c`, `sanity.c`, `language.c`, `unique.c` and their headers (4,896 lines) | `libapp` keeps `gimpcoreapp.c`, `gimpconsoleapp.c`, `gimp-log.c`, `gimp-debug.c`, `gimp-update.c`, `gimp-version.c` |

**Acceptance:** parity is 100% (the intended differences of doc 03 are listed in the golden files); every new class has unit tests; the console and GUI smoke runs pass; a SIGTERM, a SIGSEGV and a SIGPIPE scenario behave as before.

### Phase 4 — Relocation of the upstream tree to `src/gimp`

| Step | Commit | Content |
|---|---|---|
| 4.1 | (in the `gimp-data` fork) `build: address the logo images relative to gimp-data` | Replace the 26 `meson.project_source_root() / 'gimp-data/...'` with a variable set in `gimp-data/meson.build` (`gimp_data_root = meson.current_source_dir()`), and `meson.global_source_root()` in `images/logo/meson.build:2` with the GIMP source root variable. This is upstreamable to GNOME/gimp-data. Point `.gitmodules` at the fork until upstream takes it. |
| 4.2 | `build: replace the root relative Meson calls with the gimp_source_root variables` | Still in place. `gimp_source_root = meson.current_source_dir()` and `gimp_build_root = meson.current_build_dir()` at the top of GIMP's root file, then every `meson.project_source_root()`/`global_*` call of the GIMP tree is replaced. The full list is in doc 01, "Root-relative build dependencies": root `meson.build` 1857, 1943, 2001-2076; `pdb/meson.build:75-79, 91-93, 122, 147, 167`; `app/{core,config,display,gegl,operations,paint,plug-in,text,tools,widgets}/meson.build:10`; `libgimpbase/meson.build:20,41`; `libgimpconfig:11`; `libgimpwidgets:10`; `app/dialogs/meson.build:2-3,8-9,68`; `app/tests/meson.build:63-65,92-96`; `libgimp/meson.build:771`; `tools/meson.build:43`; `po-tips/meson.build:5`; `po-windows-installer/meson.build:4-5`; `devel-docs/reference/gimp-ui/meson.build:27`. No behavior change. |
| 4.3 | `refactor: hide the original GIMP tree under src/gimp` | `git mv` of every upstream path into `src/gimp/`. The old root `meson.build` becomes `src/gimp/meson.build` without `project()`; the new root is the template's (`project()` with GIMP's name, version and default options, `subdir('meson')`, `subdir('src')`). `meson_options.txt` stays at the root. `.gitmodules` gets the `src/gimp/gimp-data` path. `COPYING` and `LICENSE` are copied back to the root. |
| 4.4 | `build: adapt the packaging and the scripts to src/gimp` | The `src/gimp/build/{linux,windows,macos}` scripts, flatpak/snap manifests, the Windows installer, `scripts/`, and the wrapper scripts and Dockerfile, which move into `misc/Dockerfiles` and `scripts/docker` (they hard-code `SRC_DIR=.../gimpxx`). |
| 4.5 | `ci: forbid the root relative Meson calls under src/gimp` | A lint that greps for `project_source_root()`/`global_*_root()` under `src/gimp`, so that upstream merges cannot reintroduce them. |

**Acceptance:** identical installed files and the same parity and test results as at the end of Phase 3.

**Upstream merges after Phase 4:** `git merge -X subtree=src/gimp upstream/master`, with `merge.renameLimit` raised, a fixed cadence (for example each upstream 3.3.x tag), and the 4.5 lint.

### Phase 5 — Documentation, CI and the first release

| Step | Content |
|---|---|
| 5.1 | `doc/sections/en_US` and `uk_UA`, adapted from the template and darknetxx: requirements (GIMP's dependency list), project structure, components and architecture (4-7-x, with the doc 03 diagrams), handling the OS signals (4-8), build, run, install. `doc/README.uk_UA.md`. Doxygen with PlantUML. |
| 5.2 | `.github/workflows/ci.yml`, from the template's: the Debian trixie image (babl and GEGL from source), debug build plus Meson tests plus unit tests plus parity, a clang-format check on the new C++, and optional cppcheck/clang-tidy and sanitizer jobs. Upstream's `.gitlab-ci.yml` stays untouched for reference. |
| 5.3 | `CHANGELOG.md` (Keep a Changelog, as in the template and darknetxx), version `0.1.0`, tag `gimpxx-0.1.0`. |

## Testing strategy

| Level | What | Where |
|---|---|---|
| Parity (differential) | The candidate binaries against the upstream ones over the doc 03 matrix: exit code, stdout, stderr, normalized. The single most important safety net. | `scripts/parity/`, a Meson test suite `gimpxx-parity` |
| Unit (GTest/GMock) | Every new class. The template conventions apply: `tests/unit/<Class>/UTEST_<Class>.cpp`, mocks under `tests/mocks/<Class>/src/...` that shadow the real headers through include order. | next to each component |
| Component | Classes that need the real GLib or GIMP pieces: `CommandLineParser` over a real `GOptionContext`, `SanityChecker` against the real libraries, the signal handlers in a forked child. | `tests/component/<Class>/CTEST_<Class>.cpp` |
| GIMP's own | The 21 Meson tests (app: core, gimpidtable, xcf, save-and-export, app-config; desktop: 2; libgimp: 7 topics, each in Python and C), plus the in-build image rendering. | unchanged |
| Smoke | Console batch run; GUI binary with `-i`; GUI with an interface under `xvfb-run` when available. | Phase 0.4 |
| Sanitizers | The template's `ENABLE_SANITIZERS` on the new targets. | CI optional job |

## Risks

| Risk | Impact | Mitigation |
|---|---|---|
| The GOption post-parse hooks run in a different order, or GTK is initialized for a terminal option. | Wrong language, or `--dump-gimprc` failing without a display. | The pre-scan of terminal options, the group order kept, and the parity matrix run without `DISPLAY`. |
| A broken entry layer breaks the build itself (in-build GIMP). | Red builds. | The thin bridge first (Phase 2), one responsibility per commit, and parity on every commit. |
| Windows and macOS are not exercised in the gimpxx environment. | Platform regressions. | Platform code moved 1:1 into `platform/win32` and `platform/macos`, Objective-C kept in C/ObjC files, a CI job for MSYS2 later. |
| C++ exceptions crossing GLib callbacks; static initialization order. | Crashes. | Every C callback is a `noexcept` static thunk that catches everything; no namespace-scope objects with GLib state. |
| The template's signals and logger defaults change behavior. | Ctrl-C handling, files in the current directory, stdout pollution. | GIMP's signal semantics (D8) and quiet logging defaults (D7); deviations listed explicitly. |
| `gimp-data` blocks the relocation. | Phase 4 blocked. | The fork/patch (4.1); Phase 4 is independent of everything else. |
| Upstream drift after the relocation. | Painful merges. | Subtree merges, a cadence, the lint. |
| C++ creeping into the public libraries. | `check-def-files` failures, broken GIR, plug-in ABI breaks. | No C++ in `libgimp*` within this plan; doc 04 defines the rules (hidden visibility, `extern "C"` API, `.def` files unchanged). |
| Licensing confusion through the template files. | Legal. | GPLv3+ headers on the new files, `COPYING`/`LICENSE` at the root, the template `LICENSE` replaced. |

## Open decisions

1. **Relocation timing:** after parity (recommended), or first, as darknetxx did.
2. **gimp-data:** fork it and send the patch upstream (recommended), or keep the tree at the root.
3. **Branding:** drop-in names and IDs (recommended for now), or a side-by-side installable gimpxx. The latter means renaming `project()`, `GIMP_PACKAGE`, `gimpdir`, the desktop ID and the D-Bus names.
4. **`--version` output:** add the gimpxx build line only with `--verbose` (recommended, so the default output stays byte-identical), or always.
5. **SIGINT/SIGTERM in the GUI:** keep the immediate terminate (recommended at first), or adopt the template's cooperative stop (graceful `gimp_exit`).
6. **Logging:** confirm the defaults (no file, stderr, WARNING) and the template's `--log-file`/`-l` flag.
7. **Library facade:** when the `applibMeson` facade (`libgimpxx`) enters (doc 04).
8. **gtkmm-3.0:** whether the GUI layers adopt it (doc 04).
9. **Upstream:** the sync cadence and the eventual cut-off point.
10. **Platforms:** whether the gimpxx CI covers Windows (MSYS2) and macOS, or Linux only at first.

## Rough size

| Phase | Commits | New or changed | Removed C | Effort (one developer) |
|---|---|---|---|---|
| 0 | 4 | about 500 lines of Python plus golden files | 0 | 2-3 days |
| 1 | 5 | about 60 template files, about 300 lines of Meson, about 300 lines of C++ helpers plus tests | 0 | 4-6 days |
| 2 | 3 | the template application skeleton (about 2,600 lines of C++, about 3,000 lines of tests in the template's own measure), about 150 lines of Meson | 0 (`main()` renamed) | 3-5 days |
| 3 | 14 | about 7,000-9,000 lines of C++ plus about 5,000-7,000 lines of tests | 4,896 | 4-7 weeks |
| 4 | 5 | 97 root-relative calls in 36 `meson.build` files plus the packaging scripts; a 27-line `gimp-data` patch | 0 | 3-5 days |
| 5 | 3 | about 40 adapted documentation sections, the CI workflow | 0 | 1-2 weeks |
