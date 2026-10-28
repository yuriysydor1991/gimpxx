#ifndef GIMPXX_PROJECT_COMMANDLINEPARSER_CLASS_H
#define GIMPXX_PROJECT_COMMANDLINEPARSER_CLASS_H

#include <memory>

#include "src/app/ApplicationContext.h"

namespace app
{

/**
 * @brief The command line parser which scans the arguments for the few options
 * GIMP needs before its option parsing, the same way the main() of GIMP did,
 * and keeps them in the ApplicationContext. The Application parses all of the
 * options with the GIMP option context later.
 */
class CommandLineParser
{
 public:
  virtual ~CommandLineParser() = default;
  CommandLineParser() = default;

  /**
   * @brief Scans the arguments of the given context for the verbose and the no
   * interface options, which must be known before GTK starts, for the version
   * ones, which end the scan, and for the help ones.
   *
   * @param ctx The application context with the arguments to scan.
   *
   * @return Returns false if no valid context is given.
   */
  virtual bool parse_args(std::shared_ptr<ApplicationContext> ctx);
};

}  // namespace app

#endif  // GIMPXX_PROJECT_COMMANDLINEPARSER_CLASS_H
