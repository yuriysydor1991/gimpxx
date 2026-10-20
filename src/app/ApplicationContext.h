#ifndef GIMPXX_PROJECT_APPLICATIONCONTEXT_CLASS_H
#define GIMPXX_PROJECT_APPLICATIONCONTEXT_CLASS_H

#include <glib.h>

#include <atomic>
#include <memory>
#include <string>
#include <vector>

#include "src/gimp-controller/GIMPController.h"

namespace app
{

/**
 * @brief The class that holds necessary information for the application
 * to run properly.
 *
 * May hold parsed parameters, configuration values and error messages
 * with application status code. The command line parameters are parsed by
 * the CommandLineParser class instance inside the ApplicationFactory class,
 * into the GIMP options of the GIMPController the context holds. The context
 * owns the GLib resources the start-up of GIMP allocates for the whole run.
 *
 * All the data members are private ones, so the held data is reachable through
 * the appropriate access methods only.
 */
class ApplicationContext
{
 public:
  /**
   * @brief An ApplicationContext constructor.
   *
   * @param gargc Command line arguments count passed by the OS through the main
   * executable function that will be referenced in the margc member field.
   * @param gargv  The command line arguments array passed by the OS through
   * the main executable function that will be referenced in the margv member
   * field.
   * @param gimpController The controller which starts and runs GIMP, must not
   * be nullptr.
   */
  ApplicationContext(
      int& gargc, char**& gargv,
      std::shared_ptr<gimp_controller::GIMPController> gimpController);

  /// @brief Frees the adopted command line arguments copy and option context.
  virtual ~ApplicationContext();

  ApplicationContext(const ApplicationContext&) = delete;
  ApplicationContext& operator=(const ApplicationContext&) = delete;

  /// @brief Provides the referenced command line arguments count. The
  /// reference is modifiable, since toolkits like Gtk::Application::create and
  /// glutInit consume argc/argv through a non constant one.
  int& get_argc() const;

  /// @brief Provides the referenced command line arguments array. Modifiable
  /// for the same reason as ApplicationContext::get_argc.
  char**& get_argv() const;

  /// @brief Replaces the command line arguments with their GLib allocated
  /// copy, which the GIMP option parsing modifies, and frees it at the end.
  void adopt_argv(char** argvCopy);

  /// @brief Provides the controller which starts and runs GIMP.
  gimp_controller::GIMPController& get_gimp_controller() const;

  /// @brief Keeps the parsed GIMP option context until the end of the run.
  void adopt_option_context(GOptionContext* optionContext);

  /// @brief Provides the file the GIMP signal handlers write the backtraces
  /// into, empty if there is none.
  const std::string& get_backtrace_file() const;

  /// @brief Sets the backtrace file the GIMP signal handlers chose.
  void set_backtrace_file(const std::string& backtraceFile);

  /// @brief Tells if the ApplicationFactory should create and run an
  /// ApplicationVersionPrinter instance.
  bool get_print_version_and_exit() const;

  /// @brief Sets the version printing flag. Only the ApplicationFactory reads
  /// it.
  void set_print_version_and_exit(const bool newValue);

  /// @brief Provides the accumulated errors descriptions.
  const std::vector<std::string>& get_errors() const;

  /// @brief Pushes a new error description into the merrors field.
  void push_error(const std::string& errorDescription);

  /// @brief Tells if the application was asked to stop. Atomic, so it is safe
  /// to poll it from any thread.
  bool get_stop() const;

  /// @brief Sets the application stop flag. Raise it to ask the running
  /// routines to return from IApplication::run.
  void set_stop(const bool newValue);

  /// @brief Tells if the application was asked to pause. Atomic, so it is safe
  /// to poll it from any thread.
  bool get_pause() const;

  /// @brief Sets the application pause flag. Raise it to ask the running
  /// routines to suspend their work without leaving IApplication::run and
  /// lower it to let them continue.
  void set_pause(const bool newValue);

  /// @brief Tells if the application was asked to reload its configuration.
  /// Atomic, so it is safe to poll it from any thread.
  bool get_reload() const;

  /// @brief Sets the application reload flag. The application is expected to
  /// reread its configuration once the flag is raised and to lower it back on
  /// its own afterwards.
  void set_reload(const bool newValue);

  /// @brief Tells if the first user defined request is pending. Atomic, so it
  /// is safe to poll it from any thread.
  bool get_first_user_request() const;

  /// @brief Sets the first user defined request flag. The meaning of the
  /// request is the application own one - reopening the log files is the
  /// classic one - and the application is expected to lower the flag back
  /// after serving it.
  void set_first_user_request(const bool newValue);

  /// @brief Tells if the second user defined request is pending. Atomic, so it
  /// is safe to poll it from any thread.
  bool get_second_user_request() const;

  /// @brief Sets the second user defined request flag. See the
  /// ApplicationContext::set_first_user_request setter.
  void set_second_user_request(const bool newValue);

 private:
  /// @brief Command line arguments count passed by the OS through the main
  /// executable function.
  int& margc;

  /// @brief The command line arguments array passed by the OS through
  /// the main executable function.
  char**& margv;

  /// @brief Tells if margv holds the adopted copy of the arguments.
  bool margv_adopted{false};

  /// @brief The controller which starts and runs GIMP.
  std::shared_ptr<gimp_controller::GIMPController> mgimp_controller;

  /// @brief The adopted GIMP option context.
  GOptionContext* moption_context{nullptr};

  /// @brief The backtrace file of the GIMP signal handlers.
  std::string mbacktrace_file;

  /// @brief The version printing flag. See the
  /// ApplicationContext::set_print_version_and_exit setter.
  bool mprint_version_and_exit{false};

  /// @brief Errors description.
  std::vector<std::string> merrors;

  /// @brief The thread safe application stop flag. See the
  /// ApplicationContext::set_stop setter.
  std::atomic_bool mstop{false};

  /// @brief The thread safe application pause flag. See the
  /// ApplicationContext::set_pause setter.
  std::atomic_bool mpause{false};

  /// @brief The thread safe configuration reload flag. See the
  /// ApplicationContext::set_reload setter.
  std::atomic_bool mreload{false};

  /// @brief The thread safe first user defined request flag. See the
  /// ApplicationContext::set_first_user_request setter.
  std::atomic_bool mfirst_user_request{false};

  /// @brief The thread safe second user defined request flag. See the
  /// ApplicationContext::set_second_user_request setter.
  std::atomic_bool msecond_user_request{false};
};

}  // namespace app

#endif  // GIMPXX_PROJECT_APPLICATIONCONTEXT_CLASS_H
