# gimpxx architecture: from `main()` down

**Status:** analysis that feeds the [transition plan](02-gimpxx-cpp-transition-plan.md). It describes the code as it is and changes nothing.

**Analyzed on 2026-10-09**, against gimpxx `master` `b7f8cac066` (2026-10-07), which is GIMP 3.3.1 (the 3.3 development branch), with the `gimp-data` submodule at `fe4ecc0`. The line numbers are from that revision; once they drift, search by the function names.

## 1. The repository at a glance

| Metric | Value |
|---|---|
| Tracked files (without the `gimp-data` submodule) | 5,254 |
| C sources / headers | 1,825 `.c`, 1,665 `.h`, 1,269,910 lines |
| C++ | 10 `.cc` files, 9,517 lines (9 in `app/`, plus `plug-ins/file-exr/openexr-wrapper.cc`) |
| Other languages | 158 Scheme (`.scm`) scripts, about 70 Python files, 62 PDB definitions (`.pdb`) plus the Perl generator, 35 GtkBuilder `.ui` files |
| Meson | 142 tracked `meson.build` files plus 40 in `gimp-data`; the root one has 2,323 lines |
| GObject types and interfaces (`G_DEFINE_*`) | 908: `app/` 622, libraries 142, plug-ins 129, modules 14 |
| Upstream history | 59,365 commits |

**Products of the build:**
- the executables `gimp-3.3` and `gimp-console-3.3`;
- nine shared libraries `lib*-3.0.so.0` (soname version 0.300.1), plus `libgimp-scriptfu-3.0`;
- `Gimp-3.0.gir` / `GimpUi-3.0.gir` and their typelibs (and VAPIs when Vala is enabled);
- about 130 plug-in executables plus Python and Script-Fu plug-ins;
- 11 loadable modules;
- data, translations and desktop files.

Size by area (C/C++ lines including headers):

| Area | Directories | Lines |
|---|---|---|
| Application glue | `app/*.c`, `app/*.h` | 7,133 |
| GUI assembly | `app/gui` | 7,962 |
| GUI frameworks | `app/widgets` 158,303 · `app/tools` 79,743 · `app/display` 68,987 · `app/dialogs` 40,236 · `app/actions` 40,118 · `app/propgui` 6,840 · `app/menus` 2,570 | 396,797 |
| Core services | `app/core` 163,462 · `app/pdb` 62,542 · `app/plug-in` 14,670 · `app/path` 12,111 · `app/xcf` 11,351 · `app/config` 10,903 · `app/text` 8,797 · `app/file` 2,490 · `app/file-data` 2,392 | 288,718 |
| Engine | `app/operations` 30,515 · `app/paint` 19,798 · `app/gegl` 8,553 | 58,866 |
| App tests | `app/tests` | 3,996 |
| Public libraries | `libgimp` 91,709 · `libgimpwidgets` 59,076 · `libgimpbase` 22,667 · `libgimpconfig` 9,261 · `libgimpcolor` 6,217 · `libgimpthumb` 3,066 · `libgimpmath` 2,825 · `libgimpmodule` 1,485 | 196,306 |
| Plug-ins | `plug-ins` | 307,854 |
| Modules, tools | `modules` 8,267 · `tools` 3,084 · `extensions` 300 · `app-tools` 144 | 11,795 |

Top-level directories:

| Directory | Role |
|---|---|
| `app/` | The application (the `gimp-3.3` and `gimp-console-3.3` executables), as 24 static libraries `libapp*` |
| `libgimpbase`, `libgimpcolor`, `libgimpmath`, `libgimpconfig`, `libgimpmodule`, `libgimpthumb`, `libgimpwidgets`, `libgimp` | Installed public libraries with a frozen C ABI, shared by the core and the plug-ins |
| `plug-ins/` | Separate executables that talk to the core over pipes (the wire protocol) |
| `modules/` | `GTypeModule`s loaded at run time: color selectors, display filters, controllers |
| `pdb/` | The Perl generator of the PDB procedure code (`app/pdb/*-cmds.c`, `libgimp/*_pdb.[ch]`) |
| `data/`, `etc/`, `menus/`, `themes/`, `gimp-data/` (submodule) | Brushes, gradients, gimprc defaults, GtkBuilder menus, CSS themes, icons and images |
| `po*/` | Translations (8 `po*` directories) |
| `build/` | Packaging: Linux (AppImage, flatpak, snap), Windows (MSYS2, installer, store), macOS |
| `tools/`, `app-tools/` | Build helpers (`meson-mkenums.py`, `in-build-gimp.py`, `defcheck.py`), `gimptool`, `gimp-debug-tool` |
| `docs/`, `devel-docs/` | Man pages, gi-docgen reference, developer notes |
| `extensions/` | The "goat exercise" demo in C, Python, Vala, GJS and Lua |

## 2. The build

**`project()`** (`meson.build:1-10`):
- `project('gimp', 'c', 'cpp', version: '3.3.1', meson_version: '>=0.61.0')`, with `cpp_std=c++14` and `buildtype=debugoptimized`. `c_std` is deliberately unset.
- `meson.project_version()` derives the executable names, `so_version`, the API name `gimp-3.0`, the gettext domain and the install directories (`:26-132`).
- `meson.project_name()` sets `GIMP_PACKAGE`/`PACKAGE_NAME` (`:1672-1673`) and the `gimp/3.0` install subdirectory (`:1698`).

**Flags:**
- C and C++ share the common warnings and `-DHAVE_CONFIG_H` (`compiler_args`, `:1887`).
- C++ has an empty warning list (`:321-326`).
- The `GLIB_VERSION_*` and `GDK_VERSION_*` limits are C-only (`:485,491,506-507`).
- On macOS every C file is compiled as Objective-C (`add_project_arguments('-ObjC', language: 'c')`, `libgimpbase/meson.build:2`).

**Order** (`:1951-2130`):
1. `libgimpbase`, `tools`, `pdb`
2. `po*`
3. `data`, `etc`, `menus`, `themes`, `gimp-data`
4. the other libraries, up to `libgimp`
5. `extensions`, `modules`, `plug-ins`, `app`, `app-tools`, `desktop`
6. the run environment, `gimp_run_env` (`:2000-2108`)
7. `gimp-data/images`, `app/tests`, `libgimp/tests`, `docs`, `devel-docs`

`config.h` is generated in the build root (`:1880-1883`) and found through `rootInclude = include_directories('.')` (`:1948`); `appInclude` is `include_directories('app')`.

**The application** (`app/meson.build`):
- Each subdirectory is a static library: `libappcore`, `libappwidgets`, ... 24 of them.
- **`libapp`** (`:72-77`) is the glue: `app.c`, `errors.c`, `gimpcoreapp.c`, `gimpconsoleapp.c`, `gimp-debug.c`, `gimp-log.c`, `gimp-update.c`, `gimp-version.c`, `language.c`, `sanity.c`, `signals.c`, `unique.c`. It is compiled with `-DG_LOG_DOMAIN="Gimp" -DGIMP_APP_GLUE_COMPILATION`.
- **`gimp-console-3.3`** (`:188-206`) compiles `main.c` *and the glue sources again* with `-DGIMP_CONSOLE_COMPILATION`. It links only the core-side libraries (`console_libapps`, `:95-110`) and no GTK.
- **`gimp-3.3`** (`:208-219`) compiles `main.c` and links `libapp`, the core-side and the GUI libraries (`gui_libapps`, `:143-154`).
- The unversioned `gimp`/`gimp-3` symlinks are only installed for stable releases (`enable-default-bin`).

**The build runs GIMP.** `gimp_run_env` points `GIMP_SELF_IN_BUILD` at the fresh `gimp-console-3.3` (or `gimp-3.3`) (`:2028-2034`). `tools/in-build-gimp.py` then runs it with a temporary config directory to:
- render the `gimp-data/images` (splash, logos) through Python plug-ins;
- run the Python and C `libgimp` tests.

A broken entry point therefore breaks the build itself.

**Generated code written into the source tree:**
- The PDB generator writes `app/pdb/*-cmds.c`, `internal-procs.[ch]`, `libgimp/*_pdb.[ch]` and `libgimp/gimpenums.h` on every build (`destdir = <source root>`, `pdb/meson.build:117-170`).
- `tools/meson-mkenums.py` rewrites the `*-enums.c` files (`:57-59`).
- CI fails if the tree differs afterwards.

**Generated in the build directory:** `config.h`, `git-version.h`, the genmarshal marshallers (`app/core`, `libgimpwidgets`, `libgimp`), the gdbus-codegen service (`app/gui/meson.build:2-7`), GResources (icons, cursors), the GIR/typelib/VAPI files and `check-def-files`.

**Tests:** 21 Meson tests.
- `app`: `core`, `gimpidtable`, `xcf` (GLib GTest), `save-and-export` (Python), `app-config`.
- `desktop`: 2 tests.
- `libgimp`: 7 topics, each run as a Python and a C test.

`headless` (xvfb) is the default test setup when `xvfb-run` and `dbus-run-session` are found (`:1377-1385`).

**CI:**
- Only GitLab (`.gitlab-ci.yml`): Debian trixie, babl and GEGL from source, then `meson setup` / `ninja` / `ninja test` / `ninja dist`, plus packaging jobs for AppImage, flatpak, snap, MSYS2 and MacPorts.
- There is no `.github/`, and **no `.gitignore` anywhere in the tree**.

## 3. The layers, top to bottom

```
 L0  Process entry and application glue   app/main.c, app/app.c, app/{signals,errors,sanity,language,unique,
                                           gimp-update,gimp-version,gimp-log,gimp-debug}.c          7.1K
 L1  GApplication glue                     app/gimpcoreapp.c (interface), app/gimpconsoleapp.c (GApplication),
                                           app/gui/gimpapp.c (GtkApplication)
 L2  GUI assembly                          app/gui: gui_init, gui-vtable, session, splash, themes, unique service 8K
 L3  GUI frameworks                        app/widgets, app/display, app/tools, app/dialogs, app/actions,
                                           app/menus, app/propgui                                    397K
 L4  Core services                         app/core (the Gimp root object), app/config, app/pdb, app/plug-in,
                                           app/xcf, app/file, app/file-data, app/text, app/path       289K
 L5  Engine                                app/gegl, app/operations (custom GEGL ops), app/paint      59K
 L6  Public libraries (frozen C ABI)       libgimpbase -> math/color/thumb -> config -> module/widgets
                                           -> libgimp -> libgimpui                                    196K
 L7  External                              GLib/GObject/GIO >= 2.70, GTK 3.24, GEGL >= 0.4.66, babl, cairo,
                                           pango, lcms2, gexiv2, json-glib, appstream, libmypaint, ...

 Beside the stack:  plug-ins (separate processes over the wire protocol, 308K), modules (8K),
                    pdb generator, data, translations, packaging
```

The `gimp-console-3.3` binary is L0 + L1 (without `GimpApp`) + L4 + L5 + L6. The GUI binary adds L1's `GimpApp`, L2 and L3.

## 4. Run time, top to bottom

### 4.1 `main()` (`app/main.c:661-1077`)

| Step | Lines | What |
|---|---|---|
| 1 | 682-685 | Windows, relocatable builds: `gimp_windows_setenv()` sets `GSETTINGS_SCHEMA_DIR` through Win32 calls, **before GLib initializes** |
| 2 | 687-689 | Unstable builds: `gimp_attach_console_window()` (Windows) |
| 3 | 691-712 | macOS: drops the `-psn_*` arguments, then `gimp_macos_setenv()` (`:378-530`): `XDG_CACHE_HOME`, `GTK_IM_MODULE`, and the bundle relocation of `PATH`, `XDG_DATA_DIRS`, GIO, GDK-Pixbuf, GTK, fontconfig, GS, WMF, GI and Python paths |
| 4 | 714-722 | mingw-w64: takes `argc`/`argv` from `__argc`/`__argv` |
| 5 | 728 | `gimp_backtrace_init()` (`app/core/gimpbacktrace.h`) |
| 6 | 731 | `gimp_init_signal_handlers(&backtrace_file)` (`app/signals.c:53`) |
| 7 | 733-820 | Windows: the installer mutex `GIMP-<mutex version>`, `PANGOCAIRO_BACKEND=fc`, `SetDllDirectoryW`, `PATH`, DEP, `SetCurrentProcessExplicitAppUserModelID` |
| 8 | 822-830 | `gimp_init_malloc()` (a no-op today), `gimp_env_init(FALSE)` (libgimpbase relocation and `LD_LIBRARY_PATH`), `gimp_log_init()` (`GIMP_DEBUG` flags), `gimp_init_i18n()` (`:1328`: `setlocale`, the `gimp30-libgimp` and `gimp30` gettext domains plus `iso_639_3`; macOS `LC_MONETARY`/`LC_NUMERIC` workarounds), `g_set_application_name()` |
| 9 | 832-838 | `argv` becomes a GLib copy (`g_win32_get_command_line()` on Windows); `g_set_prgname(GIMP_DESKTOP_NAME)` |
| 10 | 840-878 | **Pre-scan**: `--verbose`/`-v` sets verbose; `--no-interface`/`-i` sets no-interface; `--version`/`-v` prints the version and exits (so `-v` prints the *verbose* version); Windows attaches the console for `--help*` |
| 11 | 880-882 | The console binary forces `no_interface` |
| 12 | 884-920 | `GOptionContext` with `main_entries` (`:165-328`, 29 entries); an empty `gimp` group whose **post-parse hook** runs `gimp_early_configuration()` (`:578`: `GimpEarlyRc` reads the language and the Win32 pointer API from gimprc, then `language_init()`); `app_libs_init()` (`app/app.c:116`) adds the GEGL group and, in the GUI, `gui_libs_init()` adds the GTK group (**whose hook opens the display**); then `g_option_context_parse_strv()`. Some callbacks exit inside the parser: `--license`, `--dump-gimprc*` and `--dump-pdb-procedures-deprecated` (`:1205-1300`); the two dumps create a temporary `Gimp` first. |
| 13 | 922-968 | GUI on flatpak (not on X11): `g_set_prgname()` from `/.flatpak-info` |
| 14 | 970-974 | Console attach when needed; `no_interface` implies `new_instance` |
| 15 | 976-1029 | GUI, unless `--new-instance`: `gimp_unique_open()` sends the files (and `gimp_unique_batch_run()` the batch commands) to a running instance over D-Bus (`org.gimp.GIMP.UI`) or Win32 `WM_COPYDATA`, then returns. On Windows a `Local\gimp_primary_instance_running` mutex is retried 20 times. |
| 16 | 1031-1033 | `sanity_check_early()` (`app/sanity.c:58`): versions of GIMP, GLib, cairo, pango, fontconfig, freetype, gdk-pixbuf, lcms, gexiv2, babl, GEGL and the filename encoding; `app_abort()` on failure |
| 17 | 1041-1063 | `app_run()` with **23 arguments** |
| 18 | 1065-1076 | `gimp_env_exit()`, frees |
| — | 1088-1095 | `WinMain()` for MSVC builds, which calls `main()` |

### 4.2 `app_run()` (`app/app.c:179-347`)

1. A single directory argument becomes `default_folder` (`:211-229`).
2. `gimp_new()` (`app/core/gimp.c:610`) creates the root object.
3. `gimp_app_new()` (GUI) or `gimp_console_app_new()` creates the `GApplication`, and `gimp->app` points to it (`:251-264`).
4. CPU acceleration; when the user directory is missing, `GimpUserInstall` runs, through `user_install_dialog_run()` in the GUI (`:268-289`).
5. `gimp_load_config()` (`:291`): unitrc, then the system and user gimprc into `GimpRc`; `edit_config` is kept in sync.
6. `errors_init()` (`:303`), `sanity_check_late()` (GEGL operations, `:309`), `gimp_gegl_init()` (`:314`).
7. Connects `exit` (after) to `app_exit_after_callback()`, which calls `g_application_quit()`, and `activate` to `app_activate_callback()`.
8. `g_application_run(app, 0, NULL)`. GApplication does not parse `argv`; uniqueness is GIMP's own (`G_APPLICATION_NON_UNIQUE`).
9. Teardown (`:325-346`): exit status, main context drained, app unref, `gimp_gegl_exit()`, `errors_exit()`, gimp unref, `gimp_debug_instances()`, `gegl_exit()`.

### 4.3 The `GApplication` layer

| Type | File | Base | Purpose |
|---|---|---|---|
| `GimpCoreApp` | `app/gimpcoreapp.c:52` | interface | Properties `gimp`, `filenames`, `as-new`, `quit`, `batch-interpreter`, `batch-commands`, kept in qdata; exit status |
| `GimpConsoleApp` | `app/gimpconsoleapp.c` | `GApplication` | The console variant |
| `GimpApp` | `app/gui/gimpapp.c:56` | `GtkApplication` | The GUI variant, plus `no-splash`; id `org.gimp.GIMP`, `G_APPLICATION_NON_UNIQUE` |

`gimp->app` is read by `core/gimp.c:1383` (whether `--quit` was given), `display/gimpimagewindow.c:454-522` (the menubar), `display/gimpdisplayshell-tool-events.c:2496`, `menus/menus.c:444` (shortcuts) and `widgets/gimpuimanager.c:1174-1240`, which registers all `<Image>` actions as `app.*` and their accelerators.

### 4.4 `activate` (`app_activate_callback()`, `app/app.c:374-577`)

1. `language_init(NULL, ...)` returns the actual language.
2. GUI: `gui_init()` (`app/gui/gui.c:231`):
   - GTK version check, `gui_unique_init()` (D-Bus service);
   - translations, icon themes, `gimp_widgets_init()`, DnD, themes, canvas styles;
   - the splash;
   - connects the Gimp `initialize`/`restore`/`exit` handlers. It returns `splash_update` as the status callback.
3. `gimp_initialize()` (`gimp.c:834`) emits `initialize`:
   - core: internal data, `internal_procs_init()` (803 procedures), PDB compat, plug-in manager initialization;
   - GUI (after): `gimp_tools_init()`.
4. `query_all` when the language changed; `gimp_restore()` (`gimp.c:857`) emits `restore`:
   - GUI (before): `gui_vtable_init()` (`gui.c:556`), actions, menus, dialogs, clipboard, devices, controllers, modifiers, session;
   - core: parasiterc, data factories, templates, modules, plug-in manager restore (pluginrc, queries, `-init`, persistent plug-ins);
   - GUI (after): recent documents, UI manager, `menus_restore()` (shortcutsrc), splash destroy, the empty display, `session_restore()`.
5. gimprc autosave on; GUI crash recovery (`errors_recovered()`, `gui_recover()`).
6. `gimp_update_auto_check()`; `config-version`/`prev-language`.
7. `file_open_from_command_line()` for every file; `gimp->initialized = TRUE`.
8. The font error message; `gimp_batch_run()`.
9. `--quit` calls `gimp_exit(gimp, TRUE)`. Without it, console mode installs a SIGINT (or Ctrl-C) handler that calls `gimp_exit()` and holds the application.

### 4.5 Quit

`gimp_exit()` (`gimp.c:914`) emits `exit`, which has a "true handled" accumulator:
1. `gui_exit_callback()` may cancel it with the quit dialog when images are dirty; otherwise it saves the session, devices, controllers and menus and deletes the displays.
2. The core class handler (`gimp_real_exit()`, `:590`) shuts the plug-ins and extensions down and saves data, templates, parasiterc and unitrc.
3. `app_exit_after_callback()` quits the `GApplication`.
4. `gui_exit_after_callback()` tears the GUI down.

`app_run()` then finishes the teardown (4.2, step 9).

### 4.6 Signals and fatal errors

**`signals.c`:**
- Computes the crash log path: `<gimpdir>/CrashLog/gimp-crash-<time>.txt`, or the local data directory on Windows.
- **POSIX:**
  - SIGHUP, SIGINT, SIGQUIT, SIGTERM call `gimp_terminate()`;
  - SIGABRT, SIGBUS, SIGSEGV, SIGFPE call `gimp_fatal_error()` (not on macOS);
  - SIGPIPE is ignored;
  - SIGCHLD keeps the default action with `SA_RESTART`;
  - everything goes through `gimp_signal_private()` (`sigaction`).
- **Windows:** `SetUnhandledExceptionFilter()` plus Dr. MinGW (`ExcHndlInit()`).

**`errors.c`:**
- `errors_init()` routes the `Gimp*`/`LibGimp*`/`GEGL` log domains (WARNING, MESSAGE, CRITICAL) to `gimp_show_message()` according to the `debug-policy` preference. `G_LOG_LEVEL_ERROR` becomes `gimp_fatal_error()`.
- `gimp_eek()` prints the reason. When a GUI is up and the policy allows, it writes a backtrace and spawns `gimp-debug-tool-3.3` (`GIMP_TOOL_VERSION`). Otherwise it follows `--stack-trace-mode` (`never`/`query`/`always`). On Windows it shows a `MessageBox`. Then `exit(EXIT_FAILURE)`.
- `errors_recovered()` lists `<cache>/images/image-<id>/wlbr-project.xml` and reserves those image IDs.
- Note: `gimp_eek()` dereferences `the_errors_gimp->config`, so a fatal signal **before** `errors_init()` crashes inside the handler.

### 4.7 The other glue files

| File | Lines | Public API | Callers outside `app/*.c` |
|---|---|---|---|
| `sanity.c` | 738 | `sanity_check_early()`, `sanity_check_late()` | none |
| `language.c` | 933 | `language_init()`, `language_set_macos_menu_bar_lang()` | none. A 700-line Win32 LANGID table; Objective-C (`NSLocale`) at `:817-828` |
| `unique.c` | 335 | `gimp_unique_open()`, `gimp_unique_batch_run()` | none. D-Bus client (Linux), `WM_COPYDATA` (Windows), always FALSE on macOS |
| `gimp-update.c` | 679 | `gimp_update_auto_check()`, `gimp_update_check()`, `gimp_update_refresh()` | `app/dialogs/about-dialog.c`. json-glib over GIO; `NSURLSession` in Objective-C on macOS (`:604-675`) |
| `gimp-version.c` | 359 | `gimp_version_show()`, `gimp_version()`, `gimp_version_get_revision()`, `gimp_version_check_update()` | `widgets/gimpcriticaldialog.c`, `widgets/gimpdashboard.c`, `dialogs/{about,welcome,preferences}-dialog.c` |
| `gimp-log.c` | 219 | `gimp_log_flags`, `GIMP_LOG()`, `gimp_log_set_handler()` | everywhere (21 debug keys from `GIMP_DEBUG`, 30 log domains) |
| `gimp-debug.c` | 122 | `gimp_debug_add/remove_instance()`, `gimp_debug_instances()` | `core/gimpobject.c`, `config/gimpgeglconfig.c` |
| `gimpcoreapp.c` | 327 | the `GimpCoreApp` interface | `core/gimp.c`, `gui/gimpapp.c`, `app/tests`, `libgimp/gimpparamspecs-body.c` |
| `gimpconsoleapp.c` | 84 | `gimp_console_app_new()` | `app.c` |

## 5. The layers below

### 5.1 Core: the `Gimp` root object (`app/core`, 241 files, 146K lines of `.c`/`.cc`)

**Life cycle of `Gimp`** (a `GimpObject`, `gimp.c:146`):

| Function | Lines | What it does |
|---|---|---|
| `gimp_init()` | 240 | parasites, enums, units, containers (images, displays, buffers, tools, documents, templates), three ID tables, eight data factories |
| `gimp_constructed()` | 312 | modules, 12 paint cores, the extension manager, the plug-in manager, the PDB, `xcf_init()`, `file_data_init()`, the default and user contexts |
| `gimp_new()` | 610 | — |
| `gimp_load_config()` | 761 | — |
| `gimp_initialize()` | 834 | — |
| `gimp_restore()` | 857 | — |
| `gimp_exit()` | 914 | — |

**Signals:** `initialize`, `restore`, `exit`, `clipboard-changed`, `filter-history-changed`, `image-opened`, `focused-once`.

**Class hierarchy** (abbreviated):

```
GObject
├ GimpObject
│ ├ Gimp, GimpPDB, GimpPlugIn(Def/Manager), GimpExtension(Manager), GimpDisplay (base; GUI subclass in app/display)
│ ├ GimpViewable [GimpConfig]
│ │ ├ GimpFilter ─ GimpItem [Savable] ─┬ GimpDrawable [Pickable, ColorManaged] ─┬ GimpLayer ─ Group/Text/Vector/Link
│ │ │                                  │                                      └ GimpChannel ─ Selection, LayerMask, ...
│ │ │                                  └ GimpPath
│ │ ├ GimpImage [Projectable, Pickable, ColorManaged, Savable]
│ │ ├ GimpContext ─ GimpToolOptions ─ GimpPaintOptions (+9); GimpPDBContext; GimpFillOptions ─ GimpStrokeOptions
│ │ ├ GimpResource ─ GimpData [Tagged, Savable] ─ Brush, Pattern, Gradient, Palette, Dynamics, Mybrush, Font, ...
│ │ ├ GimpUndo ─ GimpItemUndo (20), GimpUndoStack, ...
│ │ └ GimpProcedure ─ GimpPlugInProcedure ─ GimpTemporaryProcedure
│ ├ GimpContainer [GListModel] ─ GimpList ─ ... ─ GimpItemStack ─ GimpDrawableStack ─ GimpLayerStack
│ ├ GimpDataFactory, GimpProjection [Pickable], GimpItemTree, GimpIdTable, GimpTagCache, ...
│ └ GimpPaintCore ─ GimpBrushCore ─ Paintbrush ─ Airbrush/Eraser/Pencil; SourceCore ─ Clone/Heal; Ink; ...
├ GimpAuxItem ─ Guide, SamplePoint; GimpAsync, GimpAsyncSet [Waitable, Cancelable]
└ GimpGeglConfig ─ GimpCoreConfig ─ GimpDisplayConfig ─ GimpGuiConfig ─ GimpDialogConfig ─ GimpPluginConfig ─ GimpRc
```

**Type counts:**

| Directory | Types |
|---|---|
| `app/core` | 110 types, 8 interfaces |
| `app/config` | 11 |
| `app/operations` | 65 |
| `app/paint` | 29 |
| `app/path` | 9 |
| `app/plug-in` | 7 |
| `app/text` | 6 |
| `app/pdb` | 3 |
| `app/gegl` | 2 |
| `app/xcf`, `app/file`, `app/file-data` | 0 (plain functions) |

**Inversion points** (how the core reaches upward without including GUI headers):
- **The `GimpGui` vtable** (`app/core/gimp-gui.h:23`) has 27 function pointers: ungrab, busy, messages, help, display create/delete, progress, PDB dialogs, recent list, mount operation, profile and rotation policies. It is embedded in `Gimp` and filled by `gui_vtable_init()` during `restore`; the console never fills it.
- **Interfaces:** the `GimpProgress` interface, the `GimpDisplay` base type, and the `GimpCoreApp` interface (`core/gimp.c` includes the top-level `gimpcoreapp.h`, **an upward dependency from core to the glue**).
- **Messages:** `gimp_message()` → `gimp->gui.show_message`.
- **Opaque GUI fields** on `Gimp`: `controller_manager`, `image_windows`.

**Process-wide state:**
- `the_unit_gimp` (`app/core/gimp-units.c:55`) plus the libgimpbase unit vtable: only one `Gimp` per process.
- The `GimpData` ID table; the "standard" data singletons (`gimpbrush.c:565`, `gimppattern.c:303`, ...).
- The `gimp-parallel` thread pool; the layer-mode caches and SIMD dispatch pointers.
- The wire protocol handler table (`libgimpbase/gimpwire.c:41-45`).

### 5.2 Config (`app/config`, 11 types)

**Classes:** one `GimpRc` instance carries every setting, through the chain `GimpGeglConfig` → `GimpCoreConfig` (about 78 properties) → `GimpDisplayConfig` → `GimpGuiConfig` → `GimpDialogConfig` → `GimpPluginConfig` → `GimpRc`.

**Serialization:**
- It goes through libgimpconfig (`GScanner`, `GimpConfigWriter`).
- `GimpRc` writes only the difference to the system gimprc.
- Changes autosave through an idle.

**Two details:**
- `GimpEarlyRc` is the tiny early reader used by `main()`.
- `app/config` and `app/core` include each other (a cyclic static library pair).

### 5.3 Engine (`app/gegl`, `app/operations`, `app/paint`)

**`gimp_gegl_init()`** (`app/gegl/gimp-gegl.c:54`):
- mirrors `GimpGeglConfig` into `gegl_config()`;
- starts `gimp_parallel_init()` and `gimp_babl_init()`;
- registers 56 custom `gimp:*` operations: 28 general, and 28 layer modes of which 18 are legacy (`app/operations/gimp-operations.c:123`).

**The 9 existing C++ files of `app/`:**
- Where they are: `core/gimp-parallel.cc`, `core/gimpbrush-mipmap.cc`, `core/gimpbrush-transform.cc`, `core/gimppickable-contiguous-region.cc`, `gegl/gimp-gegl-loops.cc`, `gegl/gimp-gegl-mask-combine.cc`, `operations/gimpoperationmaskcomponents.cc`, `paint/gimpbrushcore-loops.cc`, `paint/gimppaintcore-loops.cc`.
- What they use: templates for per-pixel-format specialization, and lambdas for GEGL's `gegl_parallel_distribute*`. `paint/gimppaintcore-loops.cc` alone has 115 templates.
- What they don't use: the C++ standard library. There is no `std::`, and the only standard header is `<new>` (placement new), included by `core/gimp-parallel.h`.
- How they include the C headers: inside `extern "C" {}`, either the includes only or the whole file.
- `core/gimp-parallel.h:40-153` adds `extern "C++"` template overloads, the only C++-aware header pattern in `app/`.

### 5.4 PDB and plug-ins (`app/pdb`, `app/plug-in`, `pdb/`)

**The PDB:**
- `GimpPDB` maps procedure names to `GimpProcedure`s.
- 803 internal procedures are generated from `pdb/groups/*.pdb` (61 files, 34K lines) by `pdbgen.pl` with `app.pl` (core side) and `lib.pl` (libgimp side).
- That gives 63 generated files in `app/pdb` and 121 in `libgimp`, marked "auto-generated by pdbgen.pl".

**The plug-in manager:**
- `initialize`: interpreters, environment, shared memory.
- `restore` (`gimppluginmanager-restore.c:90`): pluginrc (format 18), query, `-init`, persistent plug-ins.

**Spawning a plug-in** (`gimp_plug_in_open()`, `app/plug-in/gimpplugin.c:439`):
- two pipes;
- the arguments `-gimp <protocol> <rfd> <wfd> -query|-init|-run <stack-trace-mode>`.

**The wire protocol** (`libgimpbase/gimpprotocol.c`, `gimpwire.c`):
- `GIMP_PROTOCOL_VERSION 0x0119`, 13 message types;
- tiles inline or through shared memory (SysV, POSIX or Win32, `-Dshmem-type`; `--no-shm`).
- Synchronous calls run a nested main loop.

### 5.5 Files (`app/xcf`, `app/file`, `app/file-data`)

- `xcf_init()` registers `gimp-xcf-load`/`gimp-xcf-save` as internal plug-in procedures; versions 0-26 load through `xcf_load_image()`.
- `file-open.c` and `file-save.c` pick a procedure and run it through the PDB; `file_open_from_command_line()` (`:658`) serves `main()`.
- `file-data` registers the gbr/gih/pat/gex procedures.

### 5.6 GUI (`app/gui`, `widgets`, `display`, `tools`, `actions`, `menus`, `dialogs`, `propgui`)

**`app/gui`:** assembles the user interface (4.4).
- `gimpapp.c`: the `GtkApplication`.
- `gui-vtable.c`: the 27 core callbacks.
- `session.c` (sessionrc, dockrc), `splash.c`, `themes.c` (CSS, xdg-desktop-portal), `icon-themes.c`.
- `gui-unique.c`: the D-Bus server `org.gimp.GIMP.UI` (gdbus-codegen from `dbus-service.xml`), a Win32 window or Apple events.
- `gui-message.c`, `modifiers.c`, and `gimpuiconfigurer.c` (single- vs multi-window).

**`app/widgets`** (248 files, 210 types, 158K lines):
- **Dialogs and docking:** `GimpDialogFactory` with `GimpSessionInfo`; the docking stack `GimpDockWindow` ⊃ `GimpDockColumns` ⊃ `GimpDock` ⊃ `GimpDockbook` ⊃ `GimpDockable`.
- **Actions** over `GAction`: `GimpAction`, `GimpActionGroup`, `GimpActionFactory`.
- **Menus** over `GMenuModel`/GtkBuilder: `GimpUIManager`, `GimpMenuModel`; 38 menu managers, 36 `.ui` files in `menus/`.
- **Views and editors:** `GimpContainerView` (15 vfuncs), `GimpEditor`, `GimpViewRenderer` (9 subclasses).
- **Input:** controllers and devices.

**`app/display`** (90 files, 55 types):
- `GimpDisplayImpl` (the GUI side of the core `GimpDisplay`) and `GimpImageWindow`.
- `GimpDisplayShell`: 2,469 lines plus 30 `gimpdisplayshell-*.c` parts.
- The canvas: `GimpCanvas` and about 26 `GimpCanvasItem`s.
- `GimpToolWidget` (about 13 tool widgets).
- The render path: projection → `gegl_buffer_get` → color transform → cairo.

**`app/tools`** (114 files, 98 types):
- `GimpTool` with 19 vfuncs, and `GimpToolControl`.
- 48 `*_tool_register` functions in `gimp_tools_init()` (`gimp-tools.c:131`).
- Paint tools instantiate the paint core named in the core `GimpPaintInfo`.

**`app/actions`:**
- 46 action groups (`actions.c:112`);
- 48 `*-actions.c` files (entry tables, setup and update);
- 44 `*-commands.c` files (callbacks).

**`app/dialogs`:** about 100 dialog factory entries (`dialogs.c:221-452`), the 3,719-line preferences dialog, and about/welcome/user-install (used by the glue).

**`app/propgui`:** GTK widgets built from `GParamSpec`s, with 18 operation-specific builders.

**Coupling:** the GUI directories include each other in cycles: widgets ↔ display, widgets ↔ tools, display ↔ tools, actions → dialogs, dialogs → gui. No core-side directory includes a GUI header.

### 5.7 Public libraries and the plug-in API

| Library | Purpose | Links | Exports (`.def`) |
|---|---|---|---|
| `libgimpbase` | Types and enums, env, units, parasites, metadata, **the wire protocol** | — | 310 |
| `libgimpmath` | Matrices, vectors | base | 78 |
| `libgimpcolor` | Color profiles and transforms (lcms2), parsing | base | 74 |
| `libgimpconfig` | `GimpConfig` serialization, scanner, writer | base, color, math | 101 |
| `libgimpmodule` | `GimpModule` (`GTypeModule`) | base, config | 20 |
| `libgimpthumb` | XDG thumbnails | base | 31 |
| `libgimpwidgets` | GTK3 widgets (59 types) | base, color, config | 535 |
| `libgimp` | The plug-in API: `GimpPlugIn`, procedures, PDB, image and item proxies (39K generated lines) | base, color, config, module | 1,188 |
| `libgimpui` | Procedure dialogs, choosers (built from `libgimp/`) | libgimp, widgets | 116 |

**What is installed:**
- 249 headers under `include/gimp-3.0/`;
- three pkg-config files: `gimp-3.0`, `gimpui-3.0`, `gimpthumb-3.0`;
- GObject Introspection: `Gimp-3.0.gir` (libgimp, base, color, config, math, module) and `GimpUi-3.0.gir` (libgimpui, widgets).

**ABI control:**
- `tools/defcheck.py` compares `nm -D` against the `.def` files on every platform (`check-def-files`).
- Internal symbols use `G_GNUC_INTERNAL`; there is no `-fvisibility=hidden`.
- The installed headers have 33 derivable class structs (`G_DECLARE_DERIVABLE_TYPE`) with 340 reserved vfunc slots (`_gimp_reserved*`).

### 5.8 Plug-ins, modules, extensions

**Plug-ins** (307K lines of C):
- `common/` holds 86 single-file plug-ins.
- The largest others: imagemap, script-fu (the TinyScheme interpreter, 75 scripts, 82 tests), file-psd, file-dds, gfig, flame, metadata, gimpressionist, file-tiff, ...
- There are 127 C `GimpPlugIn` subclasses and 14 Python plug-ins; `file-exr/openexr-wrapper.cc` is C++ behind a C API.
- Lists are maintained by hand in `plug-ins/meson.build` and `plug-ins/common/meson.build`.

**Modules:**
- color selectors (cmyk, water, wheel);
- display filters (clip-warning, color-blind, aces-rrt, gamma, high-contrast);
- controllers (linux-input, dx-dinput, midi).

All use `G_DEFINE_DYNAMIC_TYPE` with `gimp_module_query`/`register`; the module ABI is `0x0005`.

## 6. Dependency structure

Outgoing includes between the `app/` units, excluding `*-types.h`/`*-enums.h` and the small `app/` utility headers. The counts are `#include` lines:

| From | To (count) |
|---|---|
| `app(top)` | core 19, config 6, gui 4, pdb 4, dialogs 3, gegl 1, file 1 |
| `config` | core 8 |
| `gegl` | core 14, operations 2, config 1 |
| `operations` | core 22, gegl 6 |
| `paint` | core 150, gegl 23, operations 6, path 2 |
| `path` | core 67, paint 2, config 1, gegl 1 |
| `text` | core 68, gegl 5, path 3 |
| `core` | gegl 118, path 39, config 32, text 17, operations 12, paint 7, pdb 7, plug-in 4, app(top) 2, file 2, xcf 1, file-data 1 |
| `pdb` | core 399, plug-in 30, text 17, path 14, gegl 8, operations 8, paint 7, file 4, config 3 |
| `plug-in` | core 64, pdb 16, config 3, gegl 3, path 2, text 2, file 1 |
| `xcf` | core 76, path 14, gegl 5, plug-in 4, text 4, config 2, operations 2 |
| `file` | core 34, plug-in 4, config 2, xcf 2, pdb 2, gegl 1, text 1 |
| `widgets` | core 706, config 45, plug-in 14, display 12, pdb 11, text 11, gegl 10, path 10, menus 7, tools 6, file 6, ... |
| `display` | core 206, widgets 122, config 24, tools 14, path 11, menus 8, gegl 6, ... |
| `tools` | core 493, widgets 230, display 213, config 36, paint 32, gegl 23, ... |
| `actions` | core 343, widgets 324, display 68, dialogs 47, tools 27, config 21, ... |
| `dialogs` | widgets 194, core 139, config 18, display 10, file 7, gui 5, ... |
| `gui` | widgets 60, core 42, display 21, config 10, dialogs 5, plug-in 4, ... |

**Findings:**
- **The core side is a strongly connected component.** `core` reaches path, text, paint, pdb, plug-in, xcf, file and file-data, mostly from `core/gimp.c` (managers it creates in `gimp_constructed()`) and `core/gimpimage*.c` (text and vector layers). These only link because Meson groups the static libraries.
- **The GUI side is a second strongly connected component:** widgets ↔ display ↔ tools ↔ actions ↔ menus ↔ dialogs ↔ gui.
- **Nothing on the core side includes a GUI header.** The only bridges are the 4.3/5.1 inversion points.
- **The glue (`app(top)`) is the only place that sees both components**, plus one upward edge from core: `gimpcoreapp.h`.

**Consequence for the transition:** the layers can be replaced top-down. The glue sits on top of both components and has almost no callers. Inside each component, the work is per subsystem, not per file.

## 7. C++ readiness

| Topic | Finding |
|---|---|
| `extern "C"` guards | **None of the 1,144 `app/` headers has `G_BEGIN_DECLS`.** 238 of the 249 installed library headers do. The real gaps, prototypes outside any guard: `libgimpcolor/gimpcairo.h`, `libgimpconfig/gimpcolorconfig.h`, `gimpconfigwriter.h`, `gimpscanner.h`, `gimpconfigenums.h`, `libgimpwidgets/gimpcairo-utils.h`. They compile from C++ but **fail to link** unless included inside `extern "C"`. |
| C++ keywords as identifiers, headers | 28 `app/` headers: `private` fields in 18 (`core/gimpdrawable.h:38`, `display/gimpcanvasitem.h`, 11 `display/gimptool*.h`, `paint/gimpmybrushcore.h`, `text/gimptextlayer.h`, `tools/gimprectangleselecttool.h`, `widgets/gimppickablebutton.h`, `widgets/gimpwindow.h`); `template` in 7 (`core/gimptemplate.h`, `core/gimpcontext.h:101` as a field, `core/gimpimage-new.h`, `widgets/gimpdockwindow.h`, `widgets/gimptemplateeditor.h`, `dialogs/image-new-dialog.h`, `dialogs/template-options-dialog.h`); `new` (`display/gimpdisplay-foreach.h:27`); `export` (`dialogs/file-save-dialog.h:25`, `widgets/gimpsettingsbox.h:46`). The public library headers have none. |
| C++ keywords as identifiers, sources | 153 `app/` `.c` files: `private` 9,138 times, `template` 330, `this` 59, `new` 57, `class` 55, `export` 14, plus `try`, `virtual`, `mutable`, `namespace`. This only matters when C files are compiled as C++ (doc 04). |
| Other C-only constructs in headers | `propgui/propgui-types.h:59-75` uses a compound literal with designated initializers. |
| The top-level closure | `main.c` reaches 45 project headers and `app.c` 58, with no clash. `errors.c` reaches `core/gimpdrawable.h` (`private`) through an include it does not need. `gui/gui.c` reaches `core/gimpcontext.h` (`template` field) and `display/gimpdisplay-foreach.h` (`new` parameter). |
| Objective-C inside `.c` files | `app/language.c`, `app/gimp-update.c`, `app/gui/gui.c`, `gui/gui-unique.c`, `menus/menus.c`, `display/gimpimagewindow.c`, `widgets/gimpdockwindow.c`, `widgets/gimpwidgets-utils.c`, `widgets/gimpcriticaldialog.c`, `widgets/gimpsearchpopup.c`, `dialogs/about-dialog.c`, `core/gimp-user-install.c`, `file/file-remote.c`, `plug-in/gimppluginmanager-call.c`, and 11 library files. They compile as Objective-C on macOS only, so a C++ port must keep that code in C/Objective-C units. |
| Side effects inside option parsing | The `gimp` group hook (early gimprc, language) and the GTK group hook (display), in group order; `exit()` inside four option callbacks. |
| `exit()` calls | `app_exit()`, `app_abort()`, `gui_abort()`, `gimp_eek()`, and `gimp_batch_exit_after_callback()` (`core/gimp-batch.c`), connected at `:155`. |
| Global state in the glue | the option statics (`main.c:129-162`), `the_errors_gimp` and friends (`errors.c:60-66`), `language_init()` statics, `the_gui_gimp` (`gui.c:170`). |

## 8. Platforms

| Platform | Specifics in the glue |
|---|---|
| Linux / BSD | D-Bus single-instance client (`unique.c`) and server (`gui/gui-unique.c`); flatpak `prgname`; the relocatable `LD_LIBRARY_PATH` (`gimp_env_init()`) |
| Windows | `WinMain()`; `win_subsystem` `console`/`windows`; `.rc` files; `gimp_windows_setenv()` before GLib; console attaching; the installer mutex, DLL directory, DEP and AppUserModelID; Dr. MinGW; a `WM_COPYDATA` single instance; the LANGID language table |
| macOS | Objective-C inside `.c`; `-psn_` arguments; bundle relocation of the environment; `LC_NUMERIC`/`LC_MONETARY` workarounds through the Objective-C runtime (`main.c:532-572`); `NSURLSession` update checks; Apple events instead of D-Bus |

## 9. Root-relative build dependencies (for relocating the tree)

These assume that GIMP's root `meson.build` is the project root.

- **`meson.global_*_root()`:**
  - root `meson.build:2001-2002, 2037, 2040, 2044-2046, 2049, 2053-2076`;
  - `gimp-data/images/logo/meson.build:2`;
  - `tools/in-build-gimp.py`, which reads `GIMP_GLOBAL_SOURCE_ROOT`/`BUILD_ROOT`.
- **`meson.project_*_root()`:**
  - root `meson.build:1857, 1943`;
  - `pdb/meson.build:75-79, 91-93, 122, 147, 167`;
  - `app/{core,config,display,gegl,operations,paint,plug-in,text,tools,widgets}/meson.build:10` (gimp-mkenums);
  - `libgimpbase/meson.build:20,41`, `libgimpconfig/meson.build:11`, `libgimpwidgets/meson.build:10`;
  - `app/dialogs/meson.build:2-3,8-9,68`;
  - `app/tests/meson.build:63-65,92-96`;
  - `libgimp/meson.build:771`, `tools/meson.build:43`;
  - `po-tips/meson.build:5`, `po-windows-installer/meson.build:4-5`;
  - `devel-docs/reference/gimp-ui/meson.build:27`;
  - **26 places in `gimp-data/icons/Default/**/meson.build`** (plus the `global_source_root()` above), in a separate GNOME repository.
- **In total:** 97 calls in 36 `meson.build` files of the GIMP tree, and 27 in `gimp-data`.
- **`../` paths:** `tools/meson.build:39`, `docs/meson.build:80,85,90`, `devel-docs/reference/*/meson.build`, `plug-ins/script-fu/{interpreter,server}/meson.build:2`.
- **Run-time paths** built by the C code from `GIMP_TESTING_ABS_TOP_SRCDIR`: `app/tests/*.c`, `app/core/gimp-templates.c:63`, `app/gui/icon-themes.c:322`, `app/tests/gimp-app-test-utils.c:157-160`.
- **Outside the repository:** `build-gimpxx-{debug,release}.sh` and `docker-gimpxx.sh` next to it expect `gimpxx/meson.build`.

## 10. What this means for the transition

1. **The top-level layer is small, almost private and C++-clean**: 7.1K lines, 45 to 58 headers per file, no keyword clashes. It is the natural first step, and it is exactly the darknetxx shape: a template `main()`, an `ApplicationFactory` and adaptor classes over the original C code.
2. **The command line semantics hide in GOption hook order and in `exit()` calls**, so a differential parity harness is mandatory before the first change.
3. **The build depends on a working binary**, so the first C++ step must be a thin bridge.
4. **`gimp-version.c`, `gimp-update.c`, `gimp-log.c`, `gimp-debug.c` and `gimpcoreapp.c` have callers below the glue**, so they keep their C ABI during the top-level work.
5. **Below the glue, two strongly connected components (core side, GUI side) are tied together only through vtables, interfaces, signals and `gimp->app`.** This makes "GUI first, then core, then engine" a workable order (doc 04).
6. **The public libraries, the GIR, the wire protocol and the module ABI are frozen**, so C++ may enter them only as a hidden implementation (doc 04).
7. **The template layout with the tree under `src/gimp/` needs the root-relative Meson calls rewritten, and `gimp-data` patched.**
