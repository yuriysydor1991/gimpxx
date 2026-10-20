#ifndef GIMPXX_PROJECT_LINUXGIMPCONTROLLER_CLASS_H
#define GIMPXX_PROJECT_LINUXGIMPCONTROLLER_CLASS_H

#include <glib.h>

#include "src/gimp-controller/GIMPController.h"

namespace gimp_controller
{

/**
 * @brief The GIMPController of Linux and of the other platforms GIMP has no
 * specifics for: none of the platform methods has anything to do there, the
 * GIMPController defaults copy the arguments and pass the files to the running
 * GIMP instance over D-Bus.
 */
class LinuxGIMPController : public GIMPController
{
 public:
  ~LinuxGIMPController() override = default;
  LinuxGIMPController() = default;

  /// @brief Does nothing, the console output needs no console window here.
  void attach_console() override;

 protected:
  /// @brief Does nothing, the command line needs no fixes here.
  void fix_arguments(int& argc, char**& argv) override;

  /// @brief Does nothing, the environment needs no changes here.
  void init_environment(const gchar* progName) override;

  /// @brief Does nothing, the process needs no preparation here.
  void prepare_process() override;

  /// @brief Does nothing, the locale of the environment fits here.
  void init_locale() override;

  /// @brief Does nothing, the early configuration has no platform settings
  /// here.
  void apply_early_configuration(GimpEarlyRc* earlyrc) override;
};

}  // namespace gimp_controller

#endif  // GIMPXX_PROJECT_LINUXGIMPCONTROLLER_CLASS_H
