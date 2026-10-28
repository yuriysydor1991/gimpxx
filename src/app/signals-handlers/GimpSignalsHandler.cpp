#include "src/app/signals-handlers/GimpSignalsHandler.h"

#include <cassert>
#include <memory>

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

  return true;
}

void GimpSignalsHandler::uninstall() {}

}  // namespace app
