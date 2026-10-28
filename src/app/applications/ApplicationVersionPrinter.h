#ifndef GIMPXX_PROJECT_APPLICATIONVERSIONPRINTER_CLASS_H
#define GIMPXX_PROJECT_APPLICATIONVERSIONPRINTER_CLASS_H

#include <memory>
#include <string>

#include "src/app/ApplicationContext.h"
#include "src/app/applications/Application.h"

namespace app
{

/**
 * @brief A version message printer application class. It starts GIMP the way
 * the Application does, then prints the GIMP version instead of running GIMP,
 * the verbose one once the verbose output is asked for.
 */
class ApplicationVersionPrinter : public Application
{
 public:
  ~ApplicationVersionPrinter() override = default;
  ApplicationVersionPrinter() = default;

 protected:
  /**
   * @brief Shows the GIMP version with the started controller.
   *
   * @return Returns the zero value.
   */
  int perform(gimp_controller::GIMPController& gimpController,
              std::shared_ptr<ApplicationContext> ctx,
              const std::string& backtraceFile) override;
};

}  // namespace app

#endif  // GIMPXX_PROJECT_APPLICATIONVERSIONPRINTER_CLASS_H
