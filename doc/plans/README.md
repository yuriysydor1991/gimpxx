# gimpxx plans

The planning documents of the GIMP C → C++ transition. They follow the same approach as [darknetxx](https://github.com/yuriysydor1991/darknetxx) (its `doc/plans`), and use the layout of [cpp-app-template](https://github.com/yuriysydor1991/cpp-app-template).

**Status:** plans only. Nothing in them is implemented yet.

**Analyzed on 2026-10-09**, against gimpxx `master` `b7f8cac066` (GIMP 3.3.1, `gimp-data` `fe4ecc0`), cpp-app-template `main` `1d445dbd` / `app` `804f3ff9` / `appMeson` `913961ad` / `applibMeson` `5e35f9e3` / `appGtkmm3` `2c6cffcb`, and darknetxx `f37cf317`.

## Reading order

| # | Document | Read it for |
|---|---|---|
| 1 | [01 - Architecture analysis](01-gimpxx-architecture-analysis.md) | What GIMP is made of, from `main()` down: the build, the start-up and shutdown sequences, the layers, the dependency structure, C++ readiness, platform specifics |
| 2 | [02 - Transition plan](02-gimpxx-cpp-transition-plan.md) | The goal, the hard constraints, the decisions (D1-D12), the target layout, the phases and their commits, tests, risks, open decisions, size |
| 3 | [03 - Top-level layer design](03-gimpxx-top-level-layer-design.md) | Every C++ class of the top-level organization layer, the mapping of each C function onto it, the command line, the start-up order, signals, errors, logging, build, parity matrix, tests |
| 4 | [04 - Lower layers roadmap](04-gimpxx-lower-layers-roadmap.md) | What follows: the header hygiene, C as C++, then the GUI, core, engine, PDB generator, libraries, modules and plug-ins |

## The plan in one paragraph

1. **Keep Meson,** and graft the template's Meson flavor onto GIMP's build: `meson/`, `src/`, `doc/`, `misc/`, `scripts/`.
2. **Build a CLI parity harness against the upstream binaries.**
3. **Replace the C `main()` with the template's `src/main.cpp` and `ApplicationFactory`.** At first they only call the renamed C `main()`, then they take its responsibilities over one per commit:
   - the command line (still `GOptionContext` underneath);
   - the platform and process bootstrap;
   - the OS signals;
   - the help, version, license and dump applications;
   - the remote instance;
   - sanity, language, `app_run()` (`GimpController` with console and GUI runners), errors.
4. **Behavior stays identical.** Only `gimp-version.c`, `gimp-update.c`, `gimp-log.c`, `gimp-debug.c` and the `GApplication` GObjects stay C, because lower layers call them.
5. **Afterwards, move the upstream tree under `src/gimp/`.** That needs a small `gimp-data` patch.
6. **Then continue layer by layer** (doc 04).

## Diagrams

The PlantUML sources live in [`doc/diagrams`](../diagrams):

| File | Shows |
|---|---|
| [`gimpxx-current-layers.puml`](../diagrams/gimpxx-current-layers.puml) | Today's layers L0-L7 and their coupling |
| [`gimpxx-target-components.puml`](../diagrams/gimpxx-target-components.puml) | The components after the top-level transition |
| [`gimpxx-top-level-classes.puml`](../diagrams/gimpxx-top-level-classes.puml) | The classes of `app`, `converters` and `gimp_adaptor` |
| [`gimpxx-startup-sequence.puml`](../diagrams/gimpxx-startup-sequence.puml) | The start-up, annotated with the `app/main.c` and `app/app.c` lines each step replaces |

Render them with `plantuml -tsvg doc/diagrams/*.puml`. Once the template's Doxygen documentation is set up (plan step 5.1), they are rendered with it.

## Keeping the plans current

The line numbers refer to the revisions above. When the code moves on, search by the function names, and update the "Analyzed on" lines of the documents you revise.
