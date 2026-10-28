#include "src/gimp-controller/GIMPControllerFactory.h"

#include <memory>

#include "src/gimp-controller/GIMPController.h"

#if defined(G_OS_WIN32)
#include "src/gimp-controller/windows/WindowsGIMPController.h"
#elif defined(__APPLE__)
#include "src/gimp-controller/macos/MacOSGIMPController.h"
#else
#include "src/gimp-controller/linux/LinuxGIMPController.h"
#endif

namespace gimp_controller
{

std::shared_ptr<GIMPController> GIMPControllerFactory::create_gimp_controller()
{
#if defined(G_OS_WIN32)
  return std::make_shared<WindowsGIMPController>();
#elif defined(__APPLE__)
  return std::make_shared<MacOSGIMPController>();
#else
  return std::make_shared<LinuxGIMPController>();
#endif
}

}  // namespace gimp_controller
