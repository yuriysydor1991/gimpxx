#ifndef GIMPXX_PROJECT_GIMPSIGNALSHANDLER_CLASS_H
#define GIMPXX_PROJECT_GIMPSIGNALSHANDLER_CLASS_H

#include <memory>

#include "src/app/ApplicationContext.h"
#include "src/app/signals-handlers/ISignalsHandler.h"

namespace app
{

/**
 * @brief The signals handler of GIMP: it installs the GIMP signal handlers,
 * which write the backtraces of the fatal signals and quit GIMP on the
 * terminating ones. The SignalsHandler component takes their semantics over
 * later.
 */
class GimpSignalsHandler : public ISignalsHandler
{
 public:
  ~GimpSignalsHandler() override = default;
  GimpSignalsHandler() = default;

  /**
   * @brief Initializes the GIMP backtraces and installs the GIMP signal
   * handlers through the GIMPController of the context, which keeps the
   * backtrace file they chose.
   *
   * @param ctx The application context.
   *
   * @return Returns false if no valid context is given.
   */
  bool install(std::shared_ptr<ApplicationContext> ctx) override;

  /// @brief Does nothing: the GIMP signal handlers stay for the whole run.
  void uninstall() override;
};

}  // namespace app

#endif  // GIMPXX_PROJECT_GIMPSIGNALSHANDLER_CLASS_H
