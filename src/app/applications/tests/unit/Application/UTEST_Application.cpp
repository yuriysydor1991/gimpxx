#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdlib>
#include <memory>
#include <string>

#include "src/app/applications/Application.h"
#include "src/gimp-controller/GIMPController.h"

using namespace app;
using namespace gimp_controller;
using namespace testing;

class UTEST_Application : public Test
{
 public:
  inline static constexpr const int expectedStatus{7};

  UTEST_Application()
      : app{std::make_shared<Application>()},
        appCtx{std::make_shared<ApplicationContext>(argc, argv, gimpController)}
  {
  }

  char binaryName[11]{"binaryName"};
  char* args[2]{binaryName, nullptr};

  int argc{1};
  char** argv{args};

  std::shared_ptr<StrictMock<GIMPController>> gimpController{
      std::make_shared<StrictMock<GIMPController>>()};

  std::shared_ptr<Application> app;
  std::shared_ptr<ApplicationContext> appCtx;
};

TEST_F(UTEST_Application, no_context_error) { EXPECT_NE(app->run({}), 0); }

TEST_F(UTEST_Application, the_controller_runs_gimp)
{
  EXPECT_CALL(*gimpController, run(StrEq("binaryName"), IsEmpty()))
      .WillOnce(Return(expectedStatus));

  EXPECT_EQ(app->run(appCtx), expectedStatus);
}

TEST_F(UTEST_Application, the_controller_gets_the_backtrace_file)
{
  appCtx->set_backtrace_file("backtrace.txt");

  EXPECT_CALL(*gimpController,
              run(StrEq("binaryName"), std::string{"backtrace.txt"}))
      .WillOnce(Return(expectedStatus));

  EXPECT_EQ(app->run(appCtx), expectedStatus);
}

TEST_F(UTEST_Application, the_controller_gets_the_adopted_arguments)
{
  appCtx->adopt_argv(g_strsplit("adoptedName image.xcf", " ", -1));

  EXPECT_CALL(*gimpController, run(StrEq("adoptedName"), _))
      .WillOnce(Return(EXIT_SUCCESS));

  EXPECT_EQ(app->run(appCtx), EXIT_SUCCESS);
}
