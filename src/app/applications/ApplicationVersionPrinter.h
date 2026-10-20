#ifndef GIMPXX_PROJECT_APPLICATIONVERSIONPRINTER_CLASS_H
#define GIMPXX_PROJECT_APPLICATIONVERSIONPRINTER_CLASS_H

#include <memory>

#include "src/app/ApplicationContext.h"
#include "src/app/applications/IApplication.h"

namespace app
{

/**
 * @brief A version message printer application class. It prints the GIMP
 * version, the verbose one once the verbose output is asked for.
 */
class ApplicationVersionPrinter : public IApplication
{
 public:
  ~ApplicationVersionPrinter() override = default;
  ApplicationVersionPrinter() = default;

  /**
   * @brief Implemented default application. See IApplication interface.
   *
   * @param ctx Application's run context with command line parameters etc.
   *
   * @return Returns the application return status. A zero value on the success
   * and other value otherwise.
   */
  virtual int run(std::shared_ptr<ApplicationContext> ctx) override;
};

}  // namespace app

#endif  // GIMPXX_PROJECT_APPLICATIONVERSIONPRINTER_CLASS_H
