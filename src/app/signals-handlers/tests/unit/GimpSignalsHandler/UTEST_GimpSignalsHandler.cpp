#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <memory>

#include "src/app/signals-handlers/GimpSignalsHandler.h"

using namespace app;
using namespace testing;

class UTEST_GimpSignalsHandler : public Test
{
 public:
  UTEST_GimpSignalsHandler()
      : handler{std::make_shared<GimpSignalsHandler>()},
        appCtx{std::make_shared<ApplicationContext>(argc, argv)}
  {
  }

  int argc{0};
  char** argv{nullptr};

  std::shared_ptr<GimpSignalsHandler> handler;
  std::shared_ptr<ApplicationContext> appCtx;
};

TEST_F(UTEST_GimpSignalsHandler, the_handler_is_a_signals_handler)
{
  EXPECT_NE(dynamic_cast<ISignalsHandler*>(handler.get()), nullptr);
}

TEST_F(UTEST_GimpSignalsHandler, no_context_error)
{
  EXPECT_FALSE(handler->install({}));
}

TEST_F(UTEST_GimpSignalsHandler, the_signals_are_left_to_gimp)
{
  EXPECT_CALL(*appCtx, push_error(_)).Times(0);

  EXPECT_TRUE(handler->install(appCtx));

  EXPECT_FALSE(appCtx->get_stop());
}

TEST_F(UTEST_GimpSignalsHandler, uninstall_keeps_the_gimp_handlers)
{
  EXPECT_TRUE(handler->install(appCtx));

  handler->uninstall();
  handler->uninstall();

  EXPECT_FALSE(appCtx->get_stop());
}
