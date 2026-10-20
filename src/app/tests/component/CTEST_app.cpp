#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <csignal>
#include <cstdlib>
#include <memory>

#include "src/app/ApplicationFactory.h"
#include "src/app/signals-handlers/GimpSignalsHandler.h"
#include "src/app/signals-handlers/SignalsHandler.h"
#include "src/gimp-controller/gimp-c-includes.h"
#include "src/gimp-controller/linux/LinuxGIMPController.h"

using namespace app;
using namespace gimp_controller;
using namespace testing;

class CTEST_app : public Test
{
 public:
  CTEST_app()
      : glibWarningsHandler{
            g_log_set_handler("GLib", G_LOG_LEVEL_WARNING,
                              &skip_the_application_name_warning, nullptr)}
  {
    gimp.reset();
  }

  ~CTEST_app() override
  {
    gimp.reset();
    g_strfreev(args);
    g_log_remove_handler("GLib", glibWarningsHandler);
  }

  /// @brief Skips the warning of the application name, which every GIMP
  /// start-up of the test process sets again, and passes the other GLib
  /// warnings on.
  static void skip_the_application_name_warning(const gchar* logDomain,
                                                GLogLevelFlags logLevel,
                                                const gchar* message,
                                                gpointer data)
  {
    if (g_strcmp0(message, "g_set_application_name() called multiple times") !=
        0) {
      g_log_default_handler(logDomain, logLevel, message, data);
    }
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

  /// @brief Expects GIMP to run on its own, as the only instance, with the
  /// given app_run arguments.
  void expect_the_gimp_run(const Matcher<const AppRunArgs&>& appRunArgs)
  {
    EXPECT_CALL(gimp.sanity_check_early, Call()).Times(1);
    EXPECT_CALL(
        gimp.app_run,
        Call(AllOf(Field(&AppRunArgs::full_prog_name, StrEq("CTEST_app")),
                   appRunArgs)))
        .WillOnce(Return(EXIT_SUCCESS));
    EXPECT_CALL(gimp.gimp_env_exit, Call(FALSE)).Times(1);
  }

  /// @brief Makes the GIMP signal handlers choose the given backtrace file.
  void expect_the_backtrace_file(const gchar* const backtraceFile)
  {
    EXPECT_CALL(gimp.gimp_backtrace_init, Call()).Times(1);
    EXPECT_CALL(gimp.gimp_init_signal_handlers, Call(NotNull()))
        .WillOnce(
            [backtraceFile](gchar** file) { *file = g_strdup(backtraceFile); });
  }

  const guint glibWarningsHandler;
  gimpCMock& gimp{gimpCMock::instance()};

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

  ASSERT_NE(actx, nullptr);
  EXPECT_NE(dynamic_cast<LinuxGIMPController*>(&actx->get_gimp_controller()),
            nullptr);
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

  expect_the_backtrace_file("CTEST_app.backtrace");

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
  expect_the_backtrace_file("CTEST_app.backtrace");
  EXPECT_CALL(gimp.gimp_env_init, Call(FALSE)).Times(1);
  EXPECT_CALL(gimp.app_libs_init, Call(NotNull(), FALSE)).Times(1);
  EXPECT_CALL(gimp.gimp_early_rc_new, Call(IsNull(), IsNull(), FALSE)).Times(1);
  EXPECT_CALL(gimp.gimp_unique_open, Call(NotNull(), FALSE))
      .WillOnce(Return(FALSE));
  expect_the_gimp_run(
      AllOf(Field(&AppRunArgs::backtrace_file, StrEq("CTEST_app.backtrace")),
            Field(&AppRunArgs::filenames, Truly([](const gchar** filenames) {
              return filenames != nullptr &&
                     g_strcmp0(filenames[0], "image.xcf") == 0 &&
                     filenames[1] == nullptr;
            }))));

  EXPECT_EQ(execute_with("CTEST_app image.xcf"), EXIT_SUCCESS);
}

TEST_F(CTEST_app, version_is_shown_instead_of_the_gimp_run)
{
  EXPECT_CALL(gimp.gimp_version_show, Call(FALSE)).Times(1);
  EXPECT_CALL(gimp.app_libs_init, Call(_, _)).Times(0);
  EXPECT_CALL(gimp.app_run, Call(_)).Times(0);

  EXPECT_EQ(execute_with("CTEST_app --version"), EXIT_SUCCESS);
}

TEST_F(CTEST_app, short_version_is_the_verbose_one)
{
  EXPECT_CALL(gimp.gimp_version_show, Call(TRUE)).Times(1);
  EXPECT_CALL(gimp.app_run, Call(_)).Times(0);

  EXPECT_EQ(execute_with("CTEST_app -v"), EXIT_SUCCESS);
}

TEST_F(CTEST_app, no_interface_runs_a_new_instance)
{
  EXPECT_CALL(gimp.app_libs_init, Call(NotNull(), TRUE)).Times(1);
  EXPECT_CALL(gimp.gdk_display_get_default, Call()).Times(0);
  EXPECT_CALL(gimp.gimp_unique_open, Call(_, _)).Times(0);
  expect_the_gimp_run(AllOf(Field(&AppRunArgs::no_interface, TRUE),
                            Field(&AppRunArgs::backtrace_file, IsNull())));

  EXPECT_EQ(execute_with("CTEST_app --no-interface"), EXIT_SUCCESS);
}

TEST_F(CTEST_app, running_instance_takes_the_command_line_over)
{
  EXPECT_CALL(gimp.gimp_unique_open, Call(NotNull(), FALSE))
      .WillOnce(Return(TRUE));
  EXPECT_CALL(gimp.gimp_unique_batch_run, Call(IsNull(), NotNull()))
      .WillOnce(Return(FALSE));
  EXPECT_CALL(gimp.gdk_notify_startup_complete, Call()).Times(1);
  EXPECT_CALL(gimp.sanity_check_early, Call()).Times(0);
  EXPECT_CALL(gimp.app_run, Call(_)).Times(0);

  EXPECT_EQ(execute_with("CTEST_app --batch=(gimp-quit) image.xcf"),
            EXIT_FAILURE);
}

TEST_F(CTEST_app, unknown_option_fails_before_the_gimp_run)
{
  EXPECT_CALL(gimp.app_libs_init, Call(NotNull(), FALSE)).Times(1);
  EXPECT_CALL(gimp.gimp_early_rc_new, Call(_, _, _)).Times(0);
  EXPECT_CALL(gimp.app_run, Call(_)).Times(0);

  EXPECT_NE(execute_with("CTEST_app --bogus"), EXIT_SUCCESS);
}
