#ifndef GIMPXX_PROJECT_GIMPCONTROLLERFACTORY_CLASS_H
#define GIMPXX_PROJECT_GIMPCONTROLLERFACTORY_CLASS_H

#include <memory>

#include "src/gimp-controller/GIMPController.h"

namespace gimp_controller
{

/**
 * @brief Creates the GIMPController of the current platform, the only platform
 * choice of the application, as the Meson builds the descendant of the current
 * platform only.
 */
class GIMPControllerFactory
{
 public:
  virtual ~GIMPControllerFactory() = default;
  GIMPControllerFactory() = default;

  /**
   * @brief Creates the GIMPController of the current platform: the
   * WindowsGIMPController, the MacOSGIMPController or the LinuxGIMPController
   * one.
   *
   * @return Returns the created GIMP controller instance.
   */
  virtual std::shared_ptr<GIMPController> create_gimp_controller();
};

}  // namespace gimp_controller

#endif  // GIMPXX_PROJECT_GIMPCONTROLLERFACTORY_CLASS_H
