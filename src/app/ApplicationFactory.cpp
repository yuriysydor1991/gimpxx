#include "src/app/ApplicationFactory.h"

#include <cassert>
#include <memory>
#include <string>

#include "src/app/ApplicationContext.h"
#include "src/app/CommandLineParser.h"
#include "src/app/applications/Application.h"
#include "src/app/applications/ApplicationVersionPrinter.h"
#include "src/app/applications/IApplication.h"
#include "src/app/signals-handlers/GimpSignalsHandler.h"
#include "src/app/signals-handlers/ISignalsHandler.h"
#include "src/gimp/app/main.h"
#include "src/log/log.h"

namespace app
{

std::shared_ptr<ApplicationContext> ApplicationFactory::create_default_context(
    int& gargc, char**& gargv)
{
  return std::make_shared<ApplicationContext>(gargc, gargv,
                                              *gimp_main_options());
}

std::shared_ptr<CommandLineParser>
ApplicationFactory::create_default_arg_parser()
{
  return std::make_shared<CommandLineParser>();
}

std::shared_ptr<ISignalsHandler>
ApplicationFactory::create_default_signals_handler()
{
  return std::make_shared<GimpSignalsHandler>();
}

std::shared_ptr<IApplication> ApplicationFactory::create_default_application()
{
  return std::make_shared<Application>();
}

std::shared_ptr<IApplication> ApplicationFactory::create_version_printer()
{
  return std::make_shared<ApplicationVersionPrinter>();
}

std::shared_ptr<IApplication> ApplicationFactory::create_application(
    std::shared_ptr<ApplicationContext> ctx)
{
  assert(ctx != nullptr);

  if (ctx == nullptr) {
    LOGE("No valid application context pointer provided");
    return {};
  }

  if (ctx->get_print_version_and_exit()) {
    LOGD("Creating the version printer application");
    return create_version_printer();
  }

  return create_default_application();
}

bool ApplicationFactory::init_logging()
{
  return LOG_INIT(std::string{}, default_logger::DefaultLogger::LVL_WARNING,
                  true);
}

int ApplicationFactory::run(int& gargc, char**& gargv)
{
  if (!init_logging()) {
    return IApplication::INVALID;
  }

  std::shared_ptr<ApplicationContext> ctx =
      create_default_context(gargc, gargv);

  assert(ctx != nullptr);

  if (ctx == nullptr) {
    LOGE("Fail to create application context instance");
    return IApplication::INVALID;
  }

  // The platform specifics and the signal handlers come before GLib starts.
  gimp_main_init_platform(&ctx->get_argc(), &ctx->get_argv());

  std::shared_ptr<ISignalsHandler> signalsHandler =
      create_default_signals_handler();

  assert(signalsHandler != nullptr);

  if (signalsHandler == nullptr) {
    LOGE("Fail to create the signals handler object");
    return IApplication::INVALID;
  }

  if (!signalsHandler->install(ctx)) {
    LOGW("Fail to handle the application OS signals");
  }

  ctx->adopt_argv(gimp_main_init_process(ctx->get_argv()));

  std::shared_ptr<CommandLineParser> argParser = create_default_arg_parser();

  assert(argParser != nullptr);

  if (argParser == nullptr) {
    LOGE("Fail to create command line arguments parser instance");
    return IApplication::INVALID;
  }

  if (!argParser->parse_args(ctx)) {
    return IApplication::INVALID;
  }

  std::shared_ptr<IApplication> app = create_application(ctx);

  assert(app != nullptr);

  if (app == nullptr) {
    LOGE("Fail to create application instance");
    return IApplication::INVALID;
  }

  LOGD("Starting the application");

  return app->run(ctx);
}

int ApplicationFactory::execute(int& gargc, char**& gargv)
{
  std::shared_ptr<ApplicationFactory> factory =
      std::make_shared<ApplicationFactory>();

  return factory->run(gargc, gargv);
}

}  // namespace app
