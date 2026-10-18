#include "src/app/CommandLineParser.h"

#include <cassert>
#include <memory>
#include <string>

#include "src/app/ApplicationContext.h"
#include "src/app/CMDParamNames.h"
#include "src/gimp/app/main.h"
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
  GimpMainOptions& options = ctx->get_options();
  char** const argv = ctx->get_argv();

  // The whole command line is scanned for the verbose option first, since
  // the version is printed with it.
  for (int iter = 1; iter < ctx->get_argc(); ++iter) {
    const std::string param = argv[iter];

    if (param == CMDParamNames::VERBOSEW || param == CMDParamNames::VERSION) {
      options.be_verbose = TRUE;
    }
  }

  for (int iter = 1; iter < ctx->get_argc(); ++iter) {
    const std::string param = argv[iter];

    if (param == CMDParamNames::NO_INTERFACEW ||
        param == CMDParamNames::NO_INTERFACE) {
      options.no_interface = TRUE;
    } else if (param == CMDParamNames::VERSIONW ||
               param == CMDParamNames::VERSION) {
      ctx->set_print_version_and_exit(true);
      return;
    } else if (param == CMDParamNames::HELPW || param == CMDParamNames::HELP ||
               param.rfind(CMDParamNames::HELP_GROUP_PREFIX, 0) == 0) {
      gimp_main_attach_console();
    }
  }
}

bool CommandLineParser::parse_options(std::shared_ptr<ApplicationContext> ctx)
{
  GOptionContext* const optionContext = gimp_main_option_context_new();
  GError* error = nullptr;

  ctx->adopt_option_context(optionContext);

  if (g_option_context_parse_strv(optionContext, &ctx->get_argv(), &error)) {
    return true;
  }

  gimp_main_report_parse_error(error);
  ctx->push_error("Fail to parse the command line");

  return false;
}

}  // namespace app
