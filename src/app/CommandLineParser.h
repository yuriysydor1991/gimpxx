#ifndef YOUR_CPP_APP_TEMPLATE_PROJECT_COMMANDLINEPARSER_CLASS_H
#define YOUR_CPP_APP_TEMPLATE_PROJECT_COMMANDLINEPARSER_CLASS_H

#include <memory>

#include "src/app/ApplicationContext.h"

namespace app
{

/**
 * @brief The command line parser which fills the GIMP start-up options of the
 * ApplicationContext.
 *
 * It scans the arguments for the few options GIMP needs before its option
 * parsing first, then parses them with the GIMP option context, the same way
 * the main() of GIMP did.
 */
class CommandLineParser
{
 public:
  virtual ~CommandLineParser() = default;
  CommandLineParser() = default;

  /**
   * @brief Parses the command line of the given context. The GIMP option
   * parsing is skipped once the version is to be printed.
   *
   * @param ctx The application context with the arguments to parse.
   *
   * @return Returns false if the GIMP option parsing fails. The failure is
   * printed and pushed into the context errors then.
   */
  virtual bool parse_args(std::shared_ptr<ApplicationContext> ctx);

 protected:
  /**
   * @brief Scans the arguments for the verbose and the no interface options,
   * which must be known before GTK starts, and for the version ones. The help
   * ones attach the console window on Windows.
   *
   * @param ctx The application context with the arguments to scan.
   */
  virtual void prescan_args(std::shared_ptr<ApplicationContext> ctx);

  /**
   * @brief Parses the arguments with the GIMP option context, which the
   * application context keeps until the end of the run.
   *
   * @param ctx The application context with the arguments to parse.
   *
   * @return Returns false if the parsing fails.
   */
  virtual bool parse_options(std::shared_ptr<ApplicationContext> ctx);
};

}  // namespace app

#endif  // YOUR_CPP_APP_TEMPLATE_PROJECT_COMMANDLINEPARSER_CLASS_H
