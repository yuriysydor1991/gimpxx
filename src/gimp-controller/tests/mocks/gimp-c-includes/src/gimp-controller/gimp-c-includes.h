#ifndef GIMPXX_PROJECT_GIMP_C_INCLUDES_H
#define GIMPXX_PROJECT_GIMP_C_INCLUDES_H

#include <gio/gio.h>
#include <glib/gi18n.h>
#include <gmock/gmock.h>

#include <initializer_list>

// The config.h values the GIMPController uses. The build kind macros, like
// the GIMP_RELEASE one, come from the test targets.
#define GETTEXT_PACKAGE "gimp30"
#define GIMP_APP_VERSION "3.2"
#define GIMP_DESKTOP_NAME "gimp"
#define GIMP_MUTEX_VERSION "3"
#define HAVE_BIND_TEXTDOMAIN_CODESET 1
#define HAVE_ISO_CODES 1
#define ISO_CODES_LOCALEDIR "/iso-codes/locale"
#define PYTHON_VERSION "3.14"
#define USE_SYSV_SHM 1

// The about.h texts.
#define GIMP_NAME "GIMPxx, based on GIMP"
#define GIMP_LICENSE "GIMP is free software"

typedef struct _GdkDisplay GdkDisplay;
typedef struct _GimpEarlyRc GimpEarlyRc;
typedef struct _GimpPDB GimpPDB;
typedef struct _GimpProcedure GimpProcedure;

typedef enum
{
  GIMP_STACK_TRACE_NEVER,
  GIMP_STACK_TRACE_QUERY,
  GIMP_STACK_TRACE_ALWAYS
} GimpStackTraceMode;

typedef enum
{
  GIMP_PDB_COMPAT_OFF,
  GIMP_PDB_COMPAT_ON,
  GIMP_PDB_COMPAT_WARN
} GimpPDBCompatMode;

typedef enum
{
  GIMP_CONFIG_DUMP_NONE,
  GIMP_CONFIG_DUMP_GIMPRC,
  GIMP_CONFIG_DUMP_GIMPRC_SYSTEM,
  GIMP_CONFIG_DUMP_GIMPRC_MANPAGE
} GimpConfigDumpFormat;

/// @brief The fields of the GIMP root object the GIMPController uses.
typedef struct _Gimp
{
  GObject parent_instance;
  GimpPDBCompatMode pdb_compat_mode;
  GimpPDB* pdb;
} Gimp;

/// @brief Registers the GObject type of the GIMP root object mock.
// Used through the GIMP_TYPE_GIMP macro only.
// cppcheck-suppress unusedFunction
inline GType gimp_get_type()
{
  static const GType type = g_type_register_static_simple(
      G_TYPE_OBJECT, "GimpxxMockGimp", sizeof(GObjectClass), nullptr,
      sizeof(Gimp), nullptr, static_cast<GTypeFlags>(0));

  return type;
}

#define GIMP_TYPE_GIMP (gimp_get_type())
#define GIMP_PROCEDURE(obj) (static_cast<GimpProcedure*>(obj))

/// @brief The arguments of an app_run call.
struct AppRunArgs
{
  const gchar* full_prog_name;
  const gchar** filenames;
  GFile* alternate_system_gimprc;
  GFile* alternate_gimprc;
  const gchar* session_name;
  const gchar* batch_interpreter;
  const gchar** batch_commands;
  gboolean quit;
  gboolean as_new;
  gboolean no_interface;
  gboolean no_data;
  gboolean no_fonts;
  gboolean no_splash;
  gboolean be_verbose;
  gboolean use_shm;
  gboolean use_cpu_accel;
  gboolean console_messages;
  gboolean use_debug_handler;
  gboolean show_playground;
  gboolean show_debug_menu;
  GimpStackTraceMode stack_trace_mode;
  GimpPDBCompatMode pdb_compat_mode;
  const gchar* backtrace_file;
};

/// @brief The mocks of the GIMP C functions. They are the nice ones, as every
/// test case expects the calls it checks only.
class gimpCMock
{
 public:
  /// @brief Gives the mocks of the GIMP C functions. They are created at the
  /// first use, after the mock registry of a static gmock library, so they are
  /// destroyed before it too.
  static gimpCMock& instance()
  {
    static gimpCMock mocks;
    return mocks;
  }

  /// @brief Verifies the expectations of the finished test case and restores
  /// the defaults: a real GObject as the early configuration, the display of
  /// the graphical user interface and the GIMP directories.
  void reset()
  {
    for (void* const mock :
         std::initializer_list<void*>{&gimp_backtrace_init,
                                      &gimp_init_signal_handlers,
                                      &gimp_env_init,
                                      &gimp_env_exit,
                                      &gimp_log_init,
                                      &gimp_bind_text_domain,
                                      &gimp_locale_directory,
                                      &gimp_installation_directory,
                                      &gimp_early_rc_new,
                                      &gimp_early_rc_get_language,
                                      &language_init,
                                      &app_libs_init,
                                      &app_abort,
                                      &app_exit,
                                      &app_run,
                                      &sanity_check_early,
                                      &gimp_unique_open,
                                      &gimp_unique_batch_run,
                                      &gimp_version_show,
                                      &gdk_display_get_default,
                                      &gdk_notify_startup_complete,
                                      &babl_init,
                                      &gimp_load_config,
                                      &gimp_config_dump,
                                      &internal_procs_init,
                                      &gimp_pdb_get_deprecated_procedures,
                                      &gimp_object_get_name}) {
      testing::Mock::VerifyAndClear(mock);
    }

    ON_CALL(gimp_early_rc_new, Call(testing::_, testing::_, testing::_))
        .WillByDefault([](GFile*, GFile*, gboolean) {
          return static_cast<GimpEarlyRc*>(
              g_object_new(G_TYPE_OBJECT, nullptr));
        });
    ON_CALL(gdk_display_get_default, Call())
        .WillByDefault(testing::Return(reinterpret_cast<GdkDisplay*>(this)));
    ON_CALL(gimp_locale_directory, Call())
        .WillByDefault(testing::Return("/gimp/share/locale"));
    ON_CALL(gimp_installation_directory, Call())
        .WillByDefault(testing::Return("/gimp"));
  }

  testing::NiceMock<testing::MockFunction<void()>> gimp_backtrace_init;
  testing::NiceMock<testing::MockFunction<void(gchar**)>>
      gimp_init_signal_handlers;
  testing::NiceMock<testing::MockFunction<void(gboolean)>> gimp_env_init;
  testing::NiceMock<testing::MockFunction<void(gboolean)>> gimp_env_exit;
  testing::NiceMock<testing::MockFunction<void()>> gimp_log_init;
  testing::NiceMock<testing::MockFunction<void(const gchar*, const gchar*)>>
      gimp_bind_text_domain;
  testing::NiceMock<testing::MockFunction<const gchar*()>>
      gimp_locale_directory;
  testing::NiceMock<testing::MockFunction<const gchar*()>>
      gimp_installation_directory;
  testing::NiceMock<
      testing::MockFunction<GimpEarlyRc*(GFile*, GFile*, gboolean)>>
      gimp_early_rc_new;
  testing::NiceMock<testing::MockFunction<gchar*(GimpEarlyRc*)>>
      gimp_early_rc_get_language;
  testing::NiceMock<
      testing::MockFunction<const gchar*(const gchar*, const gchar**)>>
      language_init;
  testing::NiceMock<testing::MockFunction<void(GOptionContext*, gboolean)>>
      app_libs_init;
  testing::NiceMock<testing::MockFunction<void(gboolean, const gchar*)>>
      app_abort;
  testing::NiceMock<testing::MockFunction<void(gint)>> app_exit;
  testing::NiceMock<testing::MockFunction<gint(const AppRunArgs&)>> app_run;
  testing::NiceMock<testing::MockFunction<const gchar*()>> sanity_check_early;
  testing::NiceMock<testing::MockFunction<gboolean(const gchar**, gboolean)>>
      gimp_unique_open;
  testing::NiceMock<
      testing::MockFunction<gboolean(const gchar*, const gchar**)>>
      gimp_unique_batch_run;
  testing::NiceMock<testing::MockFunction<void(gboolean)>> gimp_version_show;
  testing::NiceMock<testing::MockFunction<GdkDisplay*()>>
      gdk_display_get_default;
  testing::NiceMock<testing::MockFunction<void()>> gdk_notify_startup_complete;
  testing::NiceMock<testing::MockFunction<void()>> babl_init;
  testing::NiceMock<testing::MockFunction<void(Gimp*, GFile*, GFile*)>>
      gimp_load_config;
  testing::NiceMock<
      testing::MockFunction<gboolean(GObject*, GimpConfigDumpFormat)>>
      gimp_config_dump;
  testing::NiceMock<testing::MockFunction<void(GimpPDB*)>> internal_procs_init;
  testing::NiceMock<testing::MockFunction<GList*(GimpPDB*)>>
      gimp_pdb_get_deprecated_procedures;
  testing::NiceMock<testing::MockFunction<const gchar*(gconstpointer)>>
      gimp_object_get_name;
};

inline void gimp_backtrace_init()
{
  gimpCMock::instance().gimp_backtrace_init.Call();
}

inline void gimp_init_signal_handlers(gchar** backtraceFile)
{
  gimpCMock::instance().gimp_init_signal_handlers.Call(backtraceFile);
}

inline void gimp_env_init(gboolean plugIn)
{
  gimpCMock::instance().gimp_env_init.Call(plugIn);
}

inline void gimp_env_exit(gboolean plugIn)
{
  gimpCMock::instance().gimp_env_exit.Call(plugIn);
}

inline void gimp_log_init() { gimpCMock::instance().gimp_log_init.Call(); }

inline void gimp_bind_text_domain(const gchar* domainName, const gchar* dirName)
{
  gimpCMock::instance().gimp_bind_text_domain.Call(domainName, dirName);
}

inline const gchar* gimp_locale_directory()
{
  return gimpCMock::instance().gimp_locale_directory.Call();
}

inline const gchar* gimp_installation_directory()
{
  return gimpCMock::instance().gimp_installation_directory.Call();
}

inline GimpEarlyRc* gimp_early_rc_new(GFile* systemGimprc, GFile* userGimprc,
                                      gboolean verbose)
{
  return gimpCMock::instance().gimp_early_rc_new.Call(systemGimprc, userGimprc,
                                                      verbose);
}

inline gchar* gimp_early_rc_get_language(GimpEarlyRc* rc)
{
  return gimpCMock::instance().gimp_early_rc_get_language.Call(rc);
}

inline const gchar* language_init(const gchar* language,
                                  const gchar** systemLangL10n)
{
  return gimpCMock::instance().language_init.Call(language, systemLangL10n);
}

inline void app_libs_init(GOptionContext* context, gboolean noInterface)
{
  gimpCMock::instance().app_libs_init.Call(context, noInterface);
}

/// @brief Returns, unlike the real one, which aborts the process.
inline void app_abort(gboolean noInterface, const gchar* abortMessage)
{
  gimpCMock::instance().app_abort.Call(noInterface, abortMessage);
}

/// @brief Returns, unlike the real one, which exits the process.
inline void app_exit(gint status)
{
  gimpCMock::instance().app_exit.Call(status);
}

inline gint app_run(const gchar* fullProgName, const gchar** filenames,
                    GFile* alternateSystemGimprc, GFile* alternateGimprc,
                    const gchar* sessionName, const gchar* batchInterpreter,
                    const gchar** batchCommands, gboolean quit, gboolean asNew,
                    gboolean noInterface, gboolean noData, gboolean noFonts,
                    gboolean noSplash, gboolean beVerbose, gboolean useShm,
                    gboolean useCpuAccel, gboolean consoleMessages,
                    gboolean useDebugHandler, gboolean showPlayground,
                    gboolean showDebugMenu, GimpStackTraceMode stackTraceMode,
                    GimpPDBCompatMode pdbCompatMode, const gchar* backtraceFile)
{
  return gimpCMock::instance().app_run.Call(AppRunArgs{fullProgName,
                                                       filenames,
                                                       alternateSystemGimprc,
                                                       alternateGimprc,
                                                       sessionName,
                                                       batchInterpreter,
                                                       batchCommands,
                                                       quit,
                                                       asNew,
                                                       noInterface,
                                                       noData,
                                                       noFonts,
                                                       noSplash,
                                                       beVerbose,
                                                       useShm,
                                                       useCpuAccel,
                                                       consoleMessages,
                                                       useDebugHandler,
                                                       showPlayground,
                                                       showDebugMenu,
                                                       stackTraceMode,
                                                       pdbCompatMode,
                                                       backtraceFile});
}

inline const gchar* sanity_check_early()
{
  return gimpCMock::instance().sanity_check_early.Call();
}

inline gboolean gimp_unique_open(const gchar** filenames, gboolean asNew)
{
  return gimpCMock::instance().gimp_unique_open.Call(filenames, asNew);
}

inline gboolean gimp_unique_batch_run(const gchar* batchInterpreter,
                                      const gchar** batchCommands)
{
  return gimpCMock::instance().gimp_unique_batch_run.Call(batchInterpreter,
                                                          batchCommands);
}

inline void gimp_version_show(gboolean beVerbose)
{
  gimpCMock::instance().gimp_version_show.Call(beVerbose);
}

inline GdkDisplay* gdk_display_get_default()
{
  return gimpCMock::instance().gdk_display_get_default.Call();
}

inline void gdk_notify_startup_complete()
{
  gimpCMock::instance().gdk_notify_startup_complete.Call();
}

inline void babl_init() { gimpCMock::instance().babl_init.Call(); }

inline void gimp_load_config(Gimp* gimp, GFile* alternateSystemGimprc,
                             GFile* alternateGimprc)
{
  gimpCMock::instance().gimp_load_config.Call(gimp, alternateSystemGimprc,
                                              alternateGimprc);
}

inline gboolean gimp_config_dump(GObject* gimp, GimpConfigDumpFormat format)
{
  return gimpCMock::instance().gimp_config_dump.Call(gimp, format);
}

inline void internal_procs_init(GimpPDB* pdb)
{
  gimpCMock::instance().internal_procs_init.Call(pdb);
}

inline GList* gimp_pdb_get_deprecated_procedures(GimpPDB* pdb)
{
  return gimpCMock::instance().gimp_pdb_get_deprecated_procedures.Call(pdb);
}

inline const gchar* gimp_object_get_name(gconstpointer object)
{
  return gimpCMock::instance().gimp_object_get_name.Call(object);
}

#endif  // GIMPXX_PROJECT_GIMP_C_INCLUDES_H
