## Enabling the Docker container build and run

To be able to build and run the Docker container with current template project' target binary file enable it's Meson target with setting the `ENABLE_DOCKER` option to the `true` value by using the next command:

```
# inside the project root directory

meson setup build -DENABLE_DOCKER=true
```

After that there should be available the Docker build and run Meson target `docker-single-run` which may be triggered with the command:

```
# inside the project root directory

meson compile -C build docker-single-run
```

The `docker-single-run` target depends on the `docker-single-build` one, which only builds the image. Examine the `meson/dockerers/gimpxx-project-dockerers/meson.build` file for the details of both targets.

### Starter scripts

The [scripts/docker](/scripts/docker) directory holds the shortcuts which perform both commands above, so the whole run takes a single command:

| Script | Starts the Meson target |
| --- | --- |
| [single-run.sh](/scripts/docker/single-run.sh) | `docker-single-run` |
| [single-build.sh](/scripts/docker/single-build.sh) | `docker-single-build` |

```
# inside the project root directory

scripts/docker/single-run.sh
```

Each of them configures the `build/docker` directory as a release build with the `ENABLE_DOCKER` option set to `true` and then builds the target of interest, so no `docker` command of its own is ever issued. The directory is apart from the `build/release` one of the [scripts/build](/scripts/build) scripts, so the Docker options never reach their builds. The [single-run.sh](/scripts/docker/single-run.sh) script builds the image as well, so the [single-build.sh](/scripts/docker/single-build.sh) one is only needed to build the image without starting a container.

Every extra parameter of the scripts is forwarded to the `meson setup` step, so an own `-D<option>=<value>` one wins over the options the scripts set. The configure step runs on the host, so the host needs the build dependencies of the project, although the image builds the project once more on its own.

Keep in mind that the `ENABLE_DOCKER` option makes the configure step require the `docker` executable to be present in the system, so the scripts fail the configure step on a machine with no Docker installed (see the [Optional for the Docker container runs](/doc/sections/en_US/3-10-optional-for-docker-container-runs.md) requirements section).
