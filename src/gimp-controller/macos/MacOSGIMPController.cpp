#include "src/gimp-controller/macos/MacOSGIMPController.h"

#include <sys/resource.h>

#include <clocale>

#include "src/gimp-controller/gimp-c-includes.h"

namespace gimp_controller
{

void MacOSGIMPController::attach_console() {}

void MacOSGIMPController::fix_arguments([[maybe_unused]] int& argc,
                                        [[maybe_unused]] char**& argv)
{
#ifdef ENABLE_RELOCATABLE_RESOURCES
  // Remove MacOS session identifier from the command line args.
  int newArgc = 0;

  for (int iter = 0; iter < argc; ++iter) {
    if (!g_str_has_prefix(argv[iter], "-psn_")) {
      argv[newArgc] = argv[iter];
      ++newArgc;
    }
  }

  if (argc > newArgc) {
    // GLib expects NULL terminated array.
    argv[newArgc] = nullptr;
    argc = newArgc;
  }
#endif
}

void MacOSGIMPController::init_environment(
    [[maybe_unused]] const gchar* progName)
{
  // These need to be set regardless of whether GIMP is relocatable or not.
  if (g_getenv("HOME") != nullptr) {
    // Use standard ~/Library/Caches/GIMP on macOS, not a subdirectory of the
    // config directory like in the past (~/Library/Application Support/GIMP).
    // Various libraries create their cache directory very early at startup,
    // before GIMP gets a chance to check if the config directory already
    // exists, so various dirs like e.g. "brushes" get skipped. See: #4958.
    set_path_variable("XDG_CACHE_HOME", g_getenv("HOME"),
                      "Library/Caches/GIMP/" GIMP_APP_VERSION);
  }

  g_setenv("GTK_IM_MODULE", "quartz", TRUE);

#ifdef ENABLE_RELOCATABLE_RESOURCES
  // Set the environment variables for GIMP to be relocatable. Due to the
  // changes on macOS 10.15, it is not recommended to set it in a shell wrapper
  // inside Contents/MacOS (like AppImage's AppRun) anymore. LSEnvironment on
  // Info.plist, limited to bundle scope, would not be enough. That way, we
  // make sure our python is called instead of system one etc.

  // On some OSX installations open file limit is 256 and GIMP needs more.
  rlimit limit{};

  limit.rlim_cur = 10000;
  limit.rlim_max = 10000;
  setrlimit(RLIMIT_NOFILE, &limit);

  gchar* const resolvedPath = g_canonicalize_filename(progName, nullptr);

  if (resolvedPath == nullptr || g_getenv("GIMP_NO_WRAPPER") != nullptr) {
    g_free(resolvedPath);
    return;
  }

  gchar* const binDir = g_path_get_dirname(resolvedPath);
  gchar* shareDir = new_installation_path("share");

  g_free(resolvedPath);

  // Detect if we are running from bundle or from prefix.
  if (g_str_has_suffix(binDir, "MacOS")) {
    g_printerr("GIMP is started as MacOS application\n");
  } else if (g_file_test(shareDir, G_FILE_TEST_IS_DIR)) {
    g_print("GIMP is started in the build directory\n");

    g_free(shareDir);
    shareDir = g_canonicalize_filename(gimp_installation_directory(), nullptr);
  } else {
    g_free(shareDir);
    g_free(binDir);
    return;
  }

  gchar* const libDir = new_installation_path("lib");
  gchar* const etcDir = new_installation_path("etc");

  // Minimum runtime paths.
  prepend_path_variable("PATH", binDir);
  prepend_path_variable("XDG_DATA_DIRS", shareDir);

  // Bare minimum to run GTK apps.
  set_path_variable("GIO_MODULE_DIR", libDir, "gio/modules");
  set_path_variable("GDK_PIXBUF_MODULE_FILE", libDir,
                    "gdk-pixbuf-2.0/2.10.0/loaders.cache");
  set_path_variable("GTK_PATH", libDir, "gtk-3.0/3.0.0");
  set_path_variable("GTK_IM_MODULE_FILE", libDir,
                    "gtk-3.0/3.0.0/immodules.cache");

  // Other needed runtime paths (related to features).
  set_path_variable("FONTCONFIG_PATH", etcDir, "fonts");
  set_path_variable("LIBTHAI_DICTDIR", shareDir, "libthai");
  set_path_variable("GS_LIB", shareDir, "ghostscript/Resource/Init");
  set_path_variable("WMF_FONTDIR", shareDir, "libwmf/fonts");
  set_path_variable("GI_TYPELIB_PATH", libDir, "girepository-1.0");

  gchar* const pythonHome =
      g_strdup_printf("%s/lib/Python.framework/Versions/%s",
                      gimp_installation_directory(), PYTHON_VERSION);

  if (g_file_test(pythonHome, G_FILE_TEST_IS_DIR)) {
    g_setenv("PYTHONHOME", pythonHome, TRUE);
  }

  g_free(pythonHome);
  g_free(etcDir);
  g_free(libDir);
  g_free(shareDir);
  g_free(binDir);
#endif
}

void MacOSGIMPController::prepare_process() {}

void MacOSGIMPController::init_locale()
{
  // Keep the monetary sign characters ASCII so GtkSpinButton accepts the '-'
  // (minus) character. See GNOME/gimp#13641 and GNOME/GTK!8802.
  setlocale(LC_MONETARY, "C");
}

void MacOSGIMPController::apply_early_configuration(GimpEarlyRc*) {}

gchar* MacOSGIMPController::new_installation_path(const gchar* relativePath)
{
  gchar* const path =
      g_build_filename(gimp_installation_directory(), relativePath, nullptr);
  gchar* const canonicalPath = g_canonicalize_filename(path, nullptr);

  g_free(path);

  return canonicalPath;
}

void MacOSGIMPController::set_path_variable(const gchar* name, const gchar* dir,
                                            const gchar* relativePath)
{
  gchar* const value = g_strdup_printf("%s/%s", dir, relativePath);

  g_setenv(name, value, TRUE);
  g_free(value);
}

void MacOSGIMPController::prepend_path_variable(const gchar* name,
                                                const gchar* dir)
{
  const gchar* const current = g_getenv(name);
  gchar* const value = current != nullptr
                           ? g_strdup_printf("%s:%s", dir, current)
                           : g_strdup(dir);

  g_setenv(name, value, TRUE);
  g_free(value);
}

}  // namespace gimp_controller
