#include <gmock/gmock.h>
#include <gtest/gtest.h>

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
        appCtx{std::make_shared<ApplicationContext>(argc, argv, gimpController)}
  {
  }

  int argc{0};
  char** argv{nullptr};

  std::shared_ptr<StrictMock<GIMPController>> gimpController{
      std::make_shared<StrictMock<GIMPController>>()};

  std::shared_ptr<ApplicationVersionPrinter> app;
  std::shared_ptr<ApplicationContext> appCtx;
};

TEST_F(UTEST_ApplicationVersionPrinter, no_context_error)
{
  EXPECT_NE(app->run({}), 0);
}

TEST_F(UTEST_ApplicationVersionPrinter, normal_exit)
{
  EXPECT_CALL(*gimpController, show_version()).Times(1);
  EXPECT_CALL(*appCtx, push_error(_)).Times(0);

  EXPECT_EQ(app->run(appCtx), 0);

  EXPECT_TRUE(appCtx->get_errors().empty());
  EXPECT_FALSE(appCtx->get_stop());
}
