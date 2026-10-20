# gimpxx top-level layer: the C++ design

**Status:** design for a future implementation. Nothing of it is implemented yet. It details Phases 2 and 3 of the [transition plan](02-gimpxx-cpp-transition-plan.md).

**Analyzed on 2026-10-09**, against gimpxx `master` `b7f8cac066` and cpp-app-template `app` `804f3ff9` / `appMeson` `913961ad`. The line numbers drift; search by the function names.

This document names every class of the C++ top-level layer and maps every function of GIMP's application glue onto one of them. That glue is `app/main.c`, `app/app.c`, `app/signals.c`, `app/errors.c`, `app/sanity.c`, `app/language.c` and `app/unique.c`, plus `app/gimp-version.c` and `app/gimp-update.c`, which stay C behind facades. The PlantUML sources of the diagrams are in [`doc/diagrams`](../diagrams).

## 1. Principles

1. **The template first.** cpp-app-template's classes, names, file layout and conventions stay as they are:
   - the `app` namespace;
   - `IApplication`, `ApplicationFactory`, `ApplicationContext`, `CommandLineParser`, `CMDParamNames`;
   - the `signals-handlers` component and the `log` component;
   - `project_decls`;
   - the `tests/unit/<Class>/UTEST_<Class>.cpp` and `tests/mocks/<Class>/src/...` layout.

   GIMP's specifics arrive as *new* classes next to them, the way darknetxx added `GuiApplication`, `ApplicationBinaryName.h` and the `DARKNETXX_CONVERTER_ONLY` variant, not by bending the template ones.

2. **One include boundary.** Only `src/gimp-adaptor` includes GIMP's `app/` headers, always through its `gimp-c-includes.h`, which wraps them in `extern "C"`. `src/app` sees:
   - GLib/GIO;
   - the public `libgimpbase` headers;
   - the C++ headers of `gimp_adaptor`, which forward-declare the C types they mention (`typedef struct _Gimp Gimp;`).

   That keeps `src/app` unit-testable with mocks, and keeps GIMP's keyword-unsafe headers (doc 01, section 7) out of the code that does not need them.
3. **C callbacks are `static ... noexcept` thunks** that catch every exception and log it. No exception crosses a GLib frame.
4. **GLib-owned memory lives in RAII holders** (`helpers::glib`). No namespace-scope object holds GLib state, because static initialization runs before `main()`, and so before `gimp_windows_setenv()` must have run.
5. **Parity.** Every observable behavior of today's binaries stays, except the short list in section 12.
6. **Two binaries, two compilations.**
   - The console variant of the C++ libraries is compiled with `-DGIMP_CONSOLE_COMPILATION`, the macro GIMP already uses.
   - GUI-only classes are left out of the console libraries by the Meson source lists.
   - Only `ApplicationFactory` (the remote instance decision) and `Application` (the runner choice) test `GIMP_CONSOLE_COMPILATION`, the darknetxx `DARKNETXX_CONVERTER_ONLY` way.

## 2. Components and namespaces

| Directory | Namespace | Role | May include |
|---|---|---|---|
| `src/main.cpp` | — | `return app::ApplicationFactory::execute(argc, argv);` (the template's file, unchanged) | the factory |
| `src/app` | `app` | The template application layer: factory, context, option names, parser, applications | GLib, `libgimpbase`, the `gimp_adaptor` C++ headers, `src/log`, `src/helpers/glib` |
| `src/app/platform` | `app::platform` | Per-OS process preparation that must run before GLib or GTK | OS headers, GLib |
| `src/app/signals-handlers` | `app` | The template component, with GIMP's categories | `<csignal>`, `<signal.h>`, the `gimp_adaptor::ErrorsHandler` C++ header |
| `src/gimp-adaptor` | `gimp_adaptor` | The bridge to GIMP's C code: bootstrap, early configuration, language, sanity, remote instance, controller and runners, errors, version and update facades, dumpers | GIMP `app/` headers through `gimp-c-includes.h` |
| `src/converters` | `converters` | `ApplicationContext` to `GimpRunContext` | the C++ headers of both sides |
| `src/log` | `logger`, `default_logger` | The template logger, the GLib bridge, the `cpplog4c` C shim | GLib |
| `src/helpers/glib` | `helpers::glib` | RAII holders for GLib and GObject | GLib, GObject |

```
                      ┌────────────────────── src/app (namespace app) ───────────────────────┐
 main.cpp ──────────▶ │ ApplicationFactory ──▶ ApplicationContext ◀── CommandLineParser      │
                      │        │                (StartupOptions)          │ CMDParamNames     │
                      │        ├─▶ platform::IPlatformEnvironment         │                   │
                      │        ├─▶ ISignalsHandler (SignalsHandler + GIMP categories)         │
                      │        └─▶ IApplication: Application, ApplicationHelpPrinter,         │
                      │             ApplicationVersionPrinter, ApplicationLicensePrinter,     │
                      │             GimprcDumpApplication, PdbDeprecatedDumpApplication,      │
                      │             RemoteInstanceApplication (GUI)                           │
                      └───────┬───────────────────────────────┬───────────────────────────────┘
                              │ C++ interfaces only           │
           src/converters ────┤                               │
 (ApplicationContext2         ▼                               ▼
  GimpRunContext)   ┌──────────────────────── src/gimp-adaptor (namespace gimp_adaptor) ───────┐
                    │ ProcessBootstrap  EarlyConfiguration  LibraryOptionGroups               │
                    │ LanguageInitializer  SanityChecker  RemoteInstance  ErrorsHandler       │
                    │ GimpRunContext  GimpController ─▶ runners::ICoreAppRunner               │
                    │                                    (ConsoleCoreAppRunner, GuiCoreAppRunner)│
                    │ VersionInfo  UpdateChecker  GimprcDumper  PdbDeprecatedDumper           │
                    └──────────────────────────────┬───────────────────────────────────────────┘
                                                   │ extern "C" (gimp-c-includes.h)
                    GIMP C: core/gimp.h, config/*, gegl/gimp-gegl.h, file/file-open.h, pdb/*,
                    gui/gui.h, gui/gimpapp.h, dialogs/user-install-dialog.h, gimpcoreapp.h,
                    gimpconsoleapp.h, gimp-version.h, gimp-update.h, gimp-log.h, gimp-debug.h, ...
```

## 3. Class catalog

### 3.1 `app`

| Class | Origin | Responsibility | Replaces | Binary |
|---|---|---|---|---|
| `ApplicationFactory` | template | The composition root: the ordered start-up (section 6.1) and the choice of the application (section 6.2) | `main()`, `app/main.c:661-1077` | both |
| `ApplicationContext` | template | The template flags (help, version, errors, stop/pause/reload/user requests), plus the terminal requests and the `StartupOptions` | the option statics, `app/main.c:129-162` | both |
| `StartupOptions` | new, a plain struct | GIMP's start-up parameters in `std` types (section 4) | the same statics | both |
| `CMDParamNames` | template | Every option name, long and short (section 5) | the names in `main_entries`, `app/main.c:165-328` | both |
| `CommandLineParser` | template | The pre-scan plus the `GOptionContext` parse into the context (section 5) | `app/main.c:840-920, 1153-1203` | both |
| `applications/IApplication` | template | `int run(std::shared_ptr<ApplicationContext>)`, `INVALID` | — | both |
| `applications/Application` | template | The default application: the early sanity check, the conversion, `gimp_adaptor::GimpController::perform()` | `app/main.c:1031-1063`, `app_run()` | both |
| `applications/ApplicationHelpPrinter` | template | The `GOptionContext` help of the requested section, or the parse error text | GLib's built-in `--help` and the error branch, `app/main.c:903-920` | both |
| `applications/ApplicationVersionPrinter` | template | `gimp_adaptor::VersionInfo::show(verbose)`, plus the gimpxx build line (`project_decls`) with `--verbose` | `gimp_show_version_and_exit()`, `app/main.c:1282` | both |
| `applications/ApplicationLicensePrinter` | new | The version, then `GIMP_LICENSE` (`app/about.h`) | `gimp_show_license_and_exit()`, `app/main.c:1291` | both |
| `applications/GimprcDumpApplication` | new | `gimp_adaptor::GimprcDumper::dump(format)` | `gimp_option_dump_gimprc()`, `app/main.c:1205` | both |
| `applications/PdbDeprecatedDumpApplication` | new | `gimp_adaptor::PdbDeprecatedDumper::dump()` | `gimp_option_dump_pdb_procedures_deprecated()`, `app/main.c:1241` | both |
| `applications/RemoteInstanceApplication` | new, a decorator | Forwards the files and batch commands to a running GIMP; on failure runs the wrapped application | `app/main.c:976-1029` | GUI |
| `platform/IPlatformEnvironment` | new | `preinit(argc, argv)` before GLib; `attach_console()`; `process_setup()`; `shutdown()` | — | both |
| `platform/posix/PosixPlatformEnvironment` | new | Nothing today (a placeholder for future POSIX-only work) | — | both |
| `platform/win32/Win32PlatformEnvironment` | new | `gimp_windows_setenv()`; console attaching; the installer mutex; `PANGOCAIRO_BACKEND`; the DLL directory and `PATH`; DEP; the AppUserModelID; the mingw `__argc`/`__argv` | `app/main.c:330-376, 714-722, 733-820, 1097-1148` | both |
| `platform/win32/WinMain.cpp` | new | `WinMain()` for MSVC, which calls `main()` | `app/main.c:1088-1095` | GUI |
| `platform/macos/MacOSPlatformEnvironment` | new | The `-psn_` filter, `gimp_macos_setenv()`, and the `LC_MONETARY`/`LC_NUMERIC` workarounds (the Objective-C stays in `macos-helpers.m` behind a C function) | `app/main.c:378-572, 691-712, 1339-1347` | both |
| `signals-handlers/*` | template, extended | `ISignalsHandler`, the `SignalsHandler` facade, `ASignalsHandler` (gains the `sigaction` flags), and the GIMP categories of section 8 | `app/signals.c` | both |

### 3.2 `gimp_adaptor`

| Class | Responsibility | Replaces | Binary |
|---|---|---|---|
| `gimp-c-includes.h` | The `extern "C"` umbrella of every GIMP header the adaptor uses; defines `GIMP_APP_GLUE_COMPILATION` before `app.h`-guarded headers while they exist | — | both |
| `ProcessBootstrap` | `prepare_crash_reporting()`: `gimp_backtrace_init()` and the crash log path. `early_init()`: `gimp_env_init(FALSE)` (`gimp_env_exit(FALSE)` in `shutdown()`); `gimp_log_init()`; i18n; `g_set_application_name()`; the argv copy (`g_strdupv` / `g_win32_get_command_line`) written back through the context's `char**&`; `g_set_prgname()` | `app/main.c:724-728, 822-838, 1302-1375`; the crash log path part of `gimp_init_signal_handlers()` (`app/signals.c:86-101`) | both |
| `EarlyConfiguration` | The `gimp` option group and its post-parse hook: `GimpEarlyRc` gives the language (and the Win32 pointer input API), then `LanguageInitializer::init()` | `gimp_early_configuration()`, `gimp_options_group_parse_hook()`, `app/main.c:578-659, 896-899` | both |
| `LibraryOptionGroups` | `gegl_config()` (`use-opencl` FALSE, the GPL3 license), the GEGL option group; in the GUI and with an interface, the GTK group and `g_type_class_ref(GIMP_TYPE_DISPLAY)` | `app_libs_init()` (`app/app.c:116-151`), `gui_libs_init()` (`app/gui/gui.c:178-187`) | both |
| `LanguageInitializer` | `init(language)` and `current()`, idempotent as today; the Win32 LANGID table as a `constexpr` array; the macOS parts in `language-macos.m` behind a C API | `app/language.c` (933 lines) | both |
| `SanityChecker` | `check_early()` and `check_late()` return `std::optional<std::string>`; every check is a small function object in an ordered list; `version_at_least()` | `app/sanity.c` (738 lines) | both |
| `IRemoteInstance`, `RemoteInstance` | `open(files, as_new)` and `batch_run(interpreter, commands)`: a D-Bus client of `org.gimp.GIMP.UI` (Linux/BSD), a `WM_COPYDATA` client plus the primary-instance mutex kept for the process lifetime (Windows), always false (macOS) | `app/unique.c` (335 lines), the mutex block of `app/main.c:984-1010` | GUI |
| `GimpRunContext` | The run parameters in GIMP types (`GimpStackTraceMode`, `GimpPDBCompatMode`, `GFile` gimprc files, `gchar**` filenames and batch commands), plus the run state: the `Gimp*`, the `GApplication*`, the exit status | the 23 `app_run()` parameters | both |
| `GimpController` | `perform(runCtx)`, the `app_run()` sequence (section 7); `on_activate()`, the activate sequence; `on_exit_after()` | `app_run()`, `app_activate_callback()`, `app_exit_after_callback()` (`app/app.c:179-590`) | both |
| `runners/ICoreAppRunner` | The strategy for the `GIMP_CONSOLE_COMPILATION` and `no_interface` branches of `app.c` (section 7.3) | — | both |
| `runners/ConsoleCoreAppRunner` | `gimp_console_app_new()`, the console user install, a printed abort, the background hold with a graceful SIGINT/Ctrl-C | `app/app.c:251-262, 277-281, 553-576, 592-623` | both |
| `runners/GuiCoreAppRunner` | `gimp_app_new()`, `user_install_dialog_run()`, `gui_abort()`, `gui_init()`, the crash recovery | `app/app.c:251-262, 280-284, 398-402, 436-486` | GUI |
| `ErrorsHandler` | `init()`, `exit()`, `recovered()`, `fatal_error()`, `terminate()`; the routing of the log domains to `gimp_show_message()`; `eek()` with the debug tool and the stack-trace modes. It is safe before `init()` (section 9). | `app/errors.c` (473 lines) | both |
| `VersionInfo` | A C++ facade: `show(verbose)`, `text(verbose, localized)`, `revision()`, `check_update()`. `gimp-version.c` stays C, because `app/widgets` and `app/dialogs` call it. | — | both |
| `UpdateChecker` | A C++ facade: `auto_check(config, gimp, files_loaded)`. `gimp-update.c` stays C, because of `app/dialogs/about-dialog.c` and the Objective-C. | — | both |
| `GimprcDumper` | `babl_init()`, a temporary `Gimp`, `gimp_load_config()`, `gimp_config_dump(format)` | `app/main.c:1205-1239` | both |
| `PdbDeprecatedDumper` | A temporary `Gimp` in PDB compat mode, `internal_procs_init()`, `gimp_pdb_get_deprecated_procedures()` | `app/main.c:1241-1280` | both |
| `LegacyEntry` | **Phase 2 only.** It calls `gimp_c_main(argc, argv)` (the renamed C `main()`) and disappears in step 3.10. | — | both |

### 3.3 `converters`, `log`, `helpers::glib`

| Class | Responsibility |
|---|---|
| `converters::ApplicationContext2GimpRunContext` | `convert(appctx, runctx)`: the `StartupOptions` in `std` types become the `GimpRunContext` in GIMP types; the enums are mapped; the gimprc paths become `GFile` (`g_file_new_for_commandline_arg()`); the vectors become `gchar**`. |
| `logger::ILogger`, `default_logger::DefaultLogger`, `RealDefaultLogger`, `log.h` | The template's, with the gimpxx defaults of section 10. |
| `log/glib-bridge/GLibLogBridge` | A tee writer for the structured GLib log: it writes into the logger, then into `g_log_writer_default()`. It is installed only with `--log-file`. It never touches `g_print`/`g_printerr`. |
| `log/cpplog4c.h`, `.cpp` | The C API of the logger (`cpplog_enabled()`, `cpploge()`, `cpplogw()`, `cpplogi()`, `cpplogd()`, `cpplogt()`), with `#ifdef __cplusplus` guards, as in darknetxx `src/log/cpplog4c.h`. |
| `helpers::glib::GObjectPtr<T>` | `g_object_ref`/`g_object_unref` ownership, `adopt()` versus `ref()`. |
| `helpers::glib::GCharPtr`, `GStrvPtr`, `GErrorPtr`, `GOptionContextPtr`, `GFilePtr` | `std::unique_ptr` with `g_free`, `g_strfreev`, `g_error_free`, `g_option_context_free` and `g_object_unref` deleters. |
| `helpers::glib::SignalConnection` | A GObject signal handler id that disconnects on destruction. |

### 3.4 What stays C in this layer, and why

| File | Why it stays |
|---|---|
| `app/gimpcoreapp.c`, `app/gimpconsoleapp.c` | GObject types. `core/gimp.c:1383`, `app/gui/gimpapp.c`, `app/tests` and `libgimp/gimpparamspecs-body.c` use the interface. |
| `app/gui/gimpapp.c` | A GObject `GtkApplication` subclass of the GUI assembly (doc 04, R1). |
| `app/gimp-log.c` | The `GIMP_LOG()` macro, `gimp_log_flags` and the domain handlers are used everywhere. Optionally its backend later forwards into the logger through `cpplog4c`. |
| `app/gimp-debug.c` | `core/gimpobject.c` and `config/gimpgeglconfig.c` call it. |
| `app/gimp-version.c`, `app/gimp-update.c` | Widgets and dialogs call them. They move with the GUI layer (doc 04), and get C++ facades now. |
| `app/about.h`, `app/gimp-intl.h`, `app/gimp-priorities.h` | Macros used by C code. |

At the end of Phase 3, `libapp` (GUI) and the new `libappconsole` (the same sources with `-DGIMP_CONSOLE_COMPILATION`, replacing today's inline compilation into the console executable) hold exactly these six C files.

## 4. `ApplicationContext` and `StartupOptions`

`ApplicationContext` keeps the template accessors:
- `get_argc()` and `get_argv()`, both modifiable references;
- `get/set_print_help_and_exit()` and `get/set_print_version_and_exit()`;
- `get_errors()` and `push_error()`;
- `get/set_stop()`, `get/set_pause()`, `get/set_reload()` and the two user request flags.

It adds the following:

| Accessor | Type | Set by | Today |
|---|---|---|---|
| `get/set_print_license_and_exit()` | `bool` | `--license` (pre-scan) | a callback that exits |
| `get/set_help_section()` | `std::string`: `""` (main), `"all"` or a group name | `-h`, `-?`, `--help`, `--help-all`, `--help-<group>` (pre-scan) | GLib's built-in help |
| `get/set_gimprc_dump_format()` | `enum class GimprcDumpFormat {None, Gimprc, GimprcSystem, GimprcManpage}` | `--dump-gimprc`, `--dump-gimprc-system`, `--dump-gimprc-manpage` (pre-scan) | a callback that exits |
| `get/set_dump_pdb_deprecated()` | `bool` | `--dump-pdb-procedures-deprecated` (pre-scan) | a callback that exits |
| `get/set_log_file()` | `std::string` | `--log-file`/`-l` (the template's own option) | — |
| `get/set_backtrace_file()` | `std::string` | `ProcessBootstrap` | `main()`'s `backtrace_file` local |
| `get/set_full_prog_name()` | `std::string` | `ProcessBootstrap` (`argv[0]` after the copy) | `argv[0]` |
| `get_startup_options()`, `startup_options()` | `const StartupOptions&`, `StartupOptions&` | `CommandLineParser` | the statics |

`StartupOptions`, a plain struct in `std` types:

| Field | Type | Default | Option |
|---|---|---|---|
| `system_gimprc`, `user_gimprc` | `std::string` (empty means none) | — | `--system-gimprc`, `--gimprc`/`-g` |
| `session_name` | `std::string` | — | `--session` |
| `batch_interpreter` | `std::string` | — | `--batch-interpreter` |
| `batch_commands` | `std::vector<std::string>` | — | `--batch`/`-b` (repeatable) |
| `filenames` | `std::vector<std::string>` | — | the remaining arguments |
| `quit`, `as_new` | `bool` | false | `--quit`, `--as-new`/`-a` |
| `no_interface` | `bool` | false; true in the console binary | `--no-interface`/`-i` |
| `no_data`, `no_fonts`, `no_splash` | `bool` | false | `-d`, `-f`, `-s` |
| `be_verbose` | `bool` | false | `--verbose`, and `-v` (the pre-scan quirk, section 5) |
| `new_instance` | `bool` | false; true when `no_interface` | `--new-instance`/`-n` |
| `use_shm` | `bool` | true when SysV/POSIX/Win32 shm is available | `--no-shm` |
| `use_cpu_accel` | `bool` | true | `--no-cpu-accel` |
| `console_messages` | `bool` | false | `--console-messages`/`-c` |
| `use_debug_handler` | `bool` | false | `--debug-handlers` |
| `show_playground`, `show_debug_menu` | `bool` | true when `GIMP_UNSTABLE` or not `GIMP_RELEASE` | `--show-playground`, `--show-debug-menu` |
| `stack_trace_mode` | `enum class StackTraceMode {Never, Query, Always}` | Query (unstable), Never (release) | `--stack-trace-mode` |
| `pdb_compat_mode` | `enum class PdbCompatMode {Off, On, Warn}` | Warn (unstable), On (release) | `--pdb-compat-mode` |

## 5. The command line

### 5.1 `CMDParamNames`

Every entry of `main_entries` (`app/main.c:165-328`) plus the template's `--log-file`:

| Constant | Long | Short | Kind | Field |
|---|---|---|---|---|
| `VERSIONW`, `VERSION` | `--version` | `-v` | pre-scan, terminal | `print_version_and_exit` (`-v` also sets `be_verbose`) |
| `LICENSEW` | `--license` | — | pre-scan, terminal | `print_license_and_exit` |
| `HELPW`, `HELP`, `HELPQ`, `HELPALLW` | `--help`, `--help-all`, `--help-<group>` | `-h`, `-?` | pre-scan, terminal | `help_section` |
| `VERBOSEW` | `--verbose` | — | pre-scan + GOption | `be_verbose` |
| `NEWINSTANCEW` | `--new-instance` | `-n` | GOption | `new_instance` |
| `ASNEWW` | `--as-new` | `-a` | GOption | `as_new` |
| `NOINTERFACEW` | `--no-interface` | `-i` | pre-scan + GOption (hidden in the console binary) | `no_interface` |
| `NODATAW` | `--no-data` | `-d` | GOption | `no_data` |
| `NOFONTSW` | `--no-fonts` | `-f` | GOption | `no_fonts` |
| `NOSPLASHW` | `--no-splash` | `-s` | GOption | `no_splash` |
| `NOSHMW` | `--no-shm` | — | GOption, reversed | `use_shm` |
| `NOCPUACCELW` | `--no-cpu-accel` | — | GOption, reversed | `use_cpu_accel` |
| `SESSIONW` | `--session` | — | GOption, filename | `session_name` |
| `GIMPRCW` | `--gimprc` | `-g` | GOption, filename | `user_gimprc` |
| `SYSTEMGIMPRCW` | `--system-gimprc` | — | GOption, filename | `system_gimprc` |
| `BATCHW` | `--batch` | `-b` | GOption, string array | `batch_commands` |
| `BATCHINTERPRETERW` | `--batch-interpreter` | — | GOption | `batch_interpreter` |
| `QUITW` | `--quit` | — | GOption | `quit` |
| `CONSOLEMESSAGESW` | `--console-messages` | `-c` | GOption | `console_messages` |
| `PDBCOMPATMODEW` | `--pdb-compat-mode` | — | GOption callback (`off`/`on`/`warn`) | `pdb_compat_mode` |
| `STACKTRACEMODEW` | `--stack-trace-mode` | — | GOption callback (`never`/`query`/`always`) | `stack_trace_mode` |
| `DEBUGHANDLERSW` | `--debug-handlers` | — | GOption | `use_debug_handler` |
| `GFATALWARNINGSW` | `--g-fatal-warnings` | — | GOption callback; it calls `g_log_set_always_fatal()` at once, as today | — |
| `DUMPGIMPRCW` | `--dump-gimprc` | — | pre-scan, terminal | `gimprc_dump_format` |
| `DUMPGIMPRCSYSTEMW`, `DUMPGIMPRCMANPAGEW` | hidden | — | pre-scan, terminal | `gimprc_dump_format` |
| `DUMPPDBDEPRECATEDW` | `--dump-pdb-procedures-deprecated` (hidden) | — | pre-scan, terminal | `dump_pdb_deprecated` |
| `SHOWPLAYGROUNDW` | `--show-playground` | — | GOption | `show_playground` |
| `SHOWDEBUGMENUW` | `--show-debug-menu` | — | GOption | `show_debug_menu` |
| `LOGPATHW`, `LOGPATH` | `--log-file` | `-l` | pre-scan + GOption | `log_file` (new) |

### 5.2 `CommandLineParser`

**`parse_args(ctx)`:**

1. **`prescan(ctx)`.** One pass over `argv`, up to `--`.
   - It sets `be_verbose` (`--verbose`, `-v`), `no_interface` (`-i`, `--no-interface`) and `log_file`.
   - It sets every terminal request: version, license, help, the dumps.
   - On Windows, a help request makes the platform attach the console, as `app/main.c:870-877` does.
   - `-v` keeps today's quirk: the first pre-scan loop (`:840-852`) treats `-v` as `--verbose`, and the second (`:854-878`) treats it as `--version`, so `-v` prints the verbose version.
2. **A terminal request returns here**, before any `GOptionContext` is parsed. So no post-parse hook runs: no early gimprc, and no GTK display. Today `--version` already returns early like this; `--license` and the dumps exit inside the parser *before* the hooks; `--help` exits inside GLib *before* the hooks. The behavior therefore stays the same.
3. **The console binary forces `no_interface`** (`app/main.c:880-882`).
4. **`create_option_context(ctx, storage)`**, which `ApplicationHelpPrinter` reuses:
   - `g_option_context_new(_("[FILE|URI...]"))`, the summary `GIMP_NAME`, and the main entries generated from the table of section 5.1 and bound to a private `OptionStorage` of GLib-typed fields;
   - **then** the empty `gimp` group with `EarlyConfiguration` as its post-parse hook (`app/main.c:896-899`);
   - **then** `LibraryOptionGroups::add(context, no_interface)` adds the GEGL group and, in the GUI without `-i`, the GTK group.

   This group order is the hook order, and the language has to be set before GTK initializes.
5. **`g_option_context_parse_strv(&argv)`.** On failure, the error text goes into `ctx->push_error()`: `error->message`, or, when GLib gives no error, "GIMP could not initialize the graphical user interface..." (`app/main.c:905-918`).
6. **`apply(storage, ctx)`** copies the GLib fields into `StartupOptions` and frees them; `no_interface` implies `new_instance` (`app/main.c:973-974`).

**`get_custom_logfile(argc, argv)`** is the template's static, and still runs before the logger initializes.

**The parse has side effects that are deliberately kept:**
- the `gimp` hook reads gimprc and sets the locale;
- the GTK hook opens the display;
- `--g-fatal-warnings` changes the GLib fatal mask.

The unit tests mock `EarlyConfiguration` and `LibraryOptionGroups`; the component tests use the real ones.

## 6. `ApplicationFactory`

### 6.1 `run(argc, argv)`, the start-up order

| # | Step | Today | Why it is here |
|---|---|---|---|
| 1 | `platform = create_platform_environment(); platform->preinit(argc, argv)` | `app/main.c:682-722` (Windows environment, unstable console, `-psn_`, macOS environment, mingw argv) | `GSETTINGS_SCHEMA_DIR` must be set before GLib starts |
| 2 | The logger: `LOG_INIT(...)` with the defaults of section 10 and `get_custom_logfile()`; `GLibLogBridge::install()` when a file is given | — | before anything else logs |
| 3 | `ctx = create_default_context(argc, argv)` | — | the template's own step |
| 4 | `bootstrap = create_bootstrap(); bootstrap->prepare_crash_reporting(ctx)`: `gimp_backtrace_init()` and the crash log path | `app/main.c:728`; `app/signals.c:86-101` | The Windows backtrace backend must see the threads created later; the signals need the path. |
| 5 | `signals = create_default_signals_handler(); signals->install(ctx)` | `gimp_init_signal_handlers()`, `app/main.c:731` | Early, to catch crashes during the start-up. *The template installs them right before `run()`; the order here is GIMP's.* |
| 6 | `platform->process_setup()`: the Windows mutex, `PANGOCAIRO_BACKEND`, DLL directory, `PATH`, DEP and AUMID | `app/main.c:733-820` | today's position, between the signals and the library set-up |
| 7 | `bootstrap->early_init(ctx)`: `gimp_env_init()`, `gimp_log_init()`, i18n, the application name, the argv copy, the prgname | `app/main.c:822-838` | before the parse, which needs the translated strings |
| 8 | `create_default_arg_parser()->parse_args(ctx)` | `app/main.c:840-920` | — |
| 9 | GUI, not on X11, with an interface: the flatpak `prgname` from `/.flatpak-info` (`gimp_adaptor::ProcessBootstrap::apply_flatpak_prgname()`) | `app/main.c:922-968` | needs the GDK display, which exists only after the parse |
| 10 | `platform->attach_console()` when `no_interface`, `be_verbose`, `console_messages` or batch commands | `app/main.c:970-971` | — |
| 11 | `app = create_application(ctx)` (section 6.2) | — | — |
| 12 | `status = app->run(ctx)` | `app/main.c:1031-1063` | — |
| 13 | `bootstrap->shutdown()` (`gimp_env_exit()`), `platform->shutdown()`, then `signals` and `ctx` go out of scope | `app/main.c:1065-1076` | — |

`execute(argc, argv)` stays the template's static convenience method. `run()` catches `std::exception` around everything (for example the template logger throws when a log file cannot be opened), logs it and returns `IApplication::INVALID`.

### 6.2 `create_application(ctx)`

| Order | Condition | Application | Today |
|---|---|---|---|
| 1 | `!ctx->get_errors().empty()` (the parse failed) | `ApplicationHelpPrinter` in error mode: the error text exactly as today, `EXIT_FAILURE` | `app/main.c:903-920` |
| 2 | `get_print_version_and_exit()` | `ApplicationVersionPrinter` | the pre-scan, `app/main.c:866-869` |
| 3 | `!get_help_section().empty()` or `get_print_help_and_exit()` | `ApplicationHelpPrinter` | GLib |
| 4 | `get_print_license_and_exit()` | `ApplicationLicensePrinter` | a callback |
| 5 | `get_gimprc_dump_format() != None` | `GimprcDumpApplication` | a callback |
| 6 | `get_dump_pdb_deprecated()` | `PdbDeprecatedDumpApplication` | a callback |
| 7 | GUI binary and `!new_instance` | `RemoteInstanceApplication(create_default_application())` | `app/main.c:976-1029` |
| 8 | otherwise | `create_default_application()`, which is `Application` | `app_run()` |

`RemoteInstanceApplication::run(ctx)`:
- If `IRemoteInstance::open(filenames, as_new)` succeeds: print "Another GIMP instance is already running." when verbose, forward the batch commands (`EXIT_FAILURE` when that fails), call `gdk_notify_startup_complete()` and return.
- Otherwise run the wrapped `Application`.

On Windows, `RemoteInstance` keeps today's `Local\gimp_primary_instance_running` mutex logic, including the 20 retries of 100 ms.

## 7. `Application`, `GimpController` and the runners

### 7.1 `Application::run(ctx)`

1. `runner = create_runner(ctx)`: a `GuiCoreAppRunner` in the GUI binary without `-i`, a `ConsoleCoreAppRunner` otherwise.
2. `gimp_adaptor::SanityChecker::check_early()`; on failure `runner->abort(message)`: printed in console or `-i` mode, otherwise `gui_abort()`. Today: `app/main.c:1031-1033` and `app_abort()`, `app/app.c:153-171`.
3. `converters::ApplicationContext2GimpRunContext::convert(ctx, runCtx)`.
4. `return gimp_adaptor::GimpController(runner).perform(runCtx)`.

### 7.2 `GimpController::perform(runCtx)`, which is `app_run()` (`app/app.c:179-347`)

| Step | Method (virtual, so tests can stub it) | Lines |
|---|---|---|
| 1 | `resolve_default_folder()`: a single directory argument becomes the default folder | 211-229 |
| 2 | `create_gimp()`: `gimp_new(...)` with its 14 arguments | 234-247 |
| 3 | `runner->create_core_app()`; `gimp->app = app` | 251-264 |
| 4 | `gimp_cpu_accel_set_use()` | 266 |
| 5 | `ensure_user_install()`: `runner->run_user_install()` when the gimpdir is missing | 268-289 |
| 6 | `gimp_load_config()` | 291 |
| 7 | `ErrorsHandler::init(...)` | 303 |
| 8 | `SanityChecker::check_late()`, then `runner->abort()` on failure | 309-311 |
| 9 | `gimp_gegl_init()` | 314 |
| 10 | Connects `exit` (after) to the `on_exit_after` thunk, which calls `g_application_quit()`; connects `activate` to the `on_activate` thunk. Both are `SignalConnection`s. | 316-322 |
| 11 | `g_application_run(app, 0, NULL)`; the exit status | 323-326 |
| 12 | `teardown()`: the verbose `EXIT:` line, the main context drained, the app unref, `gimp_gegl_exit()`, `ErrorsHandler::exit()`, the gimp unref, `gimp_debug_instances()`, `gegl_exit()`. **Same order.** | 328-346 |

`on_activate()`, which is `app_activate_callback()` (`app/app.c:374-577`):
1. the exit status success;
2. `LanguageInitializer::current()`;
3. `runner->init_gui()` returns the status callback, or a no-op one;
4. `gimp_initialize()`;
5. the macOS language notify (`#ifdef PLATFORM_OSX`, through the Objective-C helper);
6. `query_all`;
7. `gimp_restore()`;
8. the autosave;
9. `runner->recover_images()`;
10. the filenames;
11. `UpdateChecker::auto_check()`;
12. `config-version` and `prev-language`;
13. `file_open_from_command_line()` for each file;
14. `initialized`;
15. the font error message;
16. `gimp_batch_run()`;
17. with `--quit`, the exit status and `gimp_exit(gimp, TRUE)`; otherwise `runner->after_activate()`.

### 7.3 The runners

| `ICoreAppRunner` | `ConsoleCoreAppRunner` | `GuiCoreAppRunner` | `app/app.c` today |
|---|---|---|---|
| `create_core_app(gimp, runCtx)` | `gimp_console_app_new()` | `gimp_app_new(gimp, no_splash, ...)` | 251-262 |
| `run_user_install(install)` | `gimp_user_install_run(install, 1)` | `user_install_dialog_run(install)`, `exit(EXIT_FAILURE)` on false | 277-284 |
| `[[noreturn]] abort(message)` | `g_print("%s\n\n")`, `exit(EXIT_FAILURE)` | `gui_abort()` | 153-171 |
| `init_gui(gimp, app, system_lang_l10n)` | `nullptr` | `gui_init(...)` | 398-402 |
| `recover_images(gimp)` | nothing | `ErrorsHandler::recovered()`, `gui_recover()`, `gimp_image_load_from_cache()`, `gimp_create_display()` | 436-486 |
| `after_activate(gimp)` | the INFO text; SIGINT (POSIX, through `gimp_signal_private`) or `SetConsoleCtrlHandler` (Windows), which calls `g_application_release()` and `gimp_exit(gimp, TRUE)`; `g_application_hold()` | nothing (the GTK main loop runs) | 553-576, 592-623 |

## 8. OS signals

The template component, populated with GIMP's categories. `ASignalsHandler` gains `get_flags()` (default 0) and installs through `gimp_signal_private()` (`libgimpbase/gimpsignal-private.h`, a `sigaction` wrapper) instead of `std::signal`, which cannot express `SA_RESTART`.

| Handler | Signals | Disposition | Flags | Today (`app/signals.c`) |
|---|---|---|---|---|
| `posix/TerminateSignalsHandler` | SIGHUP, SIGINT, SIGQUIT, SIGTERM | `ErrorsHandler::terminate(g_strsignal(sig))` | 0 | `gimp_sigfatal_handler()` calls `gimp_terminate()` |
| `posix/FatalSignalsHandler` | SIGABRT, SIGBUS, SIGSEGV, SIGFPE; **none on macOS** | `ErrorsHandler::fatal_error(g_strsignal(sig))` | 0 | `gimp_fatal_error()` |
| `posix/IgnoredSignalsHandler` (template) | SIGPIPE | `SIG_IGN` | 0 | the same |
| `posix/ChildSignalsHandler` | SIGCHLD | `SIG_DFL` | `SA_RESTART` | the same |
| `win32/Win32ExceptionHandler` | unhandled exceptions | `_NT_SYMBOL_PATH` for the relocatable bundle, `SetUnhandledExceptionFilter`, then `ExcHndlInit()` and `ExcHndlSetLogFileNameW(backtrace_file)` | — | `app/signals.c:67-84, 103-129, 163-200` |

**Notes:**
- **The template's other categories are not created** by `SignalsHandler::create_handlers()`: Stop, Pause (SIGTSTP/SIGTTIN/SIGTTOU), Continue, Reload (SIGHUP) and User (SIGUSR1/2). GIMP has no such semantics, and SIGHUP belongs to Terminate. Their classes stay in the tree for the opt-in step below.
- **The fatal and terminate dispositions are not async-signal-safe.** They print, spawn `gimp-debug-tool` and call `exit()`. That is GIMP's existing behavior, and it is kept, not "fixed", in this layer.
- **The graceful stop is opt-in, after Phase 3** (open decision 5 of doc 02):
  - `StopSignalsHandler` raises `ctx->set_stop(true)` for SIGINT and SIGTERM;
  - a `gimp_adaptor::StopRequestWatcher` (on Unix, `g_unix_signal_add()`, which is safe and runs in the main loop) calls `gimp_exit(gimp, TRUE)`;
  - a second signal falls back to Terminate.

## 9. `ErrorsHandler`

| Today (`app/errors.c`) | `ErrorsHandler` |
|---|---|
| The statics `the_errors_gimp`, `use_debug_handler`, `stack_trace_mode`, `full_prog_name`, `backtrace_file`, the handler ids (`:60-66`) | Members of one instance, reached through a function-local static `instance()`. The signal handlers need a global entry point, but nothing is constructed before `main()`. |
| `errors_init()` (`:88`) | `init(gimp, full_prog_name, use_debug_handler, stack_trace_mode, backtrace_file)` |
| `errors_exit()` (`:127`) | `exit()` |
| `errors_recovered()` (`:152`) | `recovered(gimp)` returns a `GHashTable` holder |
| `gimp_fatal_error()`, `gimp_terminate()` (`:232-243`) | `fatal_error(msg)`, `terminate(msg)`, both `[[noreturn]]` |
| `gimp_message_log_func()` (`:247`) | the same routing by `debug-policy`, plus a `LOGW` copy when a log file is active |
| `gimp_eek()` (`:319`) | `eek()`, the same logic: the GUI debug tool, `gimp_stack_trace_query`/`print`, the Windows `MessageBox`. **New: before `init()` it prints the reason and `_exit(EXIT_FAILURE)`**, where today it dereferences a NULL `the_errors_gimp`. |

## 10. Logging

| Setting | Template default | gimpxx |
|---|---|---|
| Log file | `<cwd>/<PROJECT_NAME>-<version>.log`, always | only with `--log-file <path>`/`-l` |
| Printing | yes; INFO and below to stdout, WARNING and above to stderr | yes, **everything to stderr**; stdout is reserved for `--version`, the dumps and batch output |
| Printed level | `MAX_LOG_LEVEL` (option, default 3, INFO) | WARNING; INFO with `--verbose`; `MAX_LOG_LEVEL` still caps the compiled level |
| GLib messages | — | `GLibLogBridge` tees the structured log into the file when one is requested. GIMP's own domains go through `ErrorsHandler` (dialogs), which copies them into the logger. |
| C code | — | `cpplog4c` is available; the C code does not use it in this layer |

The change to the template's `RealDefaultLogger` (stderr only) is local and documented, the way the template invites ("infrastructure implementation items may be altered"). `G_LOG_DOMAIN` is `"Gimp"` for `src/gimp-adaptor`, so the messages keep today's routing. `src/app` logs through `LOG*` only.

## 11. Build of the layer

Sketch of `src/meson.build`. While GIMP is still at the root, it is entered from the root `meson.build` right after `subdir('app')`; after the relocation, right after `subdir('gimp')`. In both cases it comes before `gimp_run_env`.

```meson
gimpxx_override_options = ['cpp_std=c++17', 'warning_level=3']
gimpxx_cpp_args = [
  '-DG_LOG_DOMAIN="Gimp"',
  '-DMAX_LOG_LEVEL=' + get_option('MAX_LOG_LEVEL').to_string(),
]
gimpxx_includes = [rootInclude, appInclude, gimpxx_root_include, gimpxx_config_include]

subdir('helpers/glib')   # gimpxx_helpers_glib     (static)
subdir('log')            # gimpxx_log              (static)
subdir('gimp-adaptor')   # gimpxx_adaptor_gui, gimpxx_adaptor_console (static, two variants)
subdir('converters')     # gimpxx_converters       (static)
subdir('app')            # gimpxx_app_gui, gimpxx_app_console       (static, two variants)

gimpmain_exe = executable(gimpmain_exe_name,
  'main.cpp', gui_rc_file, win32_winmain_sources,
  include_directories: gimpxx_includes,
  link_whole: [gimpxx_app_gui, gimpxx_adaptor_gui, gimpxx_converters, gimpxx_log, gimpxx_helpers_glib],
  link_with: [libapp, app_libgimps, gui_libapps],
  dependencies: libapp_dep,
  override_options: gimpxx_override_options,
  win_subsystem: 'windows',
  install: true,
)

if enable_console_bin
  gimpconsole_exe = executable(gimpconsole_exe_name,
    'main.cpp', console_rc_file,
    cpp_args: ['-DGIMP_CONSOLE_COMPILATION'],
    include_directories: gimpxx_includes,
    link_whole: [gimpxx_app_console, gimpxx_adaptor_console, gimpxx_converters, gimpxx_log, gimpxx_helpers_glib],
    link_with: [libappconsole, app_libgimps, console_libapps],
    dependencies: gimpconsole_deps,
    override_options: gimpxx_override_options,
    win_subsystem: 'console',
    install: true,
  )
endif
# followed by the install_symlink / Windows copy logic moved from app/meson.build:221-250
```

**Notes:**
- `gimpxx_root_include` (the repository root, so that `#include "src/app/..."` works as in the template) and `gimpxx_config_include` (the `project-global-decls.h` directory) come from `meson/gimpxx-misc-variables-declare` and `meson/gimpxx-configure-file`.
- The two variants of `gimpxx_app_*` and `gimpxx_adaptor_*` differ in `-DGIMP_CONSOLE_COMPILATION` and in the GUI-only sources:
  - `RemoteInstanceApplication`, `RemoteInstance`, `GuiCoreAppRunner` and `WinMain.cpp`;
  - the GTK part of `LibraryOptionGroups` (a separate `LibraryOptionGroupsGtk.cpp`).
- **Tests:** `if get_option('ENABLE_UNIT_TESTS')` and `ENABLE_COMPONENT_TESTS`, a `tests/` subdirectory per component, as in the template's Meson branches. They are registered in the suites `gimpxx-unit` and `gimpxx-component` (Meson shows them with the project prefix, `gimp:gimpxx-unit`). The parity harness is a separate suite, `gimpxx-parity`, which depends on both executables.

## 12. Intended differences from upstream

| # | Difference | Reason |
|---|---|---|
| 1 | `--version --verbose` (and `-v`) adds one gimpxx build line (version, commit, configure date from `project_decls`). Without `--verbose` the output is byte-identical. | The template's version printer contract |
| 2 | `--help` lists `--log-file`/`-l`. | The template's option |
| 3 | The terminal options take effect regardless of their position, with a fixed precedence: version, help, license, gimprc dump, PDB dump. Today the first one GLib meets wins, and an invalid option *before* it aborts first. | Pre-scan without the hooks |
| 4 | A fatal signal before `errors_init()` prints and exits, instead of crashing inside the handler. | A bug fix (section 9) |

Anything else that the parity harness reports is a regression.

## 13. Parity matrix

The harness runs each line against the reference and the candidate binaries:
- environments: `LANG=C` and `LANG=uk_UA.UTF-8`, a temporary `GIMP3_DIRECTORY`, no `DISPLAY`/`WAYLAND_DISPLAY`, and additionally under `xvfb-run` plus `dbus-run-session` when available;
- comparison: the exit code, stdout and stderr after normalization of the library versions, `$HOME` and build paths, PIDs, timestamps, CPU counts and memory sizes (`num-processors`, `tile-cache-size` in the gimprc dump).

| # | Binary | Arguments |
|---|---|---|
| 1-3 | both | `--version`; `-v`; `--version --verbose` |
| 4 | both | `--license` |
| 5-9 | both | `--help`; `-?`; `--help-all`; `--help-gegl`; `--help-gtk` (GUI) |
| 10-13 | both | `--dump-gimprc`; `--dump-gimprc-system`; `--dump-gimprc-manpage`; `--dump-pdb-procedures-deprecated` |
| 14-16 | both | `--bogus`; `--stack-trace-mode=bogus`; `--pdb-compat-mode=bogus` |
| 17 | both | `-idf --quit --batch-interpreter=plug-in-script-fu-eval -b '(gimp-message "ok")'` |
| 18 | both | `-idf --quit --batch-interpreter=python-fu-eval -b 'print("ok")'` (the form `libgimp/tests` uses, there with `-nis`) |
| 18a | both | `-idf --quit -b '(gimp-quit 0)'` without an interpreter: with several batch interpreters installed this is the "No batch interpreter specified" path, exit 64 (`core/gimp-batch.c`); `GIMP_BATCH_INTERPRETER` set and unset |
| 19 | both | `-idf --quit <app/tests/files test image>` (open from the command line) |
| 20 | both | `-idf --quit --gimprc=/nonexistent` |
| 21 | GUI | no arguments, no display (the GUI initialization error path) |
| 22 | GUI, xvfb | `-df --quit -b ...` (GUI start-up, splash off) |
| 23 | GUI, xvfb + dbus | a second instance with a file while the first runs (the remote instance) |
| 24 | console | `-idf` (background mode), then SIGINT: the graceful exit text and status |
| 25 | console | `-idf`, then SIGTERM: "terminated", exit 1; SIGSEGV with `--stack-trace-mode=never`: "fatal error", exit 1 |

## 14. Tests per class

| Class | Unit (`UTEST_`, mocks) | Component (`CTEST_`) |
|---|---|---|
| `CommandLineParser` | the pre-scan precedence and the `-v` quirk; the console forcing; every option of section 5.1 into its field; the error collection (`EarlyConfiguration` and `LibraryOptionGroups` mocked) | a real `GOptionContext` with the GEGL group; the help text equals `g_option_context_get_help()` |
| `ApplicationFactory` | the decision table of section 6.2 with mock applications; the `run()` order through recording mocks of the platform, bootstrap, signals and parser | — |
| `ApplicationContext`, `StartupOptions` | the accessors and defaults (unstable and release builds) | — |
| Printers | stdout captured; `VersionInfo` mocked | — |
| `RemoteInstanceApplication` | `IRemoteInstance` mocked: success skips the wrapped application; failure runs it; a batch failure returns `EXIT_FAILURE` | D-Bus against a session bus (xvfb job) |
| `platform::*` | built only on their platform; the `-psn_` filter | — |
| `signals-handlers` | the template tests adapted: the signal sets and the flags | dispositions checked with `sigaction()` in a forked child; delivery of SIGPIPE and SIGCHLD |
| `SanityChecker` | `version_at_least()`; the check order with injected version providers | against the real libraries |
| `LanguageInitializer` | the LANGID table; idempotence | `setlocale` effects in a child process |
| `GimpController` and runners | the step order with stub runners | a console run with a real `Gimp`, reusing the `app/tests` environment (`GIMP_TESTING_*`) |
| `ErrorsHandler` | the routing by `debug-policy`; the behavior before `init()` | the crash file path creation |
| `converters::ApplicationContext2GimpRunContext` | every field and enum | — |
| `log`, `GLibLogBridge` | the template tests | `g_log` into a file; `g_print` untouched |
| `helpers::glib` | ownership, moves, deleters | — |

## 15. New files

```
src/main.cpp                                   (template)
src/project-global-decls.h.meson.in            (template; gimpxx_version)
src/meson.build
src/app/{ApplicationContext,ApplicationFactory,CommandLineParser}.{h,cpp}, CMDParamNames.h, StartupOptions.h
src/app/applications/{IApplication.h, Application, ApplicationHelpPrinter, ApplicationVersionPrinter,
                      ApplicationLicensePrinter, GimprcDumpApplication, PdbDeprecatedDumpApplication,
                      RemoteInstanceApplication}.{h,cpp}
src/app/platform/IPlatformEnvironment.h, posix/PosixPlatformEnvironment.{h,cpp},
                 win32/{Win32PlatformEnvironment.{h,cpp}, WinMain.cpp},
                 macos/{MacOSPlatformEnvironment.{h,cpp}, macos-helpers.{h,m}}
src/app/signals-handlers/{ISignalsHandler.h, SignalsHandler.{h,cpp}}            (template)
src/app/signals-handlers/handlers/ASignalsHandler.{h,cpp}                        (template + flags)
src/app/signals-handlers/handlers/posix/{Terminate,Fatal,Ignored,Child}SignalsHandler.{h,cpp}
src/app/signals-handlers/handlers/posix/{Stop,Pause,Continue,Reload,User}SignalsHandler.{h,cpp}  (template, not installed)
src/app/signals-handlers/handlers/win32/Win32ExceptionHandler.{h,cpp}
src/gimp-adaptor/gimp-c-includes.h
src/gimp-adaptor/{ProcessBootstrap,EarlyConfiguration,LibraryOptionGroups,LibraryOptionGroupsGtk,
                  LanguageInitializer,SanityChecker,RemoteInstance,GimpRunContext,GimpController,
                  ErrorsHandler,VersionInfo,UpdateChecker,GimprcDumper,PdbDeprecatedDumper}.{h,cpp}
src/gimp-adaptor/IRemoteInstance.h, language-macos.{h,m}, LegacyEntry.{h,cpp} (Phase 2 only)
src/gimp-adaptor/runners/{ICoreAppRunner.h, ConsoleCoreAppRunner.{h,cpp}, GuiCoreAppRunner.{h,cpp}}
src/converters/ApplicationContext2GimpRunContext.{h,cpp}
src/log/...                                                                       (template)
src/log/glib-bridge/GLibLogBridge.{h,cpp}, src/log/cpplog4c.{h,cpp}
src/helpers/glib/{GObjectPtr.h, GPtr.h, SignalConnection.{h,cpp}}
src/**/tests/{unit,component,mocks}/...                                           (template layout)
src/tests/mocks/...                                                               (shared mocks)
scripts/parity/gimp-cli-parity.py, scripts/parity/golden/...
```

Removed at the end of Phase 3: `app/main.c`, `app/app.c`, `app/app.h`, `app/signals.c`, `app/signals.h`, `app/errors.c`, `app/errors.h`, `app/sanity.c`, `app/sanity.h`, `app/language.c`, `app/language.h`, `app/unique.c`, `app/unique.h`. That is 4,896 lines; the temporary `app/gimp-c-main.h` goes with them.
