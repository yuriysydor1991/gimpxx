if [[ -z ${PROJECT_ROOT} ]] ; then
    echo "#### No PROJECT_ROOT var given (scripts common.sh)"
    exit 1
fi

. "${PROJECT_ROOT}/scripts/build/common.sh"

# The Docker targets get a build directory of their own, so their options never
# reach the build the scripts/build ones reconfigure.
BUILD_DIR="${PROJECT_ROOT}/build/docker"

# Configures the release build with the ENABLE_DOCKER option enabled and builds
# the target the first argument names. The rest of the arguments go to the
# meson setup step after that option, so an own -D<option>=<value> wins.
#
# A reconfigure keeps the options of the previous one, so the
# JENKINS_PIPELINE_FORCE_REBUILD option, which erases the Jenkins pipeline
# container and image at every configure step, is turned off unless given.
build_docker_target()
{
    TARGET=$1
    shift

    filter_script_args "$@"

    SETUP_ARGS=(--buildtype=release "-DENABLE_DOCKER=true"
      "-DJENKINS_PIPELINE_FORCE_REBUILD=false")

    if [[ -d "${BUILD_DIR}" ]] ; then
        SETUP_ARGS+=(--reconfigure)
    fi

    meson setup "${BUILD_DIR}" "${PROJECT_ROOT}" "${SETUP_ARGS[@]}" "${BUILD_ARGS[@]}"

    meson compile -C "${BUILD_DIR}" "${TARGET}"
}

# Builds the Jenkins pipeline target the first argument names as the
# build_docker_target does, with the ENABLE_JENKINS_DOCKER_PIPELINE option
# enabled and the --rebuild flag turned into the JENKINS_PIPELINE_FORCE_REBUILD
# one.
build_jenkins_target()
{
    TARGET=$1
    shift

    FORCE_REBUILD=false

    if [[ $* =~ --rebuild ]] ; then
        FORCE_REBUILD=true
    fi

    build_docker_target "${TARGET}" -DENABLE_JENKINS_DOCKER_PIPELINE=true \
      "-DJENKINS_PIPELINE_FORCE_REBUILD=${FORCE_REBUILD}" "$@"
}
