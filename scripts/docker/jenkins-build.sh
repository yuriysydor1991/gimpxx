#!/bin/bash -e
#
# Builds the Jenkins pipeline Docker image: the `jenkins-pipeline-docker-build`
# target of a release configure with the ENABLE_DOCKER and the
# ENABLE_JENKINS_DOCKER_PIPELINE options enabled.
#
# The --rebuild flag erases the image and the container and builds the image
# anew, with the base image of the Dockerfile pulled and no cache reused.
#
# Every other extra parameter is forwarded to the meson setup step.

PROJECT_ROOT=$(realpath "$(dirname "$0")/../..")

. "${PROJECT_ROOT}/scripts/docker/common.sh"

build_jenkins_target jenkins-pipeline-docker-build "$@"
