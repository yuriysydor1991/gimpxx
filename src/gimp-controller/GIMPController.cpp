#include "src/gimp-controller/GIMPController.h"

#include <clocale>
#include <cstdlib>
#include <cstring>
#include <string>

#include "src/gimp-controller/gimp-c-includes.h"

namespace gimp_controller
{

// The console binary never initializes the graphical user interface, the
// development versions show their experimental features and debug actions.
GIMPController::GIMPController()
#ifdef GIMP_CONSOLE_COMPILATION
    : mno_interface{TRUE},
#else
    : mno_interface{FALSE},
#endif
#if defined(GIMP_UNSTABLE) || !defined(GIMP_RELEASE)
      mshow_playground{TRUE},
      mshow_debug_menu{TRUE},
      mstack_trace_mode{GIMP_STACK_TRACE_QUERY},
      mpdb_compat_mode{GIMP_PDB_COMPAT_WARN}
#else
      mshow_playground{FALSE},
      mshow_debug_menu{FALSE},
      mstack_trace_mode{GIMP_STACK_TRACE_NEVER},
      mpdb_compat_mode{GIMP_PDB_COMPAT_ON}
#endif
{
}

GIMPController::~GIMPController()
{
  g_free(const_cast<gchar*>(msystem_gimprc));
  g_free(const_cast<gchar*>(muser_gimprc));
  g_free(const_cast<gchar*>(msession_name));
  g_free(const_cast<gchar*>(mbatch_interpreter));
  g_strfreev(const_cast<gchar**>(mbatch_commands));
  g_strfreev(const_cast<gchar**>(mfilenames));
}

void GIMPController::init_platform(int& argc, char**& argv)
{
  fix_arguments(argc, argv);
  init_environment(argv[0]);

#if defined(GIMP_UNSTABLE) || !defined(GIMP_RELEASE)
  attach_console();
#endif
}

std::string GIMPController::init_signals()
{
  gchar* backtraceFile = nullptr;

  // Initialize GimpBacktrace early on. In particular, we want the Windows
  // backend to catch the SET_THREAD_NAME exceptions of newly created threads.
  gimp_backtrace_init();

  // Start signal handlers early.
  gimp_init_signal_handlers(&backtraceFile);

  const std::string result{backtraceFile != nullptr ? backtraceFile : ""};

  g_free(backtraceFile);

  return result;
}

char** GIMPController::init_process(char** argv)
{
  prepare_process();

  init_malloc();

  gimp_env_init(FALSE);

  gimp_log_init();

  init_i18n();

  g_set_application_name(GIMP_NAME);

  char** const argvCopy = copy_arguments(argv);

  g_set_prgname(GIMP_DESKTOP_NAME);

  return argvCopy;
}

void GIMPController::set_be_verbose(const bool newValue)
{
  mbe_verbose = newValue;
}

void GIMPController::set_no_interface(const bool newValue)
{
  mno_interface = newValue;
}

void GIMPController::show_version() { gimp_version_show(mbe_verbose); }

GOptionContext* GIMPController::create_option_context()
{
  // GLib copies the entries, which refer to the options of the instance.
  const GOptionEntry entries[] = {
      {"version", 'v', G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK,
       reinterpret_cast<gpointer>(&show_version_and_exit),
       N_("Show version information and exit"), nullptr},
      {"license", 0, G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK,
       reinterpret_cast<gpointer>(&show_license_and_exit),
       N_("Show license information and exit"), nullptr},
      {"verbose", 0, 0, G_OPTION_ARG_NONE, &mbe_verbose, N_("Be more verbose"),
       nullptr},
      {"new-instance", 'n', 0, G_OPTION_ARG_NONE, &mnew_instance,
       N_("Start a new GIMP instance"), nullptr},
      {"as-new", 'a', 0, G_OPTION_ARG_NONE, &mas_new, N_("Open images as new"),
       nullptr},
      {"no-interface", 'i',
#ifdef GIMP_CONSOLE_COMPILATION
       // The CLI option is not removed for gimp-console because it allows to
       // set the same options on either the GUI or the console binaries, as a
       // no-brainer. We in fact also use this in build scripts which may make
       // use of either one of the binaries to construct some images. Yet it's
       // unnecessary to show this option in the CLI tool usage as it's
       // basically a no-op.
       G_OPTION_FLAG_HIDDEN,
#else
       0,
#endif
       G_OPTION_ARG_NONE, &mno_interface, N_("Run without a user interface"),
       nullptr},
      {"no-data", 'd', 0, G_OPTION_ARG_NONE, &mno_data,
       N_("Do not load brushes, gradients, patterns, ..."), nullptr},
      {"no-fonts", 'f', 0, G_OPTION_ARG_NONE, &mno_fonts,
       N_("Do not load any fonts"), nullptr},
      {"no-splash", 's', 0, G_OPTION_ARG_NONE, &mno_splash,
       N_("Do not show a splash screen"), nullptr},
      {"no-shm", 0, G_OPTION_FLAG_REVERSE, G_OPTION_ARG_NONE, &muse_shm,
       N_("Do not use shared memory between GIMP and plug-ins"), nullptr},
      {"no-cpu-accel", 0, G_OPTION_FLAG_REVERSE, G_OPTION_ARG_NONE,
       &muse_cpu_accel, N_("Do not use special CPU acceleration functions"),
       nullptr},
      {"session", 0, 0, G_OPTION_ARG_FILENAME, &msession_name,
       N_("Use an alternate sessionrc file"), "<name>"},
      {"gimprc", 'g', 0, G_OPTION_ARG_FILENAME, &muser_gimprc,
       N_("Use an alternate user gimprc file"), "<filename>"},
      {"system-gimprc", 0, 0, G_OPTION_ARG_FILENAME, &msystem_gimprc,
       N_("Use an alternate system gimprc file"), "<filename>"},
      {"batch", 'b', 0, G_OPTION_ARG_STRING_ARRAY, &mbatch_commands,
       N_("Batch command to run (can be used multiple times)"), "<command>"},
      {"batch-interpreter", 0, 0, G_OPTION_ARG_STRING, &mbatch_interpreter,
       N_("The procedure to process batch commands with"), "<proc>"},
      {"quit", 0, 0, G_OPTION_ARG_NONE, &mquit,
       N_("Quit immediately after performing requested actions"), nullptr},
      {"console-messages", 'c', 0, G_OPTION_ARG_NONE, &mconsole_messages,
       N_("Send messages to console instead of using a dialog"), nullptr},
      // Don't translate the mode names (off|on|warn).
      {"pdb-compat-mode", 0, 0, G_OPTION_ARG_CALLBACK,
       reinterpret_cast<gpointer>(&option_pdb_compat_mode),
       N_("PDB compatibility mode (off|on|warn)"), "<mode>"},
      // Don't translate the mode names (never|query|always).
      {"stack-trace-mode", 0, 0, G_OPTION_ARG_CALLBACK,
       reinterpret_cast<gpointer>(&option_stack_trace_mode),
       N_("Debug in case of a crash (never|query|always)"), "<mode>"},
      {"debug-handlers", 0, 0, G_OPTION_ARG_NONE, &muse_debug_handler,
       N_("Enable non-fatal debugging signal handlers"), nullptr},
      {"g-fatal-warnings", 0, G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK,
       reinterpret_cast<gpointer>(&option_fatal_warnings),
       N_("Make all warnings fatal"), nullptr},
      {"dump-gimprc", 0, G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK,
       reinterpret_cast<gpointer>(&option_dump_gimprc),
       N_("Output a gimprc file with default settings"), nullptr},
      {"dump-gimprc-system", 0, G_OPTION_FLAG_NO_ARG | G_OPTION_FLAG_HIDDEN,
       G_OPTION_ARG_CALLBACK, reinterpret_cast<gpointer>(&option_dump_gimprc),
       nullptr, nullptr},
      {"dump-gimprc-manpage", 0, G_OPTION_FLAG_NO_ARG | G_OPTION_FLAG_HIDDEN,
       G_OPTION_ARG_CALLBACK, reinterpret_cast<gpointer>(&option_dump_gimprc),
       nullptr, nullptr},
      {"dump-pdb-procedures-deprecated", 0,
       G_OPTION_FLAG_NO_ARG | G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_CALLBACK,
       reinterpret_cast<gpointer>(&option_dump_pdb_procedures_deprecated),
       N_("Output a sorted list of deprecated procedures in the PDB"), nullptr},
      {"show-playground", 0, 0, G_OPTION_ARG_NONE, &mshow_playground,
       N_("Show a preferences page with experimental features"), nullptr},
      {"show-debug-menu", 0, 0, G_OPTION_ARG_NONE, &mshow_debug_menu,
       N_("Show an image submenu with debug actions"), nullptr},
      {G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_FILENAME_ARRAY, &mfilenames,
       nullptr, nullptr},
      {nullptr, 0, 0, G_OPTION_ARG_NONE, nullptr, nullptr, nullptr}};

  GOptionContext* const context = g_option_context_new(_("[FILE|URI...]"));

  g_option_context_set_summary(context, GIMP_NAME);

  // The main group gives the instance to the option callbacks.
  GOptionGroup* const mainGroup =
      g_option_group_new(nullptr, nullptr, nullptr, this, nullptr);

  g_option_group_add_entries(mainGroup, entries);
  g_option_group_set_translation_domain(mainGroup, GETTEXT_PACKAGE);
  g_option_context_set_main_group(context, mainGroup);

  // The GIMP option group is just an empty option group, created for the sole
  // purpose of running a post-parse hook before any other of dependent
  // libraries are run. This makes it possible to apply options from
  // configuration data obtained from "gimprc" files, before other libraries
  // have a chance to run some of their initialization code.
  GOptionGroup* const gimpGroup =
      g_option_group_new("gimp", "", "", this, nullptr);

  g_option_group_set_parse_hooks(gimpGroup, nullptr, &options_group_parse_hook);
  g_option_context_add_group(context, gimpGroup);

  app_libs_init(context, mno_interface);

  return context;
}

void GIMPController::report_parse_error(GError* error)
{
  if (error == nullptr) {
    g_print("%s\n",
            _("GIMP could not initialize the graphical user interface.\n"
              "Make sure a proper setup for your display environment "
              "exists."));
    return;
  }

  attach_console();

  g_print("%s\n", error->message);
  g_error_free(error);
}

int GIMPController::run(const gchar* fullProgName,
                        const std::string& backtraceFile)
{
  // False positive: the g_return_val_if_fail of the method returns false,
  // which the checker does not see without the GLib headers.
  // cppcheck-suppress knownConditionTrueFalse
  if (!set_flatpak_prgname()) {
    return EXIT_FAILURE;
  }

  if (mno_interface || mbe_verbose || mconsole_messages ||
      mbatch_commands != nullptr) {
    attach_console();
  }

  if (mno_interface) {
    mnew_instance = TRUE;
  }

  int status = EXIT_SUCCESS;

  if (!mnew_instance && run_remote(status)) {
    return status;
  }

  check_sanity();

  return run_app(fullProgName, backtraceFile);
}

char** GIMPController::copy_arguments(char** argv) { return g_strdupv(argv); }

gboolean GIMPController::open_in_running_instance(const gchar** filenames,
                                                  const gboolean asNew)
{
  return gimp_unique_open(filenames, asNew);
}

gboolean GIMPController::has_shared_memory()
{
#if defined(USE_SYSV_SHM) || defined(USE_POSIX_SHM)
  return TRUE;
#else
  return FALSE;
#endif
}

void GIMPController::init_malloc()
{
#ifdef GIMP_GLIB_MEM_PROFILER
  g_mem_set_vtable(glib_mem_profiler_table);
  g_atexit(g_mem_profile);
#endif
}

void GIMPController::init_i18n()
{
  // We may change the locale later if the user specifies a language in the
  // gimprc file. Here we are just initializing the locale according to the
  // environment variables and set up the paths to the message catalogs.
  setlocale(LC_ALL, "");

  init_locale();

  gimp_bind_text_domain(GETTEXT_PACKAGE "-libgimp", gimp_locale_directory());
#ifdef HAVE_BIND_TEXTDOMAIN_CODESET
  bind_textdomain_codeset(GETTEXT_PACKAGE "-libgimp", "UTF-8");
#endif

  gimp_bind_text_domain(GETTEXT_PACKAGE, gimp_locale_directory());
#ifdef HAVE_BIND_TEXTDOMAIN_CODESET
  bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
#endif

#ifdef HAVE_ISO_CODES
  // This iso-codes domain is the only external text domain our core code is
  // using. Let's make sure it's properly initialized.
#ifdef ENABLE_RELOCATABLE_RESOURCES
  gimp_bind_text_domain("iso_639_3", gimp_locale_directory());
#else
  gimp_bind_text_domain("iso_639_3", ISO_CODES_LOCALEDIR);
#endif

#ifdef HAVE_BIND_TEXTDOMAIN_CODESET
  bind_textdomain_codeset("iso_639_3", "UTF-8");
#endif
#endif

  textdomain(GETTEXT_PACKAGE);
}

void GIMPController::early_configuration()
{
  GFile* systemGimprc = msystem_gimprc != nullptr
                            ? g_file_new_for_commandline_arg(msystem_gimprc)
                            : nullptr;
  GFile* userGimprc = muser_gimprc != nullptr
                          ? g_file_new_for_commandline_arg(muser_gimprc)
                          : nullptr;

  // GimpEarlyRc is responsible for reading "gimprc" files for the sole purpose
  // of getting some configuration data that is needed in the early
  // initialization phase.
  GimpEarlyRc* const earlyrc =
      gimp_early_rc_new(systemGimprc, userGimprc, mbe_verbose);

  // Language needs to be determined first, before any GimpContext is
  // instantiated (which happens when the Gimp object is created) because its
  // properties need to be properly localized in the settings language (if
  // different from system language). Otherwise we end up with pieces of GUI
  // always using the system language (cf. bug 787457).
  gchar* const language = gimp_early_rc_get_language(earlyrc);

  // Change the locale if a language is specified.
  language_init(language, nullptr);
  g_free(language);

  apply_early_configuration(earlyrc);

  g_object_unref(earlyrc);

  g_clear_object(&systemGimprc);
  g_clear_object(&userGimprc);
}

bool GIMPController::set_flatpak_prgname()
{
#ifndef GIMP_CONSOLE_COMPILATION
  // The g_set_prgname() can only be called several times since GLib 2.72.0.
  if (mno_interface || glib_check_version(2, 72, 0) != nullptr) {
    return true;
  }

  g_return_val_if_fail(gdk_display_get_default() != nullptr, false);

  GKeyFile* const flatpakKeyfile = g_key_file_new();

  if (
#ifdef GDK_WINDOWING_X11
      !GDK_IS_X11_DISPLAY(gdk_display_get_default()) &&
#endif
      g_key_file_load_from_file(flatpakKeyfile, "/.flatpak-info",
                                G_KEY_FILE_NONE, nullptr)) {
    // Flatpak renames the desktop file. The .flatpak-info file tells us the
    // right desktop name we must associate our process to, especially as we
    // have flatpaks with different IDs.
    //
    // This logic should not apply on X11 which will instead apparently use
    // the StartupWMClass set in the desktop file and expect it to be the same
    // as the name set by g_set_prgname().
    //
    // Cf. #13183 and #14233.
    gchar* const flatpakName =
        g_key_file_get_string(flatpakKeyfile, "Application", "name", nullptr);

    // A missing name should never happen unless we are in some kind of broken
    // flatpak environment or somehow in a non-flatpak environment with a
    // .flatpak-info file at the root, which seems improbable. Fail silently.
    if (flatpakName != nullptr) {
      g_set_prgname(flatpakName);
      g_free(flatpakName);
    }
  }

  g_key_file_free(flatpakKeyfile);
#endif

  return true;
}

bool GIMPController::run_remote(int& status)
{
  if (!open_in_running_instance(mfilenames, mas_new)) {
    return false;
  }

  status = EXIT_SUCCESS;

  if (mbe_verbose) {
    g_print("%s\n", _("Another GIMP instance is already running."));
  }

  if (mbatch_commands != nullptr &&
      !gimp_unique_batch_run(mbatch_interpreter, mbatch_commands)) {
    status = EXIT_FAILURE;
  }

  // The console binary never gets here, as it always runs a new instance.
#ifndef GIMP_CONSOLE_COMPILATION
  gdk_notify_startup_complete();
#endif

  return true;
}

void GIMPController::check_sanity()
{
  const gchar* const abortMessage = sanity_check_early();

  if (abortMessage != nullptr) {
    app_abort(mno_interface, abortMessage);
  }
}

int GIMPController::run_app(const gchar* fullProgName,
                            const std::string& backtraceFile)
{
  GFile* systemGimprc = msystem_gimprc != nullptr
                            ? g_file_new_for_commandline_arg(msystem_gimprc)
                            : nullptr;
  GFile* userGimprc = muser_gimprc != nullptr
                          ? g_file_new_for_commandline_arg(muser_gimprc)
                          : nullptr;

  const gint retval = app_run(
      fullProgName, mfilenames, systemGimprc, userGimprc, msession_name,
      mbatch_interpreter, mbatch_commands, mquit, mas_new, mno_interface,
      mno_data, mno_fonts, mno_splash, mbe_verbose,
      muse_shm && has_shared_memory(), muse_cpu_accel, mconsole_messages,
      muse_debug_handler, mshow_playground, mshow_debug_menu,
      static_cast<GimpStackTraceMode>(mstack_trace_mode),
      static_cast<GimpPDBCompatMode>(mpdb_compat_mode),
      backtraceFile.empty() ? nullptr : backtraceFile.c_str());

  gimp_env_exit(FALSE);

  g_clear_object(&systemGimprc);
  g_clear_object(&userGimprc);

  return retval;
}

gboolean GIMPController::show_version_and_exit(const gchar*, const gchar*,
                                               gpointer data, GError**) noexcept
{
  GIMPController* const controller = static_cast<GIMPController*>(data);

  controller->attach_console();
  controller->show_version();

  app_exit(EXIT_SUCCESS);

  return TRUE;
}

gboolean GIMPController::show_license_and_exit(const gchar*, const gchar*,
                                               gpointer data, GError**) noexcept
{
  GIMPController* const controller = static_cast<GIMPController*>(data);

  controller->attach_console();
  controller->show_version();

  g_print("\n%s\n\n", GIMP_LICENSE);

  app_exit(EXIT_SUCCESS);

  return TRUE;
}

gboolean GIMPController::option_fatal_warnings(const gchar*, const gchar*,
                                               gpointer, GError**) noexcept
{
  const GLogLevelFlags fatalMask =
      g_log_set_always_fatal(static_cast<GLogLevelFlags>(G_LOG_FATAL_MASK));

  g_log_set_always_fatal(static_cast<GLogLevelFlags>(
      fatalMask | G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL));

  return TRUE;
}

gboolean GIMPController::option_stack_trace_mode(const gchar*,
                                                 const gchar* value,
                                                 gpointer data,
                                                 GError**) noexcept
{
  GIMPController* const controller = static_cast<GIMPController*>(data);

  if (std::strcmp(value, "never") == 0) {
    controller->mstack_trace_mode = GIMP_STACK_TRACE_NEVER;
  } else if (std::strcmp(value, "query") == 0) {
    controller->mstack_trace_mode = GIMP_STACK_TRACE_QUERY;
  } else if (std::strcmp(value, "always") == 0) {
    controller->mstack_trace_mode = GIMP_STACK_TRACE_ALWAYS;
  } else {
    return FALSE;
  }

  return TRUE;
}

gboolean GIMPController::option_pdb_compat_mode(const gchar*,
                                                const gchar* value,
                                                gpointer data,
                                                GError**) noexcept
{
  GIMPController* const controller = static_cast<GIMPController*>(data);

  if (std::strcmp(value, "off") == 0) {
    controller->mpdb_compat_mode = GIMP_PDB_COMPAT_OFF;
  } else if (std::strcmp(value, "on") == 0) {
    controller->mpdb_compat_mode = GIMP_PDB_COMPAT_ON;
  } else if (std::strcmp(value, "warn") == 0) {
    controller->mpdb_compat_mode = GIMP_PDB_COMPAT_WARN;
  } else {
    return FALSE;
  }

  return TRUE;
}

gboolean GIMPController::option_dump_gimprc(const gchar* optionName,
                                            const gchar*, gpointer data,
                                            GError**) noexcept
{
  GimpConfigDumpFormat format = GIMP_CONFIG_DUMP_NONE;

  static_cast<GIMPController*>(data)->attach_console();

  if (std::strcmp(optionName, "--dump-gimprc") == 0) {
    format = GIMP_CONFIG_DUMP_GIMPRC;
  } else if (std::strcmp(optionName, "--dump-gimprc-system") == 0) {
    format = GIMP_CONFIG_DUMP_GIMPRC_SYSTEM;
  } else if (std::strcmp(optionName, "--dump-gimprc-manpage") == 0) {
    format = GIMP_CONFIG_DUMP_GIMPRC_MANPAGE;
  }

  if (format == GIMP_CONFIG_DUMP_NONE) {
    return FALSE;
  }

  babl_init();

  Gimp* const gimp = static_cast<Gimp*>(g_object_new(GIMP_TYPE_GIMP, nullptr));

  gimp_load_config(gimp, nullptr, nullptr);

  const gboolean success = gimp_config_dump(G_OBJECT(gimp), format);

  g_object_unref(gimp);

  app_exit(success ? EXIT_SUCCESS : EXIT_FAILURE);

  return FALSE;
}

gboolean GIMPController::option_dump_pdb_procedures_deprecated(
    const gchar*, const gchar*, gpointer, GError**) noexcept
{
  babl_init();

  Gimp* const gimp = static_cast<Gimp*>(g_object_new(GIMP_TYPE_GIMP, nullptr));

  gimp_load_config(gimp, nullptr, nullptr);

  // Make sure to turn on compatibility mode so deprecated procedures are
  // included.
  gimp->pdb_compat_mode = GIMP_PDB_COMPAT_ON;

  // Initialize the list of procedures.
  internal_procs_init(gimp->pdb);

  // Get deprecated procedures.
  GList* const deprecatedProcs = gimp_pdb_get_deprecated_procedures(gimp->pdb);

  for (GList* iter = deprecatedProcs; iter != nullptr;
       iter = g_list_next(iter)) {
    g_print("%s\n", gimp_object_get_name(GIMP_PROCEDURE(iter->data)));
  }

  g_list_free(deprecatedProcs);

  g_object_unref(gimp);

  app_exit(EXIT_SUCCESS);

  return FALSE;
}

gboolean GIMPController::options_group_parse_hook(GOptionContext*,
                                                  GOptionGroup*, gpointer data,
                                                  GError**) noexcept
{
  // Early initialization from data stored in "gimprc" files.
  static_cast<GIMPController*>(data)->early_configuration();

  return TRUE;
}

}  // namespace gimp_controller
