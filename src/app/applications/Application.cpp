#include "src/app/applications/Application.h"

#include <glib.h>

#include <cassert>
#include <memory>
#include <string>

#include "src/gimp-controller/GIMPControllerFactory.h"
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

  gimp_controller::GIMPControllerFactory factory;
  std::shared_ptr<gimp_controller::GIMPController> gimpController =
      factory.create_gimp_controller();

  assert(gimpController != nullptr);

  if (gimpController == nullptr) {
    LOGE("Fail to create the GIMP controller instance");
    return INVALID;
  }

  // The platform specifics and the signal handlers come before GLib starts.
  gimpController->init_platform(ctx->get_argc(), ctx->get_argv());

  const std::string backtraceFile = gimpController->init_signals();

  ctx->adopt_argv(gimpController->init_process(ctx->get_argv()));

  if (ctx->get_be_verbose()) {
    gimpController->set_be_verbose(true);
  }

  if (ctx->get_no_interface()) {
    gimpController->set_no_interface(true);
  }

  if (ctx->get_print_help()) {
    gimpController->attach_console();
  }

  return perform(*gimpController, ctx, backtraceFile);
}

int Application::perform(gimp_controller::GIMPController& gimpController,
                         std::shared_ptr<ApplicationContext> ctx,
                         const std::string& backtraceFile)
{
  // The option context lives until GIMP returns, as in the main() of GIMP.
  const std::unique_ptr<GOptionContext, decltype(&g_option_context_free)>
      optionContext{gimpController.create_option_context(),
                    &g_option_context_free};
  GError* error = nullptr;

  if (!g_option_context_parse_strv(optionContext.get(), &ctx->get_argv(),
                                   &error)) {
    gimpController.report_parse_error(error);
    ctx->push_error("Fail to parse the command line");
    return INVALID;
  }

  return gimpController.run(ctx->get_argv()[0], backtraceFile);
}

}  // namespace app
