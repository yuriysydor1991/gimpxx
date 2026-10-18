#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <csignal>
#include <cstdlib>
#include <memory>

#include "src/app/ApplicationFactory.h"
#include "src/app/signals-handlers/GimpSignalsHandler.h"
#include "src/app/signals-handlers/SignalsHandler.h"
#include "src/gimp/app/main.h"

using namespace app;
using namespace testing;

class CTEST_app : public Test
{
 public:
  CTEST_app() { seam.reset(); }

  ~CTEST_app() override
  {
    seam.reset();
    g_strfreev(args);
  }

  /// @brief Runs the ApplicationFactory::execute with the given space
  /// separated command line, the binary name included.
  int execute_with(const gchar* const commandLine)
  {
    g_strfreev(args);

    args = g_strsplit(commandLine, " ", -1);
    realArgc = static_cast<int>(g_strv_length(args));
    realArgv = args;

    return ApplicationFactory::execute(realArgc, realArgv);
  }

  /// @brief Expects GIMP to run on its own, as the only instance.
  void expect_the_gimp_run(const Matcher<const gchar*>& backtraceFile)
  {
    EXPECT_CALL(seam.gimp_main_check_sanity, Call()).Times(1);
    EXPECT_CALL(seam.gimp_main_run, Call(StrEq("CTEST_app"), backtraceFile))
        .WillOnce(Return(EXIT_SUCCESS));
  }

  gimpMainMock& seam{gimpMainMock::instance()};

  int argc{0};
  char** argv{nullptr};

  gchar** args{nullptr};
  int realArgc{0};
  char** realArgv{nullptr};

  ApplicationFactory facade;
};

TEST_F(CTEST_app, create_default_context_success)
{
  auto actx = facade.create_default_context(argc, argv);

  EXPECT_NE(actx, nullptr);
  EXPECT_EQ(&actx->get_options(), gimp_main_options());
}

TEST_F(CTEST_app, create_default_arg_parser_success)
{
  auto argp = facade.create_default_arg_parser();

  EXPECT_NE(argp, nullptr);
}

TEST_F(CTEST_app, create_default_signals_handler_success)
{
  auto shandler = facade.create_default_signals_handler();

  EXPECT_NE(std::dynamic_pointer_cast<GimpSignalsHandler>(shandler), nullptr);
}

TEST_F(CTEST_app, create_default_application_success)
{
  auto dapp = facade.create_default_application();

  EXPECT_NE(dapp, nullptr);
}

TEST_F(CTEST_app, create_application_success)
{
  auto actx = facade.create_default_context(argc, argv);

  EXPECT_NE(actx, nullptr);

  auto app = facade.create_application(actx);

  EXPECT_NE(app, nullptr);
}

TEST_F(CTEST_app, default_signals_handler_keeps_the_backtrace_file)
{
  auto actx = facade.create_default_context(argc, argv);
  auto shandler = facade.create_default_signals_handler();

  EXPECT_CALL(seam.gimp_main_init_signals, Call()).WillOnce([]() {
    return g_strdup("CTEST_app.backtrace");
  });

  EXPECT_TRUE(shandler->install(actx));

  EXPECT_EQ(actx->get_backtrace_file(), "CTEST_app.backtrace");
}

TEST_F(CTEST_app, signals_handler_stops_the_context)
{
  auto actx = facade.create_default_context(argc, argv);
  auto shandler = std::make_shared<SignalsHandler>();

  EXPECT_TRUE(shandler->install(actx));
  EXPECT_FALSE(actx->get_stop());

  std::raise(SIGTERM);

  // False positive: the raise does return, since the installed handler
  // does return too.
  // cppcheck-suppress unreachableCode
  EXPECT_TRUE(actx->get_stop());
}

TEST_F(CTEST_app, signals_handler_pauses_and_resumes_the_context)
{
#if defined(SIGTSTP) && defined(SIGCONT)
  auto actx = facade.create_default_context(argc, argv);
  auto shandler = std::make_shared<SignalsHandler>();

  EXPECT_TRUE(shandler->install(actx));
  EXPECT_FALSE(actx->get_pause());

  // The installed handler replaces the OS default disposition, so the current
  // process is not suspended by the raise below.
  std::raise(SIGTSTP);

  // False positive: the raise does return, since the installed handler
  // does return too.
  // cppcheck-suppress unreachableCode
  EXPECT_TRUE(actx->get_pause());
  EXPECT_FALSE(actx->get_stop());

  std::raise(SIGCONT);

  EXPECT_FALSE(actx->get_pause());
  EXPECT_FALSE(actx->get_stop());
#endif  // defined(SIGTSTP) && defined(SIGCONT)
}

TEST_F(CTEST_app, signals_handler_reloads_the_context)
{
#ifdef SIGHUP
  auto actx = facade.create_default_context(argc, argv);
  auto shandler = std::make_shared<SignalsHandler>();

  EXPECT_TRUE(shandler->install(actx));
  EXPECT_FALSE(actx->get_reload());

  // The SIGHUP signal asks for a configuration reload here and terminates the
  // process under the OS default disposition, so the raise below proves the
  // handler took it over.
  std::raise(SIGHUP);

  // False positive: the raise does return, since the installed handler
  // does return too.
  // cppcheck-suppress unreachableCode
  EXPECT_TRUE(actx->get_reload());
  EXPECT_FALSE(actx->get_stop());
#endif  // SIGHUP
}

TEST_F(CTEST_app, signals_handler_serves_the_user_requests)
{
#if defined(SIGUSR1) && defined(SIGUSR2)
  auto actx = facade.create_default_context(argc, argv);
  auto shandler = std::make_shared<SignalsHandler>();

  EXPECT_TRUE(shandler->install(actx));
  EXPECT_FALSE(actx->get_first_user_request());
  EXPECT_FALSE(actx->get_second_user_request());

  std::raise(SIGUSR1);

  // False positive: the raise does return, since the installed handler
  // does return too.
  // cppcheck-suppress unreachableCode
  EXPECT_TRUE(actx->get_first_user_request());
  EXPECT_FALSE(actx->get_second_user_request());

  std::raise(SIGUSR2);

  EXPECT_TRUE(actx->get_second_user_request());

  EXPECT_FALSE(actx->get_stop());
#endif  // defined(SIGUSR1) && defined(SIGUSR2)
}

TEST_F(CTEST_app, signals_handler_ignores_the_broken_pipe)
{
#ifdef SIGPIPE
  auto actx = facade.create_default_context(argc, argv);
  auto shandler = std::make_shared<SignalsHandler>();

  EXPECT_TRUE(shandler->install(actx));

  // The OS default disposition terminates the process here, so the test case
  // simply reaching its end proves the signal is ignored.
  std::raise(SIGPIPE);

  // False positive: the raise does return, since the signal is ignored.
  // cppcheck-suppress unreachableCode
  EXPECT_FALSE(actx->get_stop());
  EXPECT_FALSE(actx->get_pause());
  EXPECT_FALSE(actx->get_reload());
#endif  // SIGPIPE
}

TEST_F(CTEST_app, execute_runs_gimp_with_the_backtrace_file)
{
  EXPECT_CALL(seam.gimp_main_init_platform, Call(_, _)).Times(1);
  EXPECT_CALL(seam.gimp_main_init_signals, Call()).WillOnce([]() {
    return g_strdup("CTEST_app.backtrace");
  });
  EXPECT_CALL(seam.gimp_main_init_process, Call(_)).Times(1);
  EXPECT_CALL(seam.gimp_main_option_context_new, Call()).Times(1);
  EXPECT_CALL(seam.gimp_main_run_remote, Call(_)).Times(1);
  expect_the_gimp_run(StrEq("CTEST_app.backtrace"));

  EXPECT_EQ(execute_with("CTEST_app image.xcf"), EXIT_SUCCESS);
}

TEST_F(CTEST_app, version_is_shown_instead_of_the_gimp_run)
{
  EXPECT_CALL(seam.gimp_main_show_version, Call()).Times(1);
  EXPECT_CALL(seam.gimp_main_option_context_new, Call()).Times(0);
  EXPECT_CALL(seam.gimp_main_run, Call(_, _)).Times(0);

  EXPECT_EQ(execute_with("CTEST_app --version"), EXIT_SUCCESS);

  EXPECT_FALSE(gimp_main_options()->be_verbose);
}

TEST_F(CTEST_app, short_version_is_the_verbose_one)
{
  EXPECT_CALL(seam.gimp_main_show_version, Call()).Times(1);
  EXPECT_CALL(seam.gimp_main_run, Call(_, _)).Times(0);

  EXPECT_EQ(execute_with("CTEST_app -v"), EXIT_SUCCESS);

  EXPECT_TRUE(gimp_main_options()->be_verbose);
}

TEST_F(CTEST_app, no_interface_runs_a_new_instance_in_the_console)
{
  EXPECT_CALL(seam.gimp_main_attach_console, Call()).Times(1);
  EXPECT_CALL(seam.gimp_main_run_remote, Call(_)).Times(0);
  expect_the_gimp_run(IsNull());

  EXPECT_EQ(execute_with("CTEST_app --no-interface"), EXIT_SUCCESS);

  EXPECT_TRUE(gimp_main_options()->no_interface);
  EXPECT_TRUE(gimp_main_options()->new_instance);
}

TEST_F(CTEST_app, running_instance_takes_the_command_line_over)
{
  EXPECT_CALL(seam.gimp_main_run_remote, Call(NotNull()))
      .WillOnce(DoAll(SetArgPointee<0>(EXIT_FAILURE), Return(TRUE)));
  EXPECT_CALL(seam.gimp_main_check_sanity, Call()).Times(0);
  EXPECT_CALL(seam.gimp_main_run, Call(_, _)).Times(0);

  EXPECT_EQ(execute_with("CTEST_app image.xcf"), EXIT_FAILURE);
}

TEST_F(CTEST_app, unknown_option_fails_before_the_gimp_run)
{
  EXPECT_CALL(seam.gimp_main_option_context_new, Call()).WillOnce([]() {
    return g_option_context_new(nullptr);
  });
  EXPECT_CALL(seam.gimp_main_report_parse_error,
              Call(Field(&GError::message, HasSubstr("--bogus"))))
      .Times(1);
  EXPECT_CALL(seam.gimp_main_run, Call(_, _)).Times(0);

  EXPECT_NE(execute_with("CTEST_app --bogus"), EXIT_SUCCESS);
}
