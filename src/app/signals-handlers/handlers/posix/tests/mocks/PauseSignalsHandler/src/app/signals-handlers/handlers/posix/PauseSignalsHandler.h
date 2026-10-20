#ifndef GIMPXX_PROJECT_PAUSESIGNALSHANDLER_CLASS_H
#define GIMPXX_PROJECT_PAUSESIGNALSHANDLER_CLASS_H

#include <gmock/gmock.h>

#include <functional>
#include <memory>

#include "src/app/ApplicationContext.h"
#include "src/app/signals-handlers/ISignalsHandler.h"

namespace app
{

class PauseSignalsHandler : public ISignalsHandler
{
 public:
  ~PauseSignalsHandler() override = default;

  PauseSignalsHandler()
  {
    if (onMockCreate) {
      onMockCreate(*this);
    }
  }

  inline static std::function<void(PauseSignalsHandler&)> onMockCreate;

  MOCK_METHOD(bool, install, (std::shared_ptr<ApplicationContext> ctx),
              (override));
  MOCK_METHOD(void, uninstall, (), (override));
};

}  // namespace app

#endif  // GIMPXX_PROJECT_PAUSESIGNALSHANDLER_CLASS_H
