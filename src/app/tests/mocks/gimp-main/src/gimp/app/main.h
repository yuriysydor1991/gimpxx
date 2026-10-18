#ifndef GIMPXX_GIMP_MAIN_SEAM_DECLARATIONS_H
#define GIMPXX_GIMP_MAIN_SEAM_DECLARATIONS_H

#include <glib.h>
#include <gmock/gmock.h>

#include <initializer_list>

struct GimpMainOptions
{
  gboolean be_verbose;
  gboolean no_interface;
  gboolean new_instance;
  gboolean console_messages;
  const gchar** batch_commands;
};

class gimpMainMock
{
 public:
  /// @brief Gives the mocks of the start-up steps. They are created at the
  /// first use, after the mock registry of a static gmock library, so they are
  /// destroyed before it too.
  static gimpMainMock& instance()
  {
    static gimpMainMock mocks;
    return mocks;
  }

  /// @brief Verifies the expectations of the finished test case and restores
  /// the defaults: zeroed options, a GLib copy of the arguments, an option
  /// context which accepts every option without printing the help and a named
  /// process.
  void reset()
  {
    for (void* const mock : std::initializer_list<void*>{
             &gimp_main_init_platform, &gimp_main_init_signals,
             &gimp_main_init_process, &gimp_main_attach_console,
             &gimp_main_show_version, &gimp_main_option_context_new,
             &gimp_main_report_parse_error, &gimp_main_set_flatpak_prgname,
             &gimp_main_run_remote, &gimp_main_check_sanity, &gimp_main_run}) {
      testing::Mock::VerifyAndClear(mock);
    }

    options = {};

    ON_CALL(gimp_main_init_process, Call(testing::_)).WillByDefault(&g_strdupv);
    ON_CALL(gimp_main_option_context_new, Call()).WillByDefault([]() {
      GOptionContext* const context = g_option_context_new(nullptr);

      g_option_context_set_help_enabled(context, FALSE);
      g_option_context_set_ignore_unknown_options(context, TRUE);

      return context;
    });
    ON_CALL(gimp_main_set_flatpak_prgname, Call())
        .WillByDefault(testing::Return(TRUE));
  }

  GimpMainOptions options{};

  testing::MockFunction<void(gint*, gchar***)> gimp_main_init_platform;
  testing::MockFunction<gchar*()> gimp_main_init_signals;
  testing::MockFunction<gchar**(gchar**)> gimp_main_init_process;
  testing::MockFunction<void()> gimp_main_attach_console;
  testing::MockFunction<void()> gimp_main_show_version;
  testing::MockFunction<GOptionContext*()> gimp_main_option_context_new;
  testing::MockFunction<void(GError*)> gimp_main_report_parse_error;
  testing::MockFunction<gboolean()> gimp_main_set_flatpak_prgname;
  testing::MockFunction<gboolean(gint*)> gimp_main_run_remote;
  testing::MockFunction<void()> gimp_main_check_sanity;
  testing::MockFunction<gint(const gchar*, const gchar*)> gimp_main_run;
};

inline GimpMainOptions* gimp_main_options()
{
  return &gimpMainMock::instance().options;
}

inline void gimp_main_init_platform(gint* argc, gchar*** argv)
{
  gimpMainMock::instance().gimp_main_init_platform.Call(argc, argv);
}

inline gchar* gimp_main_init_signals()
{
  return gimpMainMock::instance().gimp_main_init_signals.Call();
}

inline gchar** gimp_main_init_process(gchar** argv)
{
  return gimpMainMock::instance().gimp_main_init_process.Call(argv);
}

inline void gimp_main_attach_console()
{
  gimpMainMock::instance().gimp_main_attach_console.Call();
}

inline void gimp_main_show_version()
{
  gimpMainMock::instance().gimp_main_show_version.Call();
}

inline GOptionContext* gimp_main_option_context_new()
{
  return gimpMainMock::instance().gimp_main_option_context_new.Call();
}

/// @brief Takes the error over, as the real one does.
inline void gimp_main_report_parse_error(GError* error)
{
  gimpMainMock::instance().gimp_main_report_parse_error.Call(error);
  g_clear_error(&error);
}

inline gboolean gimp_main_set_flatpak_prgname()
{
  return gimpMainMock::instance().gimp_main_set_flatpak_prgname.Call();
}

inline gboolean gimp_main_run_remote(gint* exitStatus)
{
  return gimpMainMock::instance().gimp_main_run_remote.Call(exitStatus);
}

inline void gimp_main_check_sanity()
{
  gimpMainMock::instance().gimp_main_check_sanity.Call();
}

inline gint gimp_main_run(const gchar* fullProgName, const gchar* backtraceFile)
{
  return gimpMainMock::instance().gimp_main_run.Call(fullProgName,
                                                     backtraceFile);
}

#endif  // GIMPXX_GIMP_MAIN_SEAM_DECLARATIONS_H
