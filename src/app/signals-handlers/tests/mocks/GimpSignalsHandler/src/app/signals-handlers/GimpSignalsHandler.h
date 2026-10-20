#ifndef GIMPXX_PROJECT_GIMPSIGNALSHANDLER_CLASS_H
#define GIMPXX_PROJECT_GIMPSIGNALSHANDLER_CLASS_H

#include <gmock/gmock.h>

#include <functional>
#include <memory>

#include "src/app/ApplicationContext.h"
#include "src/app/signals-handlers/ISignalsHandler.h"

namespace app
{

class GimpSignalsHandler : public ISignalsHandler
{
 public:
  ~GimpSignalsHandler() override = default;

  GimpSignalsHandler()
  {
    if (onMockCreate) {
      onMockCreate(*this);
    }
  }

  inline static std::function<void(GimpSignalsHandler&)> onMockCreate;

  MOCK_METHOD(bool, install, (std::shared_ptr<ApplicationContext> ctx),
              (override));
  MOCK_METHOD(void, uninstall, (), (override));
};

}  // namespace app

#endif  // GIMPXX_PROJECT_GIMPSIGNALSHANDLER_CLASS_H
