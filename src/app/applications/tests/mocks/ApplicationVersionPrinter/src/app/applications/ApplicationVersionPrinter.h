#ifndef GIMPXX_PROJECT_APPLICATIONVERSIONPRINTER_CLASS_H
#define GIMPXX_PROJECT_APPLICATIONVERSIONPRINTER_CLASS_H

#include <gmock/gmock.h>

#include <memory>

#include "src/app/applications/IApplication.h"

namespace app
{

class ApplicationVersionPrinter : public IApplication
{
 public:
  ~ApplicationVersionPrinter() override = default;

  ApplicationVersionPrinter()
  {
    if (onMockCreate) {
      onMockCreate(*this);
    }
  }

  inline static std::function<void(ApplicationVersionPrinter&)> onMockCreate;

  MOCK_METHOD(int, run, (std::shared_ptr<ApplicationContext> ctx), (override));
};

}  // namespace app

#endif  // GIMPXX_PROJECT_APPLICATIONVERSIONPRINTER_CLASS_H
