#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdlib>
#include <memory>

#include "src/app/applications/Application.h"
#include "src/gimp/app/main.h"

using namespace app;
using namespace testing;

class UTEST_Application : public Test
{
 public:
  inline static constexpr const gint expectedStatus{7};

  UTEST_Application()
      : app{std::make_shared<Application>()},
        appCtx{std::make_shared<ApplicationContext>(argc, argv, options)}
  {
    seam.reset();
  }

  ~UTEST_Application() override { seam.reset(); }

  /// @brief Expects GIMP to run on its own, as the only instance.
  void expect_the_gimp_run(const Matcher<const gchar*>& backtraceFile)
  {
    EXPECT_CALL(seam.gimp_main_check_sanity, Call()).Times(1);
    EXPECT_CALL(seam.gimp_main_run, Call(StrEq("binaryName"), backtraceFile))
        .WillOnce(Return(expectedStatus));
  }

  gimpMainMock& seam{gimpMainMock::instance()};

  char binaryName[11]{"binaryName"};
  char* args[2]{binaryName, nullptr};

  int argc{1};
  char** argv{args};
  GimpMainOptions options{};

  std::shared_ptr<Application> app;
  std::shared_ptr<ApplicationContext> appCtx;
};

TEST_F(UTEST_Application, no_context_error)
{
  EXPECT_CALL(seam.gimp_main_run, Call(_, _)).Times(0);

  EXPECT_NE(app->run({}), 0);
}

TEST_F(UTEST_Application, the_flatpak_name_failure_is_an_error)
{
  EXPECT_CALL(seam.gimp_main_set_flatpak_prgname, Call())
      .WillOnce(Return(FALSE));
  EXPECT_CALL(seam.gimp_main_run, Call(_, _)).Times(0);

  EXPECT_EQ(app->run(appCtx), IApplication::INVALID);
}

TEST_F(UTEST_Application, gimp_runs_when_no_instance_is_running)
{
  EXPECT_CALL(seam.gimp_main_set_flatpak_prgname, Call()).Times(1);
  EXPECT_CALL(seam.gimp_main_attach_console, Call()).Times(0);
  EXPECT_CALL(seam.gimp_main_run_remote, Call(NotNull()))
      .WillOnce(Return(FALSE));
  expect_the_gimp_run(IsNull());

  EXPECT_EQ(app->run(appCtx), expectedStatus);

  EXPECT_FALSE(options.new_instance);
}

TEST_F(UTEST_Application, gimp_gets_the_backtrace_file)
{
  appCtx->set_backtrace_file("backtrace.txt");
  expect_the_gimp_run(StrEq("backtrace.txt"));

  EXPECT_EQ(app->run(appCtx), expectedStatus);
}

TEST_F(UTEST_Application, the_running_instance_takes_the_command_line_over)
{
  EXPECT_CALL(seam.gimp_main_run_remote, Call(NotNull()))
      .WillOnce(DoAll(SetArgPointee<0>(expectedStatus), Return(TRUE)));
  EXPECT_CALL(seam.gimp_main_check_sanity, Call()).Times(0);
  EXPECT_CALL(seam.gimp_main_run, Call(_, _)).Times(0);

  EXPECT_EQ(app->run(appCtx), expectedStatus);
}

TEST_F(UTEST_Application, the_new_instance_skips_the_running_one)
{
  options.new_instance = TRUE;

  EXPECT_CALL(seam.gimp_main_run_remote, Call(_)).Times(0);
  expect_the_gimp_run(IsNull());

  EXPECT_EQ(app->run(appCtx), expectedStatus);
}

TEST_F(UTEST_Application, no_interface_runs_a_new_instance_in_the_console)
{
  options.no_interface = TRUE;

  EXPECT_CALL(seam.gimp_main_attach_console, Call()).Times(1);
  EXPECT_CALL(seam.gimp_main_run_remote, Call(_)).Times(0);
  expect_the_gimp_run(IsNull());

  EXPECT_EQ(app->run(appCtx), expectedStatus);

  EXPECT_TRUE(options.new_instance);
}

TEST_F(UTEST_Application, the_console_output_attaches_the_console)
{
  static const gchar* batchCommands[] = {"(gimp-quit 0)", nullptr};

  EXPECT_CALL(seam.gimp_main_attach_console, Call()).Times(3);

  options.be_verbose = TRUE;
  EXPECT_EQ(app->run(appCtx), EXIT_SUCCESS);

  options = {};
  options.console_messages = TRUE;
  EXPECT_EQ(app->run(appCtx), EXIT_SUCCESS);

  options = {};
  options.batch_commands = batchCommands;
  EXPECT_EQ(app->run(appCtx), EXIT_SUCCESS);
}
