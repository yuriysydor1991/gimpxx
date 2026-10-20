#include "src/gimp-controller/linux/LinuxGIMPController.h"

namespace gimp_controller
{

void LinuxGIMPController::attach_console() {}

void LinuxGIMPController::fix_arguments(int&, char**&) {}

void LinuxGIMPController::init_environment(const gchar*) {}

void LinuxGIMPController::prepare_process() {}

void LinuxGIMPController::init_locale() {}

void LinuxGIMPController::apply_early_configuration(GimpEarlyRc*) {}

}  // namespace gimp_controller
