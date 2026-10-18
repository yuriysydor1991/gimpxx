#!/bin/bash -e
#
# Installs the packages required to build and develop the project on the
# Ubuntu 26.04. The install-ubuntu.sh script picks it on that release.
#
# Every package of the list exists in the Ubuntu 26.04 archive, the GIMP build
# dependencies among them: the release ships the babl and the GEGL versions
# GIMP needs. The GIMP build also needs a locale other than C or POSIX, such
# as the en_US.UTF-8 one of the locales package. A dependency the
# release packages no usable version of is built out of a pinned release tag
# of its sources with the cmake_install_from_source helper of the common.sh.

PROJECT_ROOT=$(realpath "$(dirname "$0")/../..")

. "${PROJECT_ROOT}/scripts/packages/common.sh"

PACKAGES=(
    appstream
    bison
    desktop-file-utils
    flex
    g++
    gegl
    gettext
    git
    gobject-introspection
    iso-codes
    libappstream-dev
    libarchive-dev
    libasound2-dev
    libbabl-dev
    libbacktrace-dev
    libbz2-dev
    libcfitsio-dev
    libcurl4-openssl-dev
    libgegl-dev
    libgexiv2-dev
    libgirepository1.0-dev
    libgmock-dev
    libgs-dev
    libgtest-dev
    libgtk-3-bin
    libgtk-3-dev
    libgudev-1.0-dev
    libheif-dev
    libjson-glib-dev
    libjxl-dev
    liblcms2-dev
    liblzma-dev
    libmaxflow-dev
    libmng-dev
    libmypaint-dev
    libopencv-dev
    libopenexr-dev
    libopenjp2-7-dev
    libpoppler-glib-dev
    libqoi-dev
    libraw-dev
    librsvg2-dev
    libtiff-dev
    libunwind-dev
    libwebp-dev
    libwmf-dev
    libxml2-dev
    libxmu-dev
    libxpm-dev
    locales
    meson
    mypaint-brushes
    ninja-build
    nlohmann-json3-dev
    opencv-data
    pkg-config
    poppler-data
    python3
    python3-gi
    python3-gi-cairo
    xsltproc
)

apt_install "${PACKAGES[@]}"

log "The required packages are installed"
