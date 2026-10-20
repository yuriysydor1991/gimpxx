## Enabling Jenkins pipeline inside Docker container

In order to enable the [Jenkins](https://www.jenkins.io/) pipeline execution inside the Docker container build and start the appropriate container with image by building the `jenkins-pipeline-docker-run` target:

```
# Inside the source root directory

# Configuring the project to support Docker with Jenkins targets
meson setup build -DENABLE_DOCKER=true -DENABLE_JENKINS_DOCKER_PIPELINE=true

# Building and starting Jenkins' pipeline Docker container
# or restarting it
meson compile -C build jenkins-pipeline-docker-run
```

Locate and save the new generated password for the [Jenkins](https://www.jenkins.io/) Web GUI in the log output that may look like next:

```
# ...

*************************************************************
*************************************************************
*************************************************************

Jenkins initial setup is required. An admin user has been created and a password generated.
Please use the following password to proceed to installation:

aefdc6a4a2214c298c821236c739165c

This may also be found at: /var/jenkins_home/secrets/initialAdminPassword

*************************************************************
*************************************************************
*************************************************************

# ...
```

Go to the `http://127.0.0.1:8080` address (the port may change) with a Web browser and finish the installation of the [Jenkins](https://www.jenkins.io/) system in its GUI. Then create a *Pipeline* job there with the *Pipeline script from SCM* definition, point it at the repository and the branch of the project and set its *Script Path* to `misc/Jenkinsfile`.

All the [Jenkins](https://www.jenkins.io/) pipeline Meson build scripts are located at the [meson/dockerers/gimpxx-project-docker-Jenkins-pipeline-runner-target/meson.build](/meson/dockerers/gimpxx-project-docker-Jenkins-pipeline-runner-target/meson.build) file. The [Jenkins](https://www.jenkins.io/) Web GUI port of the container may be changed with the `JENKINS_PIPELINES_PANEL_HTTP_PORT` Meson option.

Whether the image is built and whether the container is created anew or the already created one restarted is decided at the configure step, so configure the project before every build of the target once the container is there; the starter scripts below perform both steps.

### Starter scripts

The [scripts/docker](/scripts/docker) directory holds the shortcuts which perform both commands above, so the whole run takes a single command:

| Script | Starts the Meson target |
| --- | --- |
| [jenkins-run.sh](/scripts/docker/jenkins-run.sh) | `jenkins-pipeline-docker-run` |
| [jenkins-build.sh](/scripts/docker/jenkins-build.sh) | `jenkins-pipeline-docker-build` |

```
# inside the project root directory

scripts/docker/jenkins-run.sh
```

Each of them configures the `build/docker` directory with both the `ENABLE_DOCKER` and the `ENABLE_JENKINS_DOCKER_PIPELINE` options set to `true` and then builds the target of interest, as the [Docker container starter scripts](/doc/sections/en_US/5-12-enabling-the-docker-container-build-and-run.md) do. The `jenkins-pipeline-docker-run` Meson target depends on the `jenkins-pipeline-docker-build` one, which means the [jenkins-run.sh](/scripts/docker/jenkins-run.sh) script builds the image as well and the [jenkins-build.sh](/scripts/docker/jenkins-build.sh) one is only needed to build the image without starting the interactive container.

Every extra parameter of the scripts is forwarded to the `meson setup` step:

```
# inside the project root directory

# changes the Jenkins Web GUI port of the container
scripts/docker/jenkins-run.sh -DJENKINS_PIPELINES_PANEL_HTTP_PORT=9090
```

The `--rebuild` flag of both scripts sets the `JENKINS_PIPELINE_FORCE_REBUILD` Meson option, which erases the container and the image at the configure step and builds the image anew with the base image of the Dockerfile pulled again and no builder cache reused, so nothing of the previous image survives. A reconfigure keeps the options of the previous one, so every script run without the flag turns the option off again:

```
# inside the project root directory

scripts/docker/jenkins-run.sh --rebuild
```

### Tools of the container

The [misc/Dockerfile.Jenkins.pipeline.meson.in](/misc/Dockerfile.Jenkins.pipeline.meson.in) image carries the tools the [misc/Jenkinsfile](/misc/Jenkinsfile) stages call which its Debian base distributes: `git`, `meson` with `ninja-build`, `g++`, `pkg-config`, the GoogleTest / GoogleMock development packages, `clang-format`, `clang-tidy` and `cppcheck`, plus `flatpak` with `flatpak-builder` for the `RUN_FLATPAK_PACKAGER` stage, `rpm` for the `RUN_RPM_PACKAGER` one (the `RUN_DEB_PACKAGER` one needs the `dpkg-deb` every Debian based system has) and `doxygen` with `graphviz` for the documentation build. Two packager stages can't pass inside this container, so keep their parameters off there and run them on a host instead: `RUN_SNAP_PACKAGER` needs `snapcraft`, which comes as a snap only and therefore asks for a running `snapd`, and `RUN_APPIMAGE_PACKAGER` needs the `appimagetool`, which no Debian package provides and which runs through FUSE. The `RUN_FREEBSD_PACKAGER` and the `RUN_WIX_PACKAGER` stages are FreeBSD and MS Windows only.

The pipeline runs on a [Jenkins](https://www.jenkins.io/) instance of your own as well, with the job defined the same way: its agent needs the same tools, and the [scripts/packages](/scripts/packages) install scripts install all of them but the packager ones.
