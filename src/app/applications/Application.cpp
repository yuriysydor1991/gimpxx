#include "src/app/applications/Application.h"

#include <cassert>
#include <memory>

#include "src/gimp-controller/GIMPController.h"
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

  return ctx->get_gimp_controller().run(ctx->get_argv()[0],
                                        ctx->get_backtrace_file());
}

}  // namespace app
