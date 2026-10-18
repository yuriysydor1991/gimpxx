#include "src/app/signals-handlers/GimpSignalsHandler.h"

#include <cassert>
#include <memory>

#include "src/gimp/app/main.h"
#include "src/log/log.h"

namespace app
{

bool GimpSignalsHandler::install(std::shared_ptr<ApplicationContext> ctx)
{
  assert(ctx != nullptr);

  if (ctx == nullptr) {
    LOGE("No valid application context pointer provided");
    return false;
  }

  gchar* const backtraceFile = gimp_main_init_signals();

  if (backtraceFile != nullptr) {
    ctx->set_backtrace_file(backtraceFile);
    g_free(backtraceFile);
  }

  return true;
}

void GimpSignalsHandler::uninstall() {}

}  // namespace app
