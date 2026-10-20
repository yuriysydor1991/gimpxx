#ifndef GIMPXX_PROJECT_APPLICATION_CLASS_H
#define GIMPXX_PROJECT_APPLICATION_CLASS_H

#include <memory>

#include "src/app/ApplicationContext.h"
#include "src/app/applications/IApplication.h"

namespace app
{

/**
 * @brief The GIMP application: it runs GIMP through the GIMPController of the
 * context once the command line is parsed, here or through the already running
 * GIMP instance.
 */
class Application : public IApplication
{
 public:
  ~Application() override = default;
  Application() = default;

  /**
   * @brief Runs GIMP with the program name and the backtrace file of the
   * context. See the gimp_controller::GIMPController::run method.
   *
   * @param ctx Application's run context with command line parameters etc.
   *
   * @return Returns the application return status. A zero value on the success
   * and other value otherwise.
   */
  virtual int run(std::shared_ptr<ApplicationContext> ctx) override;
};

}  // namespace app

#endif  // GIMPXX_PROJECT_APPLICATION_CLASS_H
