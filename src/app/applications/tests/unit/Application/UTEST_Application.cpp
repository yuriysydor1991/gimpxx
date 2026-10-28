#include <glib.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdlib>
#include <functional>
#include <memory>
#include <string>
#include <utility>

#include "src/app/applications/Application.h"
#include "src/gimp-controller/GIMPController.h"

using namespace app;
using namespace gimp_controller;
using namespace testing;

class UTEST_Application : public Test
{
 public:
  inline static constexpr const int expectedStatus{7};
  inline static const std::string expectedError{
      "Fail to parse the command line"};

  UTEST_Application()
      : app{std::make_shared<Application>()},
        appCtx{std::make_shared<ApplicationContext>(argc, argv)}
  {
  }

  ~UTEST_Application() override { GIMPController::onMockCreate = nullptr; }

  /// @brief Expects the run to create a single GIMP controller, which gets
  /// the given expectations.
  void expect_the_gimp_controller(
      std::function<void(GIMPController& instance)> expectations)
  {
    EXPECT_CALL(controllerCreationEnsurer, Call(_))
        .WillOnce(std::move(expectations));

    GIMPController::onMockCreate = controllerCreationEnsurer.AsStdFunction();
  }

  /// @brief Expects the platform, the signal handlers and the process start.
  static void expect_the_start(GIMPController& instance)
  {
    EXPECT_CALL(instance, init_platform(_, _));
    EXPECT_CALL(instance, init_signals());
    EXPECT_CALL(instance, init_process(_));
  }

  MockFunction<void(GIMPController& instance)> controllerCreationEnsurer;

  char binaryName[11]{"binaryName"};
  char* args[2]{binaryName, nullptr};

  int argc{1};
  char** argv{args};

  std::shared_ptr<Application> app;
  std::shared_ptr<ApplicationContext> appCtx;
};

TEST_F(UTEST_Application, no_context_error)
{
  EXPECT_CALL(controllerCreationEnsurer, Call(_)).Times(0);
  GIMPController::onMockCreate = controllerCreationEnsurer.AsStdFunction();

  EXPECT_NE(app->run({}), 0);
}

TEST_F(UTEST_Application, run_starts_gimp_in_order)
{
  gchar** const argvCopy = g_strsplit("adoptedName image.xcf", " ", -1);

  expect_the_gimp_controller([this, argvCopy](GIMPController& instance) {
    InSequence sequence;

    EXPECT_CALL(instance, init_platform(Ref(argc), Ref(argv)));
    EXPECT_CALL(instance, init_signals()).WillOnce(Return("backtrace.txt"));
    EXPECT_CALL(instance, init_process(args)).WillOnce(Return(argvCopy));
    EXPECT_CALL(instance, create_option_context());
    EXPECT_CALL(instance,
                run(StrEq("adoptedName"), std::string{"backtrace.txt"}))
        .WillOnce(Return(expectedStatus));
  });

  EXPECT_EQ(app->run(appCtx), expectedStatus);

  // The context adopted the copy, which the option parsing and GIMP got.
  EXPECT_EQ(argv, argvCopy);
}

TEST_F(UTEST_Application, the_command_line_options_reach_the_controller)
{
  appCtx->set_be_verbose(true);
  appCtx->set_no_interface(true);
  appCtx->set_print_help(true);

  expect_the_gimp_controller([](GIMPController& instance) {
    InSequence sequence;

    expect_the_start(instance);
    EXPECT_CALL(instance, set_be_verbose(true));
    EXPECT_CALL(instance, set_no_interface(true));
    EXPECT_CALL(instance, attach_console());
    EXPECT_CALL(instance, create_option_context());
    EXPECT_CALL(instance, run(_, _)).WillOnce(Return(EXIT_SUCCESS));
  });

  EXPECT_EQ(app->run(appCtx), EXIT_SUCCESS);
}

TEST_F(UTEST_Application, the_controller_keeps_its_defaults_without_options)
{
  expect_the_gimp_controller([](GIMPController& instance) {
    expect_the_start(instance);
    EXPECT_CALL(instance, set_be_verbose(_)).Times(0);
    EXPECT_CALL(instance, set_no_interface(_)).Times(0);
    EXPECT_CALL(instance, attach_console()).Times(0);
    EXPECT_CALL(instance, create_option_context());
    EXPECT_CALL(instance, run(_, _)).WillOnce(Return(EXIT_SUCCESS));
  });

  EXPECT_EQ(app->run(appCtx), EXIT_SUCCESS);
}

TEST_F(UTEST_Application, the_failed_option_parsing_skips_the_gimp_run)
{
  appCtx->adopt_argv(g_strsplit("binaryName --unknown", " ", -1));

  expect_the_gimp_controller([](GIMPController& instance) {
    expect_the_start(instance);
    EXPECT_CALL(instance, create_option_context()).WillOnce([]() {
      return g_option_context_new(nullptr);
    });
    EXPECT_CALL(instance, report_parse_error(Field(&GError::message,
                                                   HasSubstr("--unknown"))))
        .WillOnce([](GError* error) { g_error_free(error); });
    EXPECT_CALL(instance, run(_, _)).Times(0);
  });
  EXPECT_CALL(*appCtx, push_error(expectedError)).Times(1);

  EXPECT_EQ(app->run(appCtx), IApplication::INVALID);

  EXPECT_THAT(appCtx->get_errors(), ElementsAre(expectedError));
}
