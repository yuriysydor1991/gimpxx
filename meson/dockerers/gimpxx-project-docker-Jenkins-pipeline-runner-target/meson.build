JENKINS_PIPELINE_DOCKER_IMAGE_NAME = meson.project_name().to_lower() + '-jenkins-pipeline-image'
JENKINS_PIPELINE_DOCKER_CONTAINER_NAME = meson.project_name().to_lower() + '-jenkins-pipeline-run-container'
JENKINS_PIPELINES_PANEL_HTTP_PORT = get_option('JENKINS_PIPELINES_PANEL_HTTP_PORT').to_string()
JENKINS_PIPELINE_DOCKERFILE_SRC = join_paths(meson.project_source_root(), 'misc', 'Dockerfile.Jenkins.pipeline.meson.in')
JENKINS_PIPELINE_DOCKERFILE_DST = 'Dockerfile.Jenkins.pipeline'
JENKINS_PIPELINE_DOCKERFILE_DST_PATH = join_paths(meson.current_build_dir(), JENKINS_PIPELINE_DOCKERFILE_DST)

configure_file(
  input : JENKINS_PIPELINE_DOCKERFILE_SRC,
  output : JENKINS_PIPELINE_DOCKERFILE_DST,
  copy : true
)

# Whether the image and the container are there already is decided at the
# configure step, so the targets either reuse them or make them anew.
jenkins_image_present = run_command(
  docker_exec, 'images', '-q', JENKINS_PIPELINE_DOCKER_IMAGE_NAME,
  check: false
).stdout().strip() != ''

jenkins_container_present = run_command(
  docker_exec, 'ps', '-aq', '-f', 'name=' + JENKINS_PIPELINE_DOCKER_CONTAINER_NAME,
  check: false
).stdout().strip() != ''

jenkins_build_flags = []

if get_option('JENKINS_PIPELINE_FORCE_REBUILD')
  message('Erasing the ' + JENKINS_PIPELINE_DOCKER_CONTAINER_NAME + ' container and the ' + JENKINS_PIPELINE_DOCKER_IMAGE_NAME + ' image')

  run_command(docker_exec, 'container', 'rm', '-f', JENKINS_PIPELINE_DOCKER_CONTAINER_NAME, check: false)
  run_command(docker_exec, 'image', 'rm', '-f', JENKINS_PIPELINE_DOCKER_IMAGE_NAME, check: false)

  jenkins_image_present = false
  jenkins_container_present = false

  # the erased image leaves both its own layers and the pulled base image of
  # the Dockerfile behind in the builder cache, so the build reproduces the
  # very same image until the cache is refused and the base image pulled anew
  jenkins_build_flags = ['--no-cache', '--pull']
endif

if jenkins_image_present
  jenkins_build_cmd = ['echo', 'Image ' + JENKINS_PIPELINE_DOCKER_IMAGE_NAME + ' already built']
else
  jenkins_build_cmd = [docker_exec, 'build'] + jenkins_build_flags + [
    '-f', JENKINS_PIPELINE_DOCKERFILE_DST_PATH,
    '--build-context', 'project=' + meson.project_source_root(),
    '--build-arg', 'CACHEBUST=' + timestamp,
    '-t', JENKINS_PIPELINE_DOCKER_IMAGE_NAME, '.'
  ]
endif

if jenkins_container_present
  jenkins_run_cmd = [docker_exec, 'container', 'start', '-ia', JENKINS_PIPELINE_DOCKER_CONTAINER_NAME]
else
  jenkins_run_cmd = [
    docker_exec, 'run', '-it',
    '-p', JENKINS_PIPELINES_PANEL_HTTP_PORT + ':8080',
    '-p', '50000:50000',
    '--name', JENKINS_PIPELINE_DOCKER_CONTAINER_NAME,
    JENKINS_PIPELINE_DOCKER_IMAGE_NAME
  ]
endif

jenkins_pipeline_docker_build_target = custom_target(
  'jenkins-pipeline-docker-build',
  command: jenkins_build_cmd,
  env: ['DOCKER_BUILDKIT=1'],
  build_always_stale: false,
  output: 'jenkins-pipeline-docker-build-stub.txt',
  console: true
)

# The shell prints the address of the Jenkins Web GUI, its $0, before the
# container takes the console over.
jenkins_pipeline_docker_run_target = custom_target(
  'jenkins-pipeline-docker-run',
  command: [
    'sh', '-c', 'echo "Connect to the Jenkins GUI of the started container at $0" && exec "$@"',
    'http://127.0.0.1:' + JENKINS_PIPELINES_PANEL_HTTP_PORT
  ] + jenkins_run_cmd,
  depends: jenkins_pipeline_docker_build_target,
  build_always_stale: false,
  output: 'jenkins-pipeline-docker-run-stub.txt',
  console: true
)
