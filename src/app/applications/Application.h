#ifndef YOUR_CPP_APP_TEMPLATE_PROJECT_APPLICATION_CLASS_H
#define YOUR_CPP_APP_TEMPLATE_PROJECT_APPLICATION_CLASS_H

#include <memory>

#include "src/app/ApplicationContext.h"
#include "src/app/applications/IApplication.h"

namespace app
{

/**
 * @brief The GIMP application: it runs GIMP once the command line is parsed,
 * here or through the already running GIMP instance.
 */
class Application : public IApplication
{
 public:
  ~Application() override = default;
  Application() = default;

  /**
   * @brief Runs GIMP the way the end of the main() of GIMP did: names the
   * process, attaches the console window on Windows when the output needs
   * it, passes the command line to the already running instance if there is
   * one, checks the sanity of the libraries and runs GIMP at last.
   *
   * @param ctx Application's run context with command line parameters etc.
   *
   * @return Returns the application return status. A zero value on the success
   * and other value otherwise.
   */
  virtual int run(std::shared_ptr<ApplicationContext> ctx) override;
};

}  // namespace app

#endif  // YOUR_CPP_APP_TEMPLATE_PROJECT_APPLICATION_CLASS_H
