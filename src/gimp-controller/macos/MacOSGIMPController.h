#ifndef GIMPXX_PROJECT_MACOSGIMPCONTROLLER_CLASS_H
#define GIMPXX_PROJECT_MACOSGIMPCONTROLLER_CLASS_H

#include <glib.h>

#include "src/gimp-controller/GIMPController.h"

namespace gimp_controller
{

/**
 * @brief The GIMPController of macOS: it removes the session identifier from
 * the command line of the relocatable bundle, sets the cache, the input method
 * and the bundle environment up and keeps the monetary characters of the
 * locale ASCII ones.
 */
class MacOSGIMPController : public GIMPController
{
 public:
  ~MacOSGIMPController() override = default;
  MacOSGIMPController() = default;

  /// @brief Does nothing, the console output needs no console window here.
  void attach_console() override;

 protected:
  /// @brief Removes the -psn_ session identifier the macOS passes to the
  /// relocatable bundle.
  void fix_arguments(int& argc, char**& argv) override;

  /// @brief Sets the cache directory and the input method up, and the paths
  /// of the relocatable bundle, which a shell wrapper can not set since the
  /// macOS 10.15.
  void init_environment(const gchar* progName) override;

  /// @brief Does nothing, the process needs no preparation here.
  void prepare_process() override;

  /// @brief Keeps the monetary sign characters ASCII, so the GtkSpinButton
  /// accepts the minus character.
  void init_locale() override;

  /// @brief Does nothing, the early configuration has no platform settings
  /// here.
  void apply_early_configuration(GimpEarlyRc* earlyrc) override;

 private:
  /// @brief Gives the canonical path of the given GIMP installation
  /// subdirectory, which the caller frees.
  static gchar* new_installation_path(const gchar* relativePath);

  /// @brief Sets the environment variable to the path of the given directory
  /// relative one.
  static void set_path_variable(const gchar* name, const gchar* dir,
                                const gchar* relativePath);

  /// @brief Prepends the given directory to the path list of the variable.
  static void prepend_path_variable(const gchar* name, const gchar* dir);
};

}  // namespace gimp_controller

#endif  // GIMPXX_PROJECT_MACOSGIMPCONTROLLER_CLASS_H
