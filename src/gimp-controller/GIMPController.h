#ifndef GIMPXX_PROJECT_GIMPCONTROLLER_CLASS_H
#define GIMPXX_PROJECT_GIMPCONTROLLER_CLASS_H

#include <glib.h>

#include <string>

/// @brief The early configuration GIMP reads from the gimprc files.
typedef struct _GimpEarlyRc GimpEarlyRc;

namespace gimp_controller
{

/**
 * @brief The controller of GIMP, the former main.c of GIMP: it owns the GIMP
 * command line options, starts GIMP up step by step the way the main() of
 * GIMP did and runs GIMP at last.
 *
 * The platform specifics are the pure virtual methods, which the
 * LinuxGIMPController, MacOSGIMPController and WindowsGIMPController
 * descendants implement, the empty way where their platform needs nothing.
 * The console binary gets the class compiled with the
 * GIMP_CONSOLE_COMPILATION macro, as it got main.c.
 */
class GIMPController
{
 public:
  /// @brief Frees the option values the GIMP option parsing allocated.
  virtual ~GIMPController();
  GIMPController();

  GIMPController(const GIMPController&) = delete;
  GIMPController& operator=(const GIMPController&) = delete;

  /**
   * @brief Prepares the platform before GLib starts: fixes the command line
   * the OS gave, sets the environment GIMP needs there and attaches the
   * console window of the development versions.
   *
   * @param argc The command line arguments count.
   * @param argv The command line arguments.
   */
  virtual void init_platform(int& argc, char**& argv);

  /**
   * @brief Initializes the GIMP backtraces and installs the GIMP signal
   * handlers.
   *
   * @return Returns the file the backtrace of a crash goes into, an empty one
   * if there is none.
   */
  virtual std::string init_signals();

  /**
   * @brief Prepares the process: the platform specifics, the memory
   * allocation, the GIMP environment, the GIMP logging, the translations and
   * the application and the program names.
   *
   * @param argv The command line arguments.
   *
   * @return Returns the GLib allocated copy of the command line arguments,
   * which the GIMP option parsing modifies.
   */
  virtual char** init_process(char** argv);

  /// @brief Sets the verbose output option ahead of the option parsing, since
  /// the version output needs it.
  virtual void set_be_verbose(const bool newValue);

  /// @brief Sets the no interface option ahead of the option parsing, since
  /// GTK needs it before it starts.
  virtual void set_no_interface(const bool newValue);

  /// @brief Attaches the console window of the parent process, where the
  /// platform needs it for the output.
  virtual void attach_console() = 0;

  /// @brief Shows the GIMP version, the verbose one with the verbose output.
  virtual void show_version();

  /**
   * @brief Creates the option context of the GIMP options and of the options
   * of the libraries GIMP uses. Its parsing fills the options of the current
   * instance, which must outlive it, and reads the early configuration.
   *
   * @return Returns the created option context.
   */
  virtual GOptionContext* create_option_context();

  /**
   * @brief Prints the reason of the failed option parsing.
   *
   * @param error The parsing error, which the method frees, or nullptr if the
   * graphical user interface failed to start.
   */
  virtual void report_parse_error(GError* error);

  /**
   * @brief Runs GIMP the way the end of the main() of GIMP did: names the
   * process after its flatpak, attaches the console when the output needs it,
   * passes the command line to the already running GIMP instance if there is
   * one, checks the sanity of the libraries and runs GIMP at last.
   *
   * @param fullProgName The program path, the first command line argument.
   * @param backtraceFile The file of the crash backtrace, empty for none.
   *
   * @return Returns the GIMP exit status.
   */
  virtual int run(const gchar* fullProgName, const std::string& backtraceFile);

 protected:
  /// @brief Fixes the command line arguments the OS gave to the process.
  virtual void fix_arguments(int& argc, char**& argv) = 0;

  /// @brief Sets the environment GIMP needs on the platform before GLib
  /// starts.
  virtual void init_environment(const gchar* progName) = 0;

  /// @brief Prepares the process the platform way, ahead of the GIMP
  /// environment.
  virtual void prepare_process() = 0;

  /// @brief Adjusts the locale the environment selected.
  virtual void init_locale() = 0;

  /// @brief Applies the platform settings of the early configuration.
  virtual void apply_early_configuration(GimpEarlyRc* earlyrc) = 0;

  /// @brief Copies the command line arguments for the GIMP option parsing,
  /// the GLib copy of the given ones by default.
  virtual char** copy_arguments(char** argv);

  /// @brief Passes the files to open to the already running GIMP instance,
  /// through the GIMP unique instance service by default.
  ///
  /// @return Returns TRUE if a running GIMP instance took them over.
  virtual gboolean open_in_running_instance(const gchar** filenames,
                                            const gboolean asNew);

  /// @brief Tells if GIMP and its plug-ins may share memory, which the SysV
  /// or the POSIX shared memory support decides by default.
  virtual gboolean has_shared_memory();

 private:
  /// @brief Hooks the GLib memory profiler, the GIMP builds with it only.
  void init_malloc();

  /// @brief Initializes the locale and binds the message catalogs.
  void init_i18n();

  /// @brief Reads the gimprc files for the settings the early start-up
  /// needs, the language first, before any library initializes.
  void early_configuration();

  /// @brief Names the process after its flatpak, as the desktop knows it.
  ///
  /// @return Returns false if no display is open for the user interface.
  bool set_flatpak_prgname();

  /// @brief Passes the command line to the already running GIMP instance.
  ///
  /// @return Returns true with the exit status in the given status if a
  /// running GIMP instance took the command line over.
  bool run_remote(int& status);

  /// @brief Aborts GIMP once the libraries it uses are not sane.
  void check_sanity();

  /// @brief Runs GIMP and cleans the GIMP environment up afterwards.
  int run_app(const gchar* fullProgName, const std::string& backtraceFile);

  // The callbacks of the GIMP options, which get the instance as their data.
  static gboolean show_version_and_exit(const gchar* optionName,
                                        const gchar* value, gpointer data,
                                        GError** error) noexcept;
  static gboolean show_license_and_exit(const gchar* optionName,
                                        const gchar* value, gpointer data,
                                        GError** error) noexcept;
  static gboolean option_fatal_warnings(const gchar* optionName,
                                        const gchar* value, gpointer data,
                                        GError** error) noexcept;
  static gboolean option_stack_trace_mode(const gchar* optionName,
                                          const gchar* value, gpointer data,
                                          GError** error) noexcept;
  static gboolean option_pdb_compat_mode(const gchar* optionName,
                                         const gchar* value, gpointer data,
                                         GError** error) noexcept;
  static gboolean option_dump_gimprc(const gchar* optionName,
                                     const gchar* value, gpointer data,
                                     GError** error) noexcept;
  static gboolean option_dump_pdb_procedures_deprecated(
      const gchar* optionName, const gchar* value, gpointer data,
      GError** error) noexcept;
  static gboolean options_group_parse_hook(GOptionContext* context,
                                           GOptionGroup* group, gpointer data,
                                           GError** error) noexcept;

  // The GIMP options, which the GIMP option parsing fills. The constructor
  // gives the ones without a default here the defaults of the build kind.
  gboolean mbe_verbose{FALSE};
  gboolean mno_interface;
  gboolean mnew_instance{FALSE};
  gboolean mconsole_messages{FALSE};
  const gchar** mbatch_commands{nullptr};
  const gchar* msystem_gimprc{nullptr};
  const gchar* muser_gimprc{nullptr};
  const gchar* msession_name{nullptr};
  const gchar* mbatch_interpreter{nullptr};
  const gchar** mfilenames{nullptr};
  gboolean mquit{FALSE};
  gboolean mas_new{FALSE};
  gboolean mno_data{FALSE};
  gboolean mno_fonts{FALSE};
  gboolean mno_splash{FALSE};
  /// @brief Lowered by the --no-shm option only, the shared memory is used
  /// where GIMPController::has_shared_memory allows it.
  gboolean muse_shm{TRUE};
  gboolean muse_cpu_accel{TRUE};
  gboolean muse_debug_handler{FALSE};
  gboolean mshow_playground;
  gboolean mshow_debug_menu;
  /// @brief The GimpStackTraceMode value.
  int mstack_trace_mode;
  /// @brief The GimpPDBCompatMode value.
  int mpdb_compat_mode;
};

}  // namespace gimp_controller

#endif  // GIMPXX_PROJECT_GIMPCONTROLLER_CLASS_H
