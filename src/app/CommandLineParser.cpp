#include "src/app/CommandLineParser.h"

#include <cassert>
#include <memory>
#include <string>

#include "src/app/ApplicationContext.h"
#include "src/app/CMDParamNames.h"
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

  char** const argv = ctx->get_argv();

  // The whole command line is scanned for the verbose option first, since
  // the version is printed with it.
  for (int iter = 1; iter < ctx->get_argc(); ++iter) {
    const std::string param = argv[iter];

    if (param == CMDParamNames::VERBOSEW || param == CMDParamNames::VERSION) {
      ctx->set_be_verbose(true);
    }
  }

  for (int iter = 1; iter < ctx->get_argc(); ++iter) {
    const std::string param = argv[iter];

    if (param == CMDParamNames::NO_INTERFACEW ||
        param == CMDParamNames::NO_INTERFACE) {
      ctx->set_no_interface(true);
    } else if (param == CMDParamNames::VERSIONW ||
               param == CMDParamNames::VERSION) {
      ctx->set_print_version_and_exit(true);
      break;
    } else if (param == CMDParamNames::HELPW || param == CMDParamNames::HELP ||
               param.rfind(CMDParamNames::HELP_GROUP_PREFIX, 0) == 0) {
      ctx->set_print_help(true);
    }
  }

  return true;
}

}  // namespace app
