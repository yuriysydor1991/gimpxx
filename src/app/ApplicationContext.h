#ifndef GIMPXX_PROJECT_APPLICATIONCONTEXT_CLASS_H
#define GIMPXX_PROJECT_APPLICATIONCONTEXT_CLASS_H

#include <atomic>
#include <string>
#include <vector>

namespace app
{

/**
 * @brief The class that holds necessary information for the application
 * to run properly.
 *
 * May hold parsed parameters, configuration values and error messages
 * with application status code. The command line parameters are parsed by
 * the CommandLineParser class instance inside the ApplicationFactory class,
 * into the options the Application starts its GIMPController with. The
 * context owns the GLib copy of the arguments the start-up of GIMP allocates
 * for the whole run.
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
   */
  ApplicationContext(int& gargc, char**& gargv);

  /// @brief Frees the adopted command line arguments copy.
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

  /// @brief Tells if the command line asks for the verbose output, which the
  /// version output needs ahead of the GIMP option parsing.
  bool get_be_verbose() const;

  /// @brief Sets the verbose output flag.
  void set_be_verbose(const bool newValue);

  /// @brief Tells if the command line asks to run without the user interface,
  /// which GTK needs to know ahead of the GIMP option parsing.
  bool get_no_interface() const;

  /// @brief Sets the no interface flag.
  void set_no_interface(const bool newValue);

  /// @brief Tells if the command line asks for the help output of the GIMP
  /// option parsing, which needs the console window on Windows.
  bool get_print_help() const;

  /// @brief Sets the help output flag.
  void set_print_help(const bool newValue);

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

  /// @brief The verbose output flag. See the
  /// ApplicationContext::get_be_verbose getter.
  bool mbe_verbose{false};

  /// @brief The no interface flag. See the
  /// ApplicationContext::get_no_interface getter.
  bool mno_interface{false};

  /// @brief The help output flag. See the ApplicationContext::get_print_help
  /// getter.
  bool mprint_help{false};

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
