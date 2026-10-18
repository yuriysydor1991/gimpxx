#include "src/app/applications/Application.h"

#include <cassert>
#include <cstdlib>
#include <memory>
#include <string>

#include "src/gimp/app/main.h"
#include "src/log/log.h"

namespace app
{

int Application::run(std::shared_ptr<ApplicationContext> ctx)
{
  assert(ctx != nullptr);

  if (ctx == nullptr) {
    LOGE("No valid application context pointer provided");
    return INVALID;
  }

  if (!gimp_main_set_flatpak_prgname()) {
    return INVALID;
  }

  GimpMainOptions& options = ctx->get_options();

  if (options.no_interface || options.be_verbose || options.console_messages ||
      options.batch_commands != nullptr) {
    gimp_main_attach_console();
  }

  if (options.no_interface) {
    options.new_instance = TRUE;
  }

  gint status = EXIT_SUCCESS;

  if (!options.new_instance && gimp_main_run_remote(&status)) {
    return status;
  }

  gimp_main_check_sanity();

  const std::string& backtraceFile = ctx->get_backtrace_file();

  return gimp_main_run(ctx->get_argv()[0],
                       backtraceFile.empty() ? nullptr : backtraceFile.c_str());
}

}  // namespace app
