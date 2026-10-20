# Changelog

All notable changes to this project will be documented in this file. The
changes of GIMP itself are in the [`src/gimp/NEWS`](/src/gimp/NEWS) file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

Based on the `GIMP_3_2_6` release of GIMP.

- Addressing the GIMP tree through the gimp_source_root and gimp_build_root
- Hiding the original GIMP tree under the src/gimp
- Grafting the cpp-app-template appMeson infrastructure
- Starting GIMP from the template C++ application classes
- Documenting the gimpxx layout and start-up
- Renaming the project into gimpxx
- Naming GIMPxx, based on GIMP, in the user interface
- Naming the header guards after the gimpxx project
- Porting the main.c of GIMP into the GIMPController platform classes
- Naming the Meson modules after the gimpxx project
- Grouping the Meson modules into categories, as the template CMake ones
- Splitting the hardening options per compiler family, as the template CMake ones
