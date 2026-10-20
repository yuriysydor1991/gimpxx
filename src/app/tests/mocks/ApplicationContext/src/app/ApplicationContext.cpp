#include "ApplicationContext.h"

#include <memory>
#include <utility>

namespace app
{

ApplicationContext::ApplicationContext(
    int& gargc, char**& gargv,
    std::shared_ptr<gimp_controller::GIMPController> gimpController)
    : margc{gargc}, margv{gargv}, mgimp_controller{std::move(gimpController)}
{
  // The pushed errors and the adopted option context are kept like the real
  // context keeps them, so the test cases see the errors through the
  // get_errors accessor too.
  ON_CALL(*this, push_error(testing::_))
      .WillByDefault([this](const std::string& errorDescription) {
        merrors.push_back(errorDescription);
      });
  ON_CALL(*this, adopt_option_context(testing::_))
      .WillByDefault([this](GOptionContext* optionContext) {
        moption_context.reset(optionContext);
      });
}

}  // namespace app
