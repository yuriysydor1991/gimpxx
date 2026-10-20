#!/bin/bash -e
#
# Builds the Docker image of the project and runs a container of it: the
# `docker-single-run` target of a release configure with the ENABLE_DOCKER
# option enabled.
#
# Every extra parameter is forwarded to the meson setup step, so an own
# -D<option>=<value> override wins over the ENABLE_DOCKER one.

PROJECT_ROOT=$(realpath "$(dirname "$0")/../..")

. "${PROJECT_ROOT}/scripts/docker/common.sh"

build_docker_target docker-single-run "$@"
