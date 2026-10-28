#include "src/app/applications/ApplicationVersionPrinter.h"

#include <cstdlib>
#include <memory>
#include <string>

#include "src/gimp-controller/GIMPController.h"

namespace app
{

int ApplicationVersionPrinter::perform(
    gimp_controller::GIMPController& gimpController,
    std::shared_ptr<ApplicationContext>, const std::string&)
{
  gimpController.show_version();

  return EXIT_SUCCESS;
}

}  // namespace app
