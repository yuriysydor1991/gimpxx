#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <memory>

#include "src/app/signals-handlers/GimpSignalsHandler.h"
#include "src/gimp/app/main.h"

using namespace app;
using namespace testing;

class UTEST_GimpSignalsHandler : public Test
{
 public:
  UTEST_GimpSignalsHandler()
      : handler{std::make_shared<GimpSignalsHandler>()},
        appCtx{std::make_shared<ApplicationContext>(argc, argv, options)}
  {
    seam.reset();
  }

  ~UTEST_GimpSignalsHandler() override { seam.reset(); }

  gimpMainMock& seam{gimpMainMock::instance()};

  int argc{0};
  char** argv{nullptr};
  GimpMainOptions options{};

  std::shared_ptr<GimpSignalsHandler> handler;
  std::shared_ptr<ApplicationContext> appCtx;
};

TEST_F(UTEST_GimpSignalsHandler, the_handler_is_a_signals_handler)
{
  EXPECT_NE(dynamic_cast<ISignalsHandler*>(handler.get()), nullptr);
}

TEST_F(UTEST_GimpSignalsHandler, no_context_error)
{
  EXPECT_CALL(seam.gimp_main_init_signals, Call()).Times(0);

  EXPECT_FALSE(handler->install({}));
}

TEST_F(UTEST_GimpSignalsHandler, the_context_keeps_the_backtrace_file)
{
  // The handler frees the backtrace file name GIMP allocates.
  EXPECT_CALL(seam.gimp_main_init_signals, Call()).WillOnce([]() {
    return g_strdup("backtrace.txt");
  });

  EXPECT_TRUE(handler->install(appCtx));

  EXPECT_EQ(appCtx->get_backtrace_file(), "backtrace.txt");
}

TEST_F(UTEST_GimpSignalsHandler, no_backtrace_file)
{
  EXPECT_CALL(seam.gimp_main_init_signals, Call()).WillOnce(Return(nullptr));

  EXPECT_TRUE(handler->install(appCtx));

  EXPECT_TRUE(appCtx->get_backtrace_file().empty());
}

TEST_F(UTEST_GimpSignalsHandler, uninstall_keeps_the_gimp_handlers)
{
  EXPECT_CALL(seam.gimp_main_init_signals, Call()).Times(1);

  EXPECT_TRUE(handler->install(appCtx));

  handler->uninstall();
  handler->uninstall();

  EXPECT_FALSE(appCtx->get_stop());
}
