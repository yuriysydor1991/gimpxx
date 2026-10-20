#ifndef GIMPXX_PROJECT_GIMPCONTROLLERHOOKSMOCK_CLASS_H
#define GIMPXX_PROJECT_GIMPCONTROLLERHOOKSMOCK_CLASS_H

#include <glib.h>
#include <gmock/gmock.h>

#include "src/gimp-controller/GIMPController.h"

namespace gimp_controller
{

/**
 * @brief The GIMPController with the mocks of its platform methods, so the
 * test cases see when the controller calls them among the GIMP C functions.
 * The methods with a GIMPController implementation run it by default.
 */
class GIMPControllerHooksMock : public GIMPController
{
 public:
  GIMPControllerHooksMock()
  {
    ON_CALL(*this, copy_arguments(testing::_))
        .WillByDefault([this](char** argv) {
          return GIMPController::copy_arguments(argv);
        });
    ON_CALL(*this, open_in_running_instance(testing::_, testing::_))
        .WillByDefault([this](const gchar** filenames, const gboolean asNew) {
          return GIMPController::open_in_running_instance(filenames, asNew);
        });
    ON_CALL(*this, has_shared_memory()).WillByDefault([this]() {
      return GIMPController::has_shared_memory();
    });
  }

  MOCK_METHOD(void, attach_console, (), (override));
  MOCK_METHOD(void, fix_arguments, (int& argc, char**& argv), (override));
  MOCK_METHOD(void, init_environment, (const gchar* progName), (override));
  MOCK_METHOD(void, prepare_process, (), (override));
  MOCK_METHOD(void, init_locale, (), (override));
  MOCK_METHOD(void, apply_early_configuration, (GimpEarlyRc * earlyrc),
              (override));
  MOCK_METHOD(char**, copy_arguments, (char** argv), (override));
  MOCK_METHOD(gboolean, open_in_running_instance,
              (const gchar** filenames, const gboolean asNew), (override));
  MOCK_METHOD(gboolean, has_shared_memory, (), (override));
};

}  // namespace gimp_controller

#endif  // GIMPXX_PROJECT_GIMPCONTROLLERHOOKSMOCK_CLASS_H
