#ifndef GIMPXX_PROJECT_WINDOWSGIMPCONTROLLER_CLASS_H
#define GIMPXX_PROJECT_WINDOWSGIMPCONTROLLER_CLASS_H

#include <glib.h>

#include "src/gimp-controller/GIMPController.h"

namespace gimp_controller
{

/**
 * @brief The GIMPController of MS Windows: it attaches the console window of
 * the GUI binary, takes the command line the mingw-w64 runtime and the Windows
 * give, sets the bundled GLib schemas, the installer mutex, the DLL search
 * path, the DEP and the taskbar grouping up, applies the pen input API of the
 * early configuration and passes the files to the running GIMP instance
 * through the primary instance mutex.
 */
class WindowsGIMPController : public GIMPController
{
 public:
  ~WindowsGIMPController() override = default;
  WindowsGIMPController() = default;

  /// @brief Attaches the GUI binary to the console of the native parent
  /// shell, the console binary has its own one.
  void attach_console() override;

 protected:
  /// @brief Takes the arguments of the mingw-w64 runtime, which passes bogus
  /// ones to the main() of the subsystem:windows programs.
  void fix_arguments(int& argc, char**& argv) override;

  /// @brief Points GLib to the schemas of the relocatable bundle, so the
  /// Windows Registry does not skip them.
  void init_environment(const gchar* progName) override;

  /// @brief Makes the installer aware of the process, sets the text
  /// antialiasing, the DLL search path, the DEP and the taskbar grouping up.
  void prepare_process() override;

  /// @brief Does nothing, the locale of the environment fits here.
  void init_locale() override;

  /// @brief Selects the pen and touch input API of the GUI binary.
  void apply_early_configuration(GimpEarlyRc* earlyrc) override;

  /// @brief Gives the GLib copy of the UTF-8 Windows command line.
  char** copy_arguments(char** argv) override;

  /// @brief Waits for the running GIMP instance to take the files over, once
  /// the primary instance mutex tells there is one, since the Windows
  /// Explorer starts a process for each selected file.
  gboolean open_in_running_instance(const gchar** filenames,
                                    const gboolean asNew) override;

  /// @brief Tells the GIMP and its plug-ins always share memory here.
  gboolean has_shared_memory() override;

 private:
  /// @brief Asks to close the attached console window, at the exit.
  static void wait_console_window();
};

}  // namespace gimp_controller

#endif  // GIMPXX_PROJECT_WINDOWSGIMPCONTROLLER_CLASS_H
