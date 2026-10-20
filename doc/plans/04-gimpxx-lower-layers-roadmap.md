# gimpxx: the roadmap below the top-level layer

**Status:** roadmap. It orders the work that follows the [top-level layer](03-gimpxx-top-level-layer-design.md), and states the rules that hold for all of it. When a layer starts, it gets its own detailed plan, in the style of doc 03.

**Analyzed on 2026-10-09**, against gimpxx `master` `b7f8cac066`. The sizes come from [doc 01](01-gimpxx-architecture-analysis.md).

## 1. Principles

1. **Top-down, one layer at a time.** Each layer ends the way the top-level layer does: parity, the Meson tests and the new tests all pass.
2. **Two moves per subsystem, in this order.**
   - **C as C++:** the files are renamed `.c` → `.cc` (`git mv`, so `git log --follow` keeps the history) and fixed until they compile as C++, without a design change.
   - **Redesign:** C++ classes replace C code where that pays off.

   Separating the two keeps every diff reviewable and every behavior change visible.
3. **GObject stays wherever GObject is the contract**, and is then *implemented in C++*, not replaced. That covers:
   - types that GTK, GEGL or the GIR look up by `GType` or by name (custom widgets, `gimp:*` operations, modules);
   - classes whose properties are serialized (gimprc, sessionrc, tool presets);
   - classes whose signals drive the GUI (`GimpViewable`, `GimpContainer`, `GimpImage`, ...).

   Plain C++ classes replace GObject only for internal objects that never cross the type system.
4. **The rule for C++ inside a GObject:** the instance and class structs stay standard-layout C structs, with no virtual functions and no non-trivial members, because GType allocates them zero-filled and never runs a C++ constructor or destructor. The C++ state lives behind a raw private pointer (`new` in `*_init()`, `delete` in `*_finalize()`), or in a `G_ADD_PRIVATE` struct that `*_init()` placement-constructs and `*_finalize()` destroys explicitly.
5. **The public contract is frozen** (doc 02, C2):
   - the `libgimp*-3.0` C ABI and API, with the `.def` files and `tools/defcheck.py`;
   - `Gimp-3.0.gir` and `GimpUi-3.0.gir`;
   - wire protocol `0x0119` and module ABI `0x0005`;
   - the gimprc, sessionrc, pluginrc and XCF formats.
6. **Headers first.** Before a layer converts, its headers become includable from both C and C++ (track T1). Both kinds of callers then coexist during the conversion.

## 2. Tracks that run beside the layers

### T1. Header hygiene (prerequisite of every layer, upstreamable)

- **`G_BEGIN_DECLS`/`G_END_DECLS` in all 1,144 `app/` headers.** A mechanical commit per directory, verified by a generated C++ translation unit that includes every header of the directory.
- **The 28 `app/` headers with C++ keyword identifiers:**
  - Prototype parameter names are free to rename: `template` → `tmpl`, `new` → `new_image`, `export` → `exporting`.
  - Struct fields need their uses changed: `GimpTemplate *template` (`core/gimpcontext.h:101`), and the `private` fields of 18 headers, among them `GimpDrawablePrivate *private` (`core/gimpdrawable.h:38`, 129 uses in 5 files). The others are `display/gimpcanvasitem.h`, 11 `display/gimptool*.h`, `paint/gimpmybrushcore.h`, `text/gimptextlayer.h`, `tools/gimprectangleselecttool.h`, `widgets/gimppickablebutton.h` and `widgets/gimpwindow.h`.
  - The usual GIMP spelling is `priv`.
- **The six public headers whose prototypes sit outside `extern "C"`:** `libgimpcolor/gimpcairo.h`, `libgimpconfig/gimpcolorconfig.h`, `gimpconfigwriter.h`, `gimpscanner.h`, `gimpconfigenums.h` and `libgimpwidgets/gimpcairo-utils.h`. Today they compile from C++ but fail to link.
- **`propgui/propgui-types.h:59-75`:** the compound literal becomes a function.

**Gate:** a `gimpxx-headers` test suite compiles one C++ file per directory that includes all of its headers.

### T2. C-as-C++ readiness, per directory

The C constructs that C++ rejects, and that GObject code is full of:

| Construct | Typical place | Fix |
|---|---|---|
| implicit `void*` → `T*` | `g_malloc`, `g_new` (fine), `g_object_new`, `g_hash_table_lookup`, `gpointer` callback data | the `GIMP_FOO()` cast macros, `static_cast` |
| C++ keywords as identifiers | `private` 9,138 times in 153 `app/` `.c` files; `template` 330, `this`, `new`, `class`, `export`, `try`, `virtual`, `mutable`, `namespace` | rename (`priv`, `tmpl`, ...) |
| enum arithmetic and int ↔ enum | flag OR-assignments on enum-typed variables | casts or `int` locals |
| `goto` over initializations | error paths | scopes |
| string literals into `gchar *` | tables | `const` |
| C99 compound literals, VLAs, designated initializers out of order or on arrays | tables, local buffers | constructors, `std::array`, `g_alloca` |
| Objective-C in `.c` (macOS, doc 01 section 7) | GUI files | move it into `.m` helpers behind C functions |

**Tool:** GCC's `-Wc++-compat` flags most of these *while the files are still C*. Enabling it per directory (one Meson `c_args` change) and fixing its warnings in C is the cheap first step of every conversion. The rename to `.cc` is then a small final commit.

### T3. Ownership helpers

Extend `src/helpers/glib` as the layers need it: `GListHolder<T>`, `GHashTableHolder`, `GBytesPtr`, signal and idle/timeout source holders, and a `GObjectPtr` that works with `G_TYPE_CHECK_INSTANCE_CAST`.

### T4. Tests before conversion

GIMP has 21 Meson tests. Before a layer converts, add the tests that will tell whether it still works:
- PDB-driven Python and C tests through the existing `libgimp/tests` runner;
- XCF round trips over a corpus;
- pixel-exact output hashes of the `gimp:*` operations and the paint cores;
- GUI smoke runs under `xvfb-run`.

### T5. Tooling

clang-tidy (`modernize-*`, `cppcoreguidelines-*` gradually), the template's sanitizer options on the converted directories, and compile-time and binary-size tracking. Performance checks use GIMP's own `tools/performance-log-*`.

### T6. The upstream cut-off

Once a layer is renamed to `.cc`, upstream changes to it no longer merge and must be ported by hand. Decide per layer whether to:
- keep syncing (port the upstream changes); or
- freeze, and from then on only cherry-pick fixes.

Doc 02 lists the cadence as an open decision.

## 3. The layers, in order

The `R` numbers below are roadmap steps. The `L0`-`L7` numbers of [doc 01](01-gimpxx-architecture-analysis.md) name the architecture layers. They map as follows:
- R1 is what remains of L0, plus L1 and L2;
- R2 and R3 are L3;
- R4 is L4 without the generated PDB code, and R6 is the generator of that code;
- R5 is L5;
- R7 is L6;
- R8 and R9 sit beside the stack.

### R1. The rest of the glue and the GUI assembly

**Scope:**
- `app/gui`: 12 files, 8K lines; `GimpApp`, `GimpDBusService`, `GimpUIConfigurer`.
- `app/gimpcoreapp.c` and `app/gimpconsoleapp.c`.
- `app/gimp-version.c`, `app/gimp-update.c`, `app/gimp-log.c` and `app/gimp-debug.c` (left C by doc 03).

**What is special:**
- `gui_init()` connects the Gimp `initialize`/`restore`/`exit` handlers.
- `gui-vtable.c` fills the 27 `GimpGui` function pointers during `restore`.
- The D-Bus server (gdbus-codegen) lives here.
- So does the Objective-C of `gui.c` and `gui-unique.c`.

**Approach:** a `src/gui` component, as in the template's `appGtkmm3` (`src/gtkmm3`) and darknetxx (`src/gui/gtkmm4`):
- `GuiInitializer`, with the Gimp signal handlers as `SignalConnection` members;
- `GuiVtable`, a C++ object behind 27 static thunks that `gimp->gui` points to;
- `Session`, `Splash`, `Themes`, `IconThemes`, `UniqueService` and `GuiMessage`.

`GimpApp`, `GimpCoreApp` and `GimpConsoleApp` are reimplemented in `.cc` with the same GTypes. `gimp-version.c` and `gimp-update.c` move into the adaptor, keeping their C API for the widgets and dialogs.

**Decision point, gtkmm-3.0:**
- Recommendation: not here, because wrapping GIMP's own GObject widgets for gtkmm needs `Glib::wrap` registrations for each type.
- Plain GTK C calls from C++ with RAII; gtkmm only for wholly new widgets, if any.

**Done when:** the GUI smoke runs and the parity matrix pass; the session, menus and shortcuts files are byte-identical after a start-quit cycle.

**Effort:** 2-4 weeks.

### R2. Actions, menus and dialog registries

**Scope:**
- `app/actions`: 40K lines; 46 action groups, 48 `*-actions.c`, 44 `*-commands.c`.
- `app/menus`: 2.6K lines; 38 menu managers. The GtkBuilder `.ui` files in `menus/` stay.
- `app/dialogs`: 40K lines; about 100 dialog factory entries, the 3,719-line preferences dialog.
- There are almost no GTypes here.

**Approach:**
- C as C++ first.
- Then the action entry tables become `constexpr` tables with typed callbacks, `actions_init()` becomes an `ActionGroupsRegistry`, and `dialogs.c` becomes a `DialogsRegistry`.
- The command callbacks stay free functions in a namespace per group.

**Done when:** every menu path and action name is unchanged (a dump of `gimp->app` actions and accelerators compared before and after), and the GUI smoke runs pass.

**Effort:** 4-6 weeks.

### R3. Widgets, display, tools, propgui

**Scope:**
- `app/widgets`: 158K lines, 210 types.
- `app/display`: 69K lines, 55 types.
- `app/tools`: 80K lines, 98 types.
- `app/propgui`: 7K lines.

This is the largest GObject/GTK layer, and its directories include each other in cycles (widgets ↔ display ↔ tools).

**Approach:**
- T1 for all four directories first, because of the cycles.
- Then C as C++ in an order that starts at the leaves:
  1. `propgui`;
  2. the `display` canvas items and tool widgets;
  3. `widgets` view renderers and container views;
  4. editors and docks;
  5. `display` shell;
  6. `tools`.
- The GObject classes stay GObject (principle 3).
- Redesign candidates: `GimpToolControl` (38 behavior fields) and the `gimpdisplayshell-*.c` split of one 2,469-line class into 30 files.

**Done when:** the GUI smoke runs and a scripted tool session (through the PDB and actions) give the same images.

**Effort:** 4-8 months.

### R4. Core services

**Scope:**
- `app/core`: 163K lines, 110 types, 8 interfaces;
- `app/config` (11K) and `app/pdb` (62K, of which 63 files are generated);
- `app/plug-in` (15K), `app/xcf` (11K), `app/file` and `app/file-data` (5K);
- `app/text` (9K) and `app/path` (12K).

`core` and its neighbours form one strongly connected component (doc 01, section 6).

**Approach:**
- T1 for the whole component first.
- Then C as C++ directory by directory: `config`, `text`, `path`, `xcf`, `file`, `file-data`, `plug-in`, then `core`. The `app/pdb` generated part waits for R6.
- **The `Gimp` root object** keeps its GType. Its creation and life cycle are already owned by `gimp_adaptor::GimpController`. Its managers (plug-in, extension, data factories, PDB) can become C++ members behind its private pointer.
- **Process-wide state** (`the_unit_gimp`, the `GimpData` ID table, the "standard" data singletons) moves into owned objects.
- Undo, the image and item model, and the projection stay GObject (their signals feed R3).

**Done when:**
- the XCF round-trip corpus is byte-identical;
- the PDB tests pass;
- `pluginrc`, `gimprc` and `sessionrc` are byte-identical after a start-quit cycle.

**Effort:** 6-12 months.

### R5. The engine

**Scope:**
- `app/operations`: 31K lines, 65 types (56 registered `gimp:*` operations);
- `app/paint`: 20K lines, 29 types;
- `app/gegl`: 8.6K lines.

Nine `.cc` files already exist, and they are the most C++-idiomatic code in the tree (`paint/gimppaintcore-loops.cc` alone has 115 templates).

**Approach:**
- C as C++.
- Template the per-format pixel loops where the C version switches on formats at run time.
- The GEGL registration contract (operation names, properties, `GeglOperationClass` vfuncs) stays.

**Done when:** pixel-exact output hashes per operation and paint core, and no slowdown in the performance logs.

**Effort:** 2-4 months.

### R6. The PDB generator

**Scope:** `pdb/` (Perl: `pdbgen.pl`, `app.pl`, `lib.pl`), 61 groups, 803 procedures. It generates `app/pdb/*-cmds.c` and `internal-procs.[ch]`, and `libgimp/*_pdb.[ch]`, on every build, into the source tree.

**Approach:**
- `app.pl` emits C++ (`*-cmds.cc`) for the core side, with the same registration behavior.
- `lib.pl` keeps emitting C for `libgimp` (C2).
- The Perl generator stays; a Python rewrite is a separate, optional project.

**Done when:** `internal_procs_init()` registers the same 803 procedures with the same parameter specs, and the output of the `gimp-pdb-dump` procedure is identical.

**Effort:** 2-4 weeks.

### R7. The public libraries (optional)

**Scope:** `libgimpbase` ... `libgimpui`, 196K lines.

The C ABI and the GIR are frozen, so C++ may only be the implementation:
- every exported function keeps C linkage (the headers have `G_BEGIN_DECLS` after T1);
- no C++ type appears in an installed header, and no exception escapes;
- the `.def` files stay unchanged, as `tools/defcheck.py` checks;
- `G_GNUC_INTERNAL` keeps hiding the internals.

Linking `libstdc++` into `libgimp-3.0.so` adds a run-time dependency for every plug-in, which is acceptable but visible. The value is low.

**Recommendation:** last, or never, for `libgimp`/`libgimpui`.

**The better C++ deliverable** is a *separate* C++ plug-in SDK on top of `libgimp`: an installable library, `libgimpxx` (a gimpmm), with pkg-config and CMake package files. This is exactly the template's `applibMeson`/`libMeson` flavor: `src/lib/facade`, the public headers with an `..._LIB_API` export macro, and hidden visibility.

**Effort:** 3-6 months for the libraries, if done at all; 1-2 months for an SDK skeleton.

### R8. Modules

**Scope:** 11 modules, 8K lines: color selectors, display filters and controllers. Each one is a `GTypeModule` with `gimp_module_query`/`register` (module ABI `0x0005`).

**Approach:** independent of everything else. Convert them as examples of C++ dynamic types, at any time.

**Effort:** 1-2 weeks.

### R9. Plug-ins

**Scope:** 307K lines of C (86 plug-ins in `common/`, about 40 larger ones), the Script-Fu interpreter (TinyScheme) and the Python plug-ins. Every one is its own process that talks to the core through `libgimp` only.

**Approach:**
- Opportunistic: any plug-in can be ported alone, as a small template-shaped application.
- Best done on top of the R7 SDK.
- The file-format plug-ins keep their libraries (libpng, libjpeg, libtiff, ...).

**Effort:** per plug-in, from hours (the `common/` filters) to weeks (file-psd, imagemap, gfig, script-fu).

### R10. Data, translations and packaging

Nothing to convert. The packaging under `build/` follows the relocation (doc 02, Phase 4).

## 4. Milestones

| Milestone | Content | Gate |
|---|---|---|
| M0 | The top-level layer (docs 02 and 03) | parity, Meson tests, unit tests |
| M1 | T1 over all `app/` headers | `gimpxx-headers` passes; the 6 public headers fixed |
| M2 | R1 | GUI smoke; byte-identical rc files after a start-quit cycle |
| M3 | R2 | action and accelerator dump identical |
| M4 | R3 | GUI smoke plus scripted tool sessions |
| M5 | R4 (without the generated PDB part) | XCF corpus, PDB tests, rc files |
| M6 | R5 | pixel-exact operation and paint hashes, performance logs |
| M7 | R6 | identical PDB dump |
| M8 | R7, R8, R9 as decided | per item |

## 5. Measuring progress

`scripts/metrics/cxx-share.py` reports:
- C and C++ lines per layer of doc 01, section 3;
- the GObject types defined in `.cc` files;
- the `app/` headers that pass the T1 gate;
- the directories built with `-Wc++-compat`.

The numbers go into `CHANGELOG.md` with every gimpxx release.

## 6. Risks of the lower layers

| Risk | Mitigation |
|---|---|
| Upstream merges stop working once files are renamed. | T6 cut-off per layer; port fixes by hand; rename late within each layer. |
| C++ members inside GObject instance structs are never constructed or destroyed (GType zero-fills the memory). | Principle 4, enforced in review and with a `static_assert(std::is_standard_layout_v<...>)` per converted type. |
| Exceptions escaping into GTK/GLib/GEGL callbacks. | `noexcept` thunks; `-fno-exceptions` is an option for the converted C code. |
| Pixel or performance regressions in the engine. | T4 hashes; the performance logs; the templates already used by GIMP's own `.cc` files. |
| Compile time and binary size grow with templates and header-heavy code. | Tracked by T5; precompiled headers per static library if needed. |
| Plug-in ABI breaks. | `check-def-files` in CI; no C++ in installed headers. |

## 7. Rough size

| Layer | Lines | Effort |
|---|---|---|
| T1 | 1,144 headers; about 50 keyword identifiers in 28 headers plus about 600 uses of the renamed fields (`->private` 549, `->template` 52) | 1-2 weeks |
| R1 | 8K plus the remaining glue | 2-4 weeks |
| R2 | 83K | 4-6 weeks |
| R3 | 314K | 4-8 months |
| R4 | 289K | 6-12 months |
| R5 | 59K | 2-4 months |
| R6 | the generator | 2-4 weeks |
| R7 | 196K (optional), or an SDK skeleton | 3-6 months, or 1-2 months |
| R8 | 8K | 1-2 weeks |
| R9 | 308K, opportunistic | per plug-in |
