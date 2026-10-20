#include <windows.h>

#include <cstdlib>

#include "src/app/ApplicationFactory.h"

/**
 * @brief Provides WinMain in case we build GIMP as a subsystem:windows
 * application. Well, we do. When built with mingw, though, user code execution
 * still starts in main() in that case. So WinMain() gets used on MSVC builds
 * only, and starts the application the way the main() does.
 *
 * @return Returns the application exit status.
 */
// The C runtime calls it.
// cppcheck-suppress unusedFunction
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
  return app::ApplicationFactory::execute(__argc, __argv);
}
