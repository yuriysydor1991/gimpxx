#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdlib>
#include <memory>

#include "src/app/applications/ApplicationVersionPrinter.h"
#include "src/gimp-controller/GIMPController.h"

using namespace app;
using namespace gimp_controller;
using namespace testing;

class UTEST_ApplicationVersionPrinter : public Test
{
 public:
  UTEST_ApplicationVersionPrinter()
      : app{std::make_shared<ApplicationVersionPrinter>()},
        appCtx{std::make_shared<ApplicationContext>(argc, argv)}
  {
  }

  ~UTEST_ApplicationVersionPrinter() override
  {
    GIMPController::onMockCreate = nullptr;
  }

  /// @brief Expects the version output of the single GIMP controller the run
  /// starts, the verbose output option first if the given one is set.
  void expect_the_version(const bool beVerbose)
  {
    EXPECT_CALL(controllerCreationEnsurer, Call(_))
        .WillOnce([beVerbose](GIMPController& instance) {
          EXPECT_CALL(instance, set_be_verbose(_)).Times(beVerbose ? 1 : 0);
          EXPECT_CALL(instance, create_option_context()).Times(0);
          EXPECT_CALL(instance, run(_, _)).Times(0);

          InSequence sequence;

          EXPECT_CALL(instance, init_platform(_, _));
          EXPECT_CALL(instance, init_signals());
          EXPECT_CALL(instance, init_process(_));
          EXPECT_CALL(instance, show_version());
        });

    GIMPController::onMockCreate = controllerCreationEnsurer.AsStdFunction();
  }

  MockFunction<void(GIMPController& instance)> controllerCreationEnsurer;

  char binaryName[11]{"binaryName"};
  char* args[2]{binaryName, nullptr};

  int argc{1};
  char** argv{args};

  std::shared_ptr<ApplicationVersionPrinter> app;
  std::shared_ptr<ApplicationContext> appCtx;
};

TEST_F(UTEST_ApplicationVersionPrinter, no_context_error)
{
  EXPECT_NE(app->run({}), 0);
}

TEST_F(UTEST_ApplicationVersionPrinter, the_version_is_shown_instead_of_gimp)
{
  expect_the_version(false);
  EXPECT_CALL(*appCtx, push_error(_)).Times(0);

  EXPECT_EQ(app->run(appCtx), EXIT_SUCCESS);

  EXPECT_TRUE(appCtx->get_errors().empty());
  EXPECT_FALSE(appCtx->get_stop());
}

TEST_F(UTEST_ApplicationVersionPrinter, the_verbose_version)
{
  appCtx->set_be_verbose(true);
  expect_the_version(true);

  EXPECT_EQ(app->run(appCtx), EXIT_SUCCESS);
}
