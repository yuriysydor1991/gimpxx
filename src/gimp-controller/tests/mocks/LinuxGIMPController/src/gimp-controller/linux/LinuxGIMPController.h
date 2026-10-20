#ifndef GIMPXX_PROJECT_LINUXGIMPCONTROLLER_CLASS_H
#define GIMPXX_PROJECT_LINUXGIMPCONTROLLER_CLASS_H

#include "src/gimp-controller/GIMPController.h"

namespace gimp_controller
{

/// @brief The GIMPController mock, the GIMPController::onMockCreate hook
/// included, of the platform the ApplicationFactory creates it for.
class LinuxGIMPController : public GIMPController
{
};

}  // namespace gimp_controller

#endif  // GIMPXX_PROJECT_LINUXGIMPCONTROLLER_CLASS_H
