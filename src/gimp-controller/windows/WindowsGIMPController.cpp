#include "src/gimp-controller/windows/WindowsGIMPController.h"

#include <windows.h>

#include <cstdio>
#include <cstdlib>
#include <cwchar>

#include "src/gimp-controller/gimp-c-includes.h"

namespace gimp_controller
{

void WindowsGIMPController::attach_console()
{
#ifndef GIMP_CONSOLE_COMPILATION
  // If run on non-native shell, do nothing.
  if (g_getenv("TERM") != nullptr || g_getenv("SHELL") != nullptr) {
    g_printerr(
        "Non-native shell detected, GIMP may behave unexpectedly on Unix "
        "shells in Windows.\n");
    return;
  }

  // If run on native shell, attach to it.
  if (AttachConsole(ATTACH_PARENT_PROCESS) == 0) {
    return;
  }

  // 'r' is needed to prevent interleaving and '+' to support colors.
#ifndef _UCRT
  // The streams stay as they are if the console can not take them.
  // cppcheck-suppress ignoredReturnValue
  freopen("CONOUT$", "r+", stdout);
  // cppcheck-suppress ignoredReturnValue
  freopen("CONOUT$", "r+", stderr);
#else
  FILE* fileOut = nullptr;
  FILE* fileErr = nullptr;

  const errno_t errOut = freopen_s(&fileOut, "CONOUT$", "r+", stdout);
  const errno_t errErr = freopen_s(&fileErr, "CONOUT$", "r+", stderr);

  if (errOut != 0 || errErr != 0) {
    g_warning("Failed to redirect streams to CONOUT$. stdout: %d, stderr: %d",
              errOut, errErr);
  }
#endif
  _flushall();

  // CTRL+C handling.
  const HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
  DWORD mode = 0;

  GetConsoleMode(hIn, &mode);
  mode |= ENABLE_PROCESSED_INPUT;
  SetConsoleMode(hIn, mode);

  atexit(&wait_console_window);
#endif
}

void WindowsGIMPController::fix_arguments([[maybe_unused]] int& argc,
                                          [[maybe_unused]] char**& argv)
{
#if defined(__GNUC__) && defined(_WIN64)
  // mingw-w64, at least the unstable build from late July 2008, starts
  // subsystem:windows programs in main(), but passes them bogus argc and argv.
  // __argc and __argv are OK, though, so just use them.
  argc = __argc;
  argv = __argv;
#endif
}

void WindowsGIMPController::init_environment(const gchar*)
{
#ifdef ENABLE_RELOCATABLE_RESOURCES
  // Avoid Windows Registry interfering/skipping the bundled GLib schemas. We
  // use Win32 functions to avoid initializing GLib early. See: #13437.
  if (GetEnvironmentVariableW(L"GSETTINGS_SCHEMA_DIR", nullptr, 0) != 0) {
    return;
  }

  wchar_t exePath[MAX_PATH];
  const DWORD exePathLength = GetModuleFileNameW(nullptr, exePath, MAX_PATH);

  if (exePathLength == 0 || exePathLength >= MAX_PATH) {
    return;
  }

  // Strip the executable name, then the bin directory.
  for (int component = 0; component < 2; ++component) {
    wchar_t* lastSlash = wcsrchr(exePath, L'\\');

    if (lastSlash == nullptr) {
      lastSlash = wcsrchr(exePath, L'/');
    }

    if (lastSlash == nullptr) {
      return;
    }

    *lastSlash = L'\0';
  }

  wchar_t envPath[MAX_PATH];

  _snwprintf(envPath, sizeof(envPath) / sizeof(wchar_t),
             L"%ls\\share\\glib-2.0\\schemas", exePath);
  SetEnvironmentVariableW(L"GSETTINGS_SCHEMA_DIR", envPath);
#endif
}

void WindowsGIMPController::prepare_process()
{
  // Make Inno aware of gimp process avoiding broken install/uninstall.
  gchar* const utf8Name = g_strdup_printf("GIMP-%s", GIMP_MUTEX_VERSION);
  wchar_t* const name = reinterpret_cast<wchar_t*>(
      g_utf8_to_utf16(utf8Name, -1, nullptr, nullptr, nullptr));

  CreateMutexW(nullptr, FALSE, name);

  g_free(utf8Name);
  g_free(name);

  // Enable Anti-Aliasing.
  g_setenv("PANGOCAIRO_BACKEND", "fc", TRUE);

  // Reduce risks.
  SetDllDirectoryW(L"");

  // Set the DLL search path to $INSTALLDIR/bin so that .exe plug-ins
  // processes in the plug-ins directory can find libgimp and file library
  // DLLs.
  gchar* const binDir =
      g_build_filename(gimp_installation_directory(), "bin", nullptr);
  wchar_t* const wBinDir = reinterpret_cast<wchar_t*>(
      g_utf8_to_utf16(binDir, -1, nullptr, nullptr, nullptr));

  if (wBinDir != nullptr) {
    SetDllDirectoryW(wBinDir);
    g_free(wBinDir);
  }

#ifdef ENABLE_RELOCATABLE_RESOURCES
  // We also set external PATH variable to other processes just in case (see
  // #14716 and many other issues labeled as 'DLL Hell').
  const gchar* const path = g_getenv("PATH");
  gchar* const newPath = path != nullptr
                             ? g_strdup_printf("%s;%s", binDir, path)
                             : g_strdup(binDir);

  g_setenv("PATH", newPath, TRUE);
  g_free(newPath);
#endif

  g_free(binDir);

#ifndef _WIN64
  typedef BOOL(WINAPI * t_SetProcessDEPPolicy)(DWORD dwFlags);

  const t_SetProcessDEPPolicy p_SetProcessDEPPolicy =
      reinterpret_cast<t_SetProcessDEPPolicy>(GetProcAddress(
          GetModuleHandleW(L"kernel32.dll"), "SetProcessDEPPolicy"));

  if (p_SetProcessDEPPolicy != nullptr) {
    p_SetProcessDEPPolicy(PROCESS_DEP_ENABLE |
                          PROCESS_DEP_DISABLE_ATL_THUNK_EMULATION);
  }
#endif

  // Group all our windows together on the taskbar.
  typedef HRESULT(WINAPI *
                  t_SetCurrentProcessExplicitAppUserModelID)(PCWSTR lpPathName);

  const t_SetCurrentProcessExplicitAppUserModelID
      p_SetCurrentProcessExplicitAppUserModelID =
          reinterpret_cast<t_SetCurrentProcessExplicitAppUserModelID>(
              GetProcAddress(GetModuleHandleW(L"shell32.dll"),
                             "SetCurrentProcessExplicitAppUserModelID"));

  if (p_SetCurrentProcessExplicitAppUserModelID != nullptr) {
    p_SetCurrentProcessExplicitAppUserModelID(L"gimp.GimpApplication");
  }
}

void WindowsGIMPController::init_locale() {}

void WindowsGIMPController::apply_early_configuration(
    [[maybe_unused]] GimpEarlyRc* earlyrc)
{
#ifndef GIMP_CONSOLE_COMPILATION
#if GTK_MAJOR_VERSION > 3
#warning For GTK4 and above use the proper backend-specific API instead of the GDK_WIN32_TABLET_INPUT_API environment variable
#endif

  // Set a GdkWin32-specific environment variable to specify the desired pen /
  // touch input API to use on Windows.
  if (gtk_get_major_version() == 3 &&
      (gtk_get_minor_version() > 24 ||
       (gtk_get_minor_version() == 24 && gtk_get_micro_version() >= 30))) {
    switch (gimp_early_rc_get_win32_pointer_input_api(earlyrc)) {
      case GIMP_WIN32_POINTER_INPUT_API_WINTAB:
        g_setenv("GDK_WIN32_TABLET_INPUT_API", "wintab", TRUE);
        break;
      case GIMP_WIN32_POINTER_INPUT_API_WINDOWS_INK:
        g_setenv("GDK_WIN32_TABLET_INPUT_API", "winpointer", TRUE);
        break;
    }
  }
#endif
}

char** WindowsGIMPController::copy_arguments(char**)
{
  return g_win32_get_command_line();
}

gboolean WindowsGIMPController::open_in_running_instance(
    const gchar** filenames, const gboolean asNew)
{
  // Windows Explorer spawns separate processes when multiple files are
  // selected (except for the MSIX, where it passes each filename as on Unix:
  // 55c97120). The only reliable way to them communicate is a mutex early on.
  // See: #364
  const HANDLE primaryInstance =
      CreateMutexW(nullptr, FALSE, L"Local\\gimp_primary_instance_running");
  const DWORD primaryInstanceError =
      primaryInstance != nullptr ? GetLastError() : 0;

  // This is the primary GIMP instance, which keeps the mutex while it runs.
  if (primaryInstance == nullptr ||
      primaryInstanceError != ERROR_ALREADY_EXISTS) {
    return FALSE;
  }

  // Another GIMP instance is already running.
  gboolean isRunning = FALSE;

  for (int retry = 0; retry < 20; ++retry) {
    isRunning = GIMPController::open_in_running_instance(filenames, asNew);

    if (isRunning) {
      break;
    }

    g_usleep(100000);
  }

  CloseHandle(primaryInstance);

  return isRunning;
}

gboolean WindowsGIMPController::has_shared_memory() { return TRUE; }

void WindowsGIMPController::wait_console_window()
{
  g_print("%s", _("(Type any character to close this window)\n"));
}

}  // namespace gimp_controller
