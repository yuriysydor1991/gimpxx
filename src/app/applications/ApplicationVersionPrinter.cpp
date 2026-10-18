#include "src/app/applications/ApplicationVersionPrinter.h"

#include <cassert>
#include <cstdlib>
#include <memory>

#include "src/gimp/app/main.h"
#include "src/log/log.h"

namespace app
{

int ApplicationVersionPrinter::run(std::shared_ptr<ApplicationContext> ctx)
{
  assert(ctx != nullptr);

  if (ctx == nullptr) {
    LOGE("No valid application context pointer provided");
    return INVALID;
  }

  gimp_main_show_version();

  return EXIT_SUCCESS;
}

}  // namespace app
