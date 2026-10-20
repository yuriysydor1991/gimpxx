#include "src/app/CommandLineParser.h"

#include <cassert>
#include <memory>
#include <string>

#include "src/app/ApplicationContext.h"
#include "src/app/CMDParamNames.h"
#include "src/gimp-controller/GIMPController.h"
#include "src/log/log.h"

namespace app
{

bool CommandLineParser::parse_args(std::shared_ptr<ApplicationContext> ctx)
{
  assert(ctx != nullptr);

  if (ctx == nullptr) {
    LOGE("No valid application context pointer provided");
    return false;
  }

  prescan_args(ctx);

  if (ctx->get_print_version_and_exit()) {
    return true;
  }

  return parse_options(ctx);
}

void CommandLineParser::prescan_args(std::shared_ptr<ApplicationContext> ctx)
{
  gimp_controller::GIMPController& gimpController = ctx->get_gimp_controller();
  char** const argv = ctx->get_argv();

  // The whole command line is scanned for the verbose option first, since
  // the version is printed with it.
  for (int iter = 1; iter < ctx->get_argc(); ++iter) {
    const std::string param = argv[iter];

    if (param == CMDParamNames::VERBOSEW || param == CMDParamNames::VERSION) {
      gimpController.set_be_verbose(true);
    }
  }

  for (int iter = 1; iter < ctx->get_argc(); ++iter) {
    const std::string param = argv[iter];

    if (param == CMDParamNames::NO_INTERFACEW ||
        param == CMDParamNames::NO_INTERFACE) {
      gimpController.set_no_interface(true);
    } else if (param == CMDParamNames::VERSIONW ||
               param == CMDParamNames::VERSION) {
      ctx->set_print_version_and_exit(true);
      return;
    } else if (param == CMDParamNames::HELPW || param == CMDParamNames::HELP ||
               param.rfind(CMDParamNames::HELP_GROUP_PREFIX, 0) == 0) {
      gimpController.attach_console();
    }
  }
}

bool CommandLineParser::parse_options(std::shared_ptr<ApplicationContext> ctx)
{
  gimp_controller::GIMPController& gimpController = ctx->get_gimp_controller();
  GOptionContext* const optionContext = gimpController.create_option_context();
  GError* error = nullptr;

  ctx->adopt_option_context(optionContext);

  if (g_option_context_parse_strv(optionContext, &ctx->get_argv(), &error)) {
    return true;
  }

  gimpController.report_parse_error(error);
  ctx->push_error("Fail to parse the command line");

  return false;
}

}  // namespace app
