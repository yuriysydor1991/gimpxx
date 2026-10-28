#ifndef GIMPXX_PROJECT_APPLICATION_CLASS_H
#define GIMPXX_PROJECT_APPLICATION_CLASS_H

#include <memory>
#include <string>

#include "src/app/ApplicationContext.h"
#include "src/app/applications/IApplication.h"

namespace gimp_controller
{
class GIMPController;
}

namespace app
{

/**
 * @brief The GIMP application: it creates the GIMPController of the current
 * platform and starts GIMP through it in the order the main() of GIMP did: the
 * platform, the signal handlers and the process first, the options the
 * CommandLineParser found next, then the GIMP option parsing and GIMP itself,
 * here or through the already running GIMP instance.
 */
class Application : public IApplication
{
 public:
  ~Application() override = default;
  Application() = default;

  /**
   * @brief Creates the single GIMPController of the run through the
   * GIMPControllerFactory, starts the platform, the GIMP signal handlers and
   * the process with it and gives it the options the context holds. See the
   * Application::perform method for the rest.
   *
   * @param ctx Application's run context with command line parameters etc.
   *
   * @return Returns the application return status. A zero value on the success
   * and other value otherwise.
   */
  int run(std::shared_ptr<ApplicationContext> ctx) override;

 protected:
  /**
   * @brief Parses the command line with the GIMP option context and runs GIMP
   * with the program name and the given backtrace file. See the
   * gimp_controller::GIMPController::run method.
   *
   * @param gimpController The controller the Application::run started.
   * @param ctx Application's run context with command line parameters etc.
   * @param backtraceFile The file the GIMP signal handlers write the
   * backtraces into, empty if there is none.
   *
   * @return Returns the GIMP exit status, IApplication::INVALID once the GIMP
   * option parsing fails.
   */
  virtual int perform(gimp_controller::GIMPController& gimpController,
                      std::shared_ptr<ApplicationContext> ctx,
                      const std::string& backtraceFile);
};

}  // namespace app

#endif  // GIMPXX_PROJECT_APPLICATION_CLASS_H
