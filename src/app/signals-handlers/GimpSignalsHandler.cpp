#include "src/app/signals-handlers/GimpSignalsHandler.h"

#include <cassert>
#include <memory>

#include "src/gimp-controller/GIMPController.h"
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

  ctx->set_backtrace_file(ctx->get_gimp_controller().init_signals());

  return true;
}

void GimpSignalsHandler::uninstall() {}

}  // namespace app
