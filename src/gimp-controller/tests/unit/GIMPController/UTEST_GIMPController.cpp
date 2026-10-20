#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdlib>
#include <string>
#include <utility>
#include <vector>

#include "GIMPControllerHooksMock.h"
#include "src/gimp-controller/GIMPController.h"
#include "src/gimp-controller/gimp-c-includes.h"

using namespace gimp_controller;
using namespace testing;

class UTEST_GIMPController : public Test
{
 public:
#ifdef GIMP_CONSOLE_COMPILATION
  inline static constexpr const gboolean defaultNoInterface{TRUE};
#else
  inline static constexpr const gboolean defaultNoInterface{FALSE};
#endif

#if defined(GIMP_UNSTABLE) || !defined(GIMP_RELEASE)
  inline static constexpr const gboolean developmentBuild{TRUE};
#else
  inline static constexpr const gboolean developmentBuild{FALSE};
#endif

  inline static constexpr const gint expectedStatus{7};

  UTEST_GIMPController() { gimp.reset(); }

  ~UTEST_GIMPController() override { gimp.reset(); }

  /// @brief Parses the space separated command line with the option context
  /// of the controller, the way the CommandLineParser does.
  bool parse(const gchar* const commandLine)
  {
    gchar** parsedArgs = g_strsplit(commandLine, " ", -1);
    GOptionContext* const context = controller.create_option_context();
    const gboolean parsed =
        g_option_context_parse_strv(context, &parsedArgs, nullptr);

    g_option_context_free(context);
    g_strfreev(parsedArgs);

    return parsed;
  }

  /// @brief Matches the GFile of the given path.
  static Matcher<GFile*> file_of(const std::string& path)
  {
    return Truly([path](GFile* file) {
      gchar* const filePath = file != nullptr ? g_file_get_path(file) : nullptr;
      const bool matches = filePath != nullptr && path == filePath;

      g_free(filePath);

      return matches;
    });
  }

  /// @brief Matches the NULL terminated array of the given strings.
  static Matcher<const gchar**> strv_of(const std::vector<std::string>& strings)
  {
    return Truly([strings](const gchar** strv) {
      std::vector<std::string> actual;

      for (; strv != nullptr && *strv != nullptr; ++strv) {
        actual.emplace_back(*strv);
      }

      return actual == strings;
    });
  }

  /// @brief Expects the run of GIMP as the only instance, which returns the
  /// expectedStatus, after the given expectations.
  void expect_the_gimp_run(const Matcher<const AppRunArgs&>& appRunArgs)
  {
    EXPECT_CALL(gimp.sanity_check_early, Call()).Times(1);
    EXPECT_CALL(gimp.app_run, Call(appRunArgs))
        .WillOnce(Return(expectedStatus));
    EXPECT_CALL(gimp.gimp_env_exit, Call(FALSE)).Times(1);
  }

  gimpCMock& gimp{gimpCMock::instance()};
  NiceMock<GIMPControllerHooksMock> controller;

  char binaryName[11]{"binaryName"};
  char fixedName[10]{"fixedName"};
  char* args[2]{binaryName, nullptr};
  char* fixedArgs[2]{fixedName, nullptr};
};

TEST_F(UTEST_GIMPController, init_platform_fixes_the_arguments_first)
{
  int argc{1};
  char** argv{args};

  InSequence sequence;

  EXPECT_CALL(controller, fix_arguments(Ref(argc), Ref(argv)))
      .WillOnce([this](int& fixedArgc, char**& fixedArgv) {
        fixedArgc = 1;
        fixedArgv = fixedArgs;
      });
  EXPECT_CALL(controller, init_environment(StrEq("fixedName"))).Times(1);
  EXPECT_CALL(controller, attach_console()).Times(developmentBuild ? 1 : 0);

  controller.init_platform(argc, argv);

  EXPECT_EQ(argv, fixedArgs);
}

TEST_F(UTEST_GIMPController, init_signals_inits_the_backtraces_first)
{
  InSequence sequence;

  EXPECT_CALL(gimp.gimp_backtrace_init, Call()).Times(1);
  EXPECT_CALL(gimp.gimp_init_signal_handlers, Call(NotNull()))
      .WillOnce([](gchar** backtraceFile) {
        *backtraceFile = g_strdup("crash.txt");
      });

  EXPECT_EQ(controller.init_signals(), "crash.txt");
}

TEST_F(UTEST_GIMPController, init_signals_without_a_backtrace_file)
{
  EXPECT_CALL(gimp.gimp_init_signal_handlers, Call(NotNull())).Times(1);

  EXPECT_TRUE(controller.init_signals().empty());
}

TEST_F(UTEST_GIMPController, init_process_prepares_the_process_in_order)
{
  {
    InSequence sequence;

    EXPECT_CALL(controller, prepare_process()).Times(1);
    EXPECT_CALL(gimp.gimp_env_init, Call(FALSE)).Times(1);
    EXPECT_CALL(gimp.gimp_log_init, Call()).Times(1);
    EXPECT_CALL(controller, init_locale()).Times(1);
    EXPECT_CALL(
        gimp.gimp_bind_text_domain,
        Call(StrEq(GETTEXT_PACKAGE "-libgimp"), StrEq("/gimp/share/locale")))
        .Times(1);
    EXPECT_CALL(gimp.gimp_bind_text_domain,
                Call(StrEq(GETTEXT_PACKAGE), StrEq("/gimp/share/locale")))
        .Times(1);
    EXPECT_CALL(gimp.gimp_bind_text_domain,
                Call(StrEq("iso_639_3"), StrEq(ISO_CODES_LOCALEDIR)))
        .Times(1);
    EXPECT_CALL(controller, copy_arguments(args)).Times(1);
  }

  char** const argvCopy = controller.init_process(args);

  ASSERT_NE(argvCopy, nullptr);
  EXPECT_NE(argvCopy, args);
  EXPECT_STREQ(argvCopy[0], "binaryName");
  EXPECT_EQ(argvCopy[1], nullptr);
  EXPECT_STREQ(g_get_application_name(), GIMP_NAME);
  EXPECT_STREQ(g_get_prgname(), GIMP_DESKTOP_NAME);

  g_strfreev(argvCopy);
}

TEST_F(UTEST_GIMPController, show_version)
{
  InSequence sequence;

  EXPECT_CALL(gimp.gimp_version_show, Call(FALSE)).Times(1);
  EXPECT_CALL(gimp.gimp_version_show, Call(TRUE)).Times(1);

  controller.show_version();
  controller.set_be_verbose(true);
  controller.show_version();
}

TEST_F(UTEST_GIMPController, the_option_context_inits_the_libraries)
{
  EXPECT_CALL(gimp.app_libs_init, Call(NotNull(), defaultNoInterface)).Times(1);

  GOptionContext* const context = controller.create_option_context();

  EXPECT_NE(context, nullptr);

  g_option_context_free(context);
}

TEST_F(UTEST_GIMPController, the_libraries_get_the_no_interface_option)
{
  EXPECT_CALL(gimp.app_libs_init, Call(NotNull(), TRUE)).Times(1);

  controller.set_no_interface(true);

  g_option_context_free(controller.create_option_context());
}

TEST_F(UTEST_GIMPController, the_parsing_reads_the_early_configuration)
{
  GimpEarlyRc* const earlyrc =
      static_cast<GimpEarlyRc*>(g_object_new(G_TYPE_OBJECT, nullptr));

  InSequence sequence;

  EXPECT_CALL(gimp.app_libs_init, Call(NotNull(), defaultNoInterface)).Times(1);
  EXPECT_CALL(gimp.gimp_early_rc_new,
              Call(file_of("/system/gimprc"), file_of("/user/gimprc"), TRUE))
      .WillOnce(Return(earlyrc));
  EXPECT_CALL(gimp.gimp_early_rc_get_language, Call(earlyrc))
      .WillOnce(Return(g_strdup("uk")));
  EXPECT_CALL(gimp.language_init, Call(StrEq("uk"), IsNull())).Times(1);
  EXPECT_CALL(controller, apply_early_configuration(earlyrc)).Times(1);

  EXPECT_TRUE(
      parse("binaryName --verbose --system-gimprc=/system/gimprc "
            "--gimprc=/user/gimprc"));
}

TEST_F(UTEST_GIMPController, the_early_configuration_without_gimprc_files)
{
  EXPECT_CALL(gimp.gimp_early_rc_new, Call(IsNull(), IsNull(), FALSE)).Times(1);
  EXPECT_CALL(gimp.language_init, Call(IsNull(), IsNull())).Times(1);

  EXPECT_TRUE(parse("binaryName"));
}

TEST_F(UTEST_GIMPController, the_version_option_shows_the_version_and_exits)
{
  InSequence sequence;

  EXPECT_CALL(controller, attach_console()).Times(1);
  EXPECT_CALL(gimp.gimp_version_show, Call(TRUE)).Times(1);
  EXPECT_CALL(gimp.app_exit, Call(EXIT_SUCCESS)).Times(1);

  EXPECT_TRUE(parse("binaryName --verbose --version"));
}

TEST_F(UTEST_GIMPController, the_license_option_shows_the_version_and_exits)
{
  InSequence sequence;

  EXPECT_CALL(controller, attach_console()).Times(1);
  EXPECT_CALL(gimp.gimp_version_show, Call(FALSE)).Times(1);
  EXPECT_CALL(gimp.app_exit, Call(EXIT_SUCCESS)).Times(1);

  EXPECT_TRUE(parse("binaryName --license"));
}

TEST_F(UTEST_GIMPController, the_dump_gimprc_options_dump_the_configuration)
{
  for (const auto& [option, format] :
       std::vector<std::pair<std::string, GimpConfigDumpFormat>>{
           {"--dump-gimprc", GIMP_CONFIG_DUMP_GIMPRC},
           {"--dump-gimprc-system", GIMP_CONFIG_DUMP_GIMPRC_SYSTEM},
           {"--dump-gimprc-manpage", GIMP_CONFIG_DUMP_GIMPRC_MANPAGE}}) {
    InSequence sequence;

    EXPECT_CALL(controller, attach_console()).Times(1);
    EXPECT_CALL(gimp.babl_init, Call()).Times(1);
    EXPECT_CALL(gimp.gimp_load_config, Call(NotNull(), IsNull(), IsNull()))
        .Times(1);
    EXPECT_CALL(gimp.gimp_config_dump, Call(NotNull(), format))
        .WillOnce(Return(TRUE));
    EXPECT_CALL(gimp.app_exit, Call(EXIT_SUCCESS)).Times(1);

    // The real app_exit does not return, the mock one fails the parsing.
    EXPECT_FALSE(parse(("binaryName " + option).c_str())) << option;

    Mock::VerifyAndClearExpectations(&controller);
    gimp.reset();
  }
}

TEST_F(UTEST_GIMPController, the_failed_gimprc_dump_exits_with_a_failure)
{
  EXPECT_CALL(gimp.gimp_config_dump, Call(NotNull(), GIMP_CONFIG_DUMP_GIMPRC))
      .WillOnce(Return(FALSE));
  EXPECT_CALL(gimp.app_exit, Call(EXIT_FAILURE)).Times(1);

  EXPECT_FALSE(parse("binaryName --dump-gimprc"));
}

TEST_F(UTEST_GIMPController, the_deprecated_procedures_dump)
{
  static const gchar* const firstProcedure{"first-procedure"};
  static const gchar* const secondProcedure{"second-procedure"};

  Gimp* loadedGimp{nullptr};

  InSequence sequence;

  EXPECT_CALL(gimp.babl_init, Call()).Times(1);
  EXPECT_CALL(gimp.gimp_load_config, Call(NotNull(), IsNull(), IsNull()))
      .WillOnce(SaveArg<0>(&loadedGimp));
  EXPECT_CALL(gimp.internal_procs_init, Call(IsNull()))
      .WillOnce([&loadedGimp](GimpPDB*) {
        // The compatibility mode includes the deprecated procedures.
        EXPECT_EQ(loadedGimp->pdb_compat_mode, GIMP_PDB_COMPAT_ON);
      });
  EXPECT_CALL(gimp.gimp_pdb_get_deprecated_procedures, Call(IsNull()))
      .WillOnce([](GimpPDB*) {
        return g_list_append(
            g_list_append(nullptr, const_cast<gchar*>(firstProcedure)),
            const_cast<gchar*>(secondProcedure));
      });
  EXPECT_CALL(gimp.gimp_object_get_name, Call(firstProcedure))
      .WillOnce(Return(firstProcedure));
  EXPECT_CALL(gimp.gimp_object_get_name, Call(secondProcedure))
      .WillOnce(Return(secondProcedure));
  EXPECT_CALL(gimp.app_exit, Call(EXIT_SUCCESS)).Times(1);

  EXPECT_FALSE(parse("binaryName --dump-pdb-procedures-deprecated"));
}

TEST_F(UTEST_GIMPController, the_fatal_warnings_option)
{
  const GLogLevelFlags previousMask =
      g_log_set_always_fatal(static_cast<GLogLevelFlags>(G_LOG_FATAL_MASK));

  EXPECT_TRUE(parse("binaryName --g-fatal-warnings"));

  const GLogLevelFlags fatalMask = g_log_set_always_fatal(previousMask);

  EXPECT_EQ(fatalMask & (G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL),
            G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL);
}

TEST_F(UTEST_GIMPController, the_wrong_mode_names_fail_the_parsing)
{
  EXPECT_FALSE(parse("binaryName --stack-trace-mode=sometimes"));
  EXPECT_FALSE(parse("binaryName --pdb-compat-mode=maybe"));
}

TEST_F(UTEST_GIMPController, report_parse_error_attaches_the_console)
{
  EXPECT_CALL(controller, attach_console()).Times(1);

  controller.report_parse_error(g_error_new_literal(
      G_OPTION_ERROR, G_OPTION_ERROR_UNKNOWN_OPTION, "Unknown option --bogus"));
}

TEST_F(UTEST_GIMPController, report_parse_error_of_the_interface)
{
  EXPECT_CALL(controller, attach_console()).Times(0);

  controller.report_parse_error(nullptr);
}

TEST_F(UTEST_GIMPController, run_starts_gimp_in_order)
{
  InSequence sequence;

#ifdef GIMP_CONSOLE_COMPILATION
  EXPECT_CALL(gimp.gdk_display_get_default, Call()).Times(0);
  EXPECT_CALL(controller, attach_console()).Times(1);
  EXPECT_CALL(controller, open_in_running_instance(_, _)).Times(0);
#else
  EXPECT_CALL(gimp.gdk_display_get_default, Call()).Times(AtLeast(1));
  EXPECT_CALL(controller, attach_console()).Times(0);
  EXPECT_CALL(controller, open_in_running_instance(IsNull(), FALSE)).Times(1);
  EXPECT_CALL(gimp.gimp_unique_open, Call(IsNull(), FALSE))
      .WillOnce(Return(FALSE));
#endif
  EXPECT_CALL(gimp.sanity_check_early, Call()).Times(1);
  EXPECT_CALL(controller, has_shared_memory()).Times(1);
  EXPECT_CALL(gimp.app_run, Call(_)).WillOnce(Return(expectedStatus));
  EXPECT_CALL(gimp.gimp_env_exit, Call(FALSE)).Times(1);

  EXPECT_EQ(controller.run("binaryName", ""), expectedStatus);
}

TEST_F(UTEST_GIMPController, run_passes_the_defaults_to_gimp)
{
  expect_the_gimp_run(AllOf(
      Field(&AppRunArgs::full_prog_name, StrEq("binaryName")),
      Field(&AppRunArgs::filenames, IsNull()),
      Field(&AppRunArgs::alternate_system_gimprc, IsNull()),
      Field(&AppRunArgs::alternate_gimprc, IsNull()),
      Field(&AppRunArgs::session_name, IsNull()),
      Field(&AppRunArgs::batch_interpreter, IsNull()),
      Field(&AppRunArgs::batch_commands, IsNull()),
      Field(&AppRunArgs::quit, FALSE), Field(&AppRunArgs::as_new, FALSE),
      Field(&AppRunArgs::no_interface, defaultNoInterface),
      Field(&AppRunArgs::no_data, FALSE), Field(&AppRunArgs::no_fonts, FALSE),
      Field(&AppRunArgs::no_splash, FALSE),
      Field(&AppRunArgs::be_verbose, FALSE), Field(&AppRunArgs::use_shm, TRUE),
      Field(&AppRunArgs::use_cpu_accel, TRUE),
      Field(&AppRunArgs::console_messages, FALSE),
      Field(&AppRunArgs::use_debug_handler, FALSE),
      Field(&AppRunArgs::show_playground, developmentBuild),
      Field(&AppRunArgs::show_debug_menu, developmentBuild),
      Field(&AppRunArgs::stack_trace_mode,
            developmentBuild ? GIMP_STACK_TRACE_QUERY : GIMP_STACK_TRACE_NEVER),
      Field(&AppRunArgs::pdb_compat_mode,
            developmentBuild ? GIMP_PDB_COMPAT_WARN : GIMP_PDB_COMPAT_ON),
      Field(&AppRunArgs::backtrace_file, IsNull())));

  EXPECT_EQ(controller.run("binaryName", ""), expectedStatus);
}

TEST_F(UTEST_GIMPController, run_passes_the_parsed_options_to_gimp)
{
  ASSERT_TRUE(
      parse("binaryName --new-instance --as-new --no-data --no-fonts "
            "--no-splash --no-shm --no-cpu-accel --session=work "
            "--system-gimprc=/system/gimprc --gimprc=/user/gimprc "
            "--batch-interpreter=python-fu-eval --batch=first --batch=second "
            "--quit --console-messages --debug-handlers --show-playground "
            "--show-debug-menu --stack-trace-mode=always "
            "--pdb-compat-mode=off first.xcf second.xcf"));

  EXPECT_CALL(controller, attach_console()).Times(1);
  EXPECT_CALL(controller, open_in_running_instance(_, _)).Times(0);
  expect_the_gimp_run(AllOf(
      Field(&AppRunArgs::filenames, strv_of({"first.xcf", "second.xcf"})),
      Field(&AppRunArgs::alternate_system_gimprc, file_of("/system/gimprc")),
      Field(&AppRunArgs::alternate_gimprc, file_of("/user/gimprc")),
      Field(&AppRunArgs::session_name, StrEq("work")),
      Field(&AppRunArgs::batch_interpreter, StrEq("python-fu-eval")),
      Field(&AppRunArgs::batch_commands, strv_of({"first", "second"})),
      Field(&AppRunArgs::quit, TRUE), Field(&AppRunArgs::as_new, TRUE),
      Field(&AppRunArgs::no_data, TRUE), Field(&AppRunArgs::no_fonts, TRUE),
      Field(&AppRunArgs::no_splash, TRUE), Field(&AppRunArgs::use_shm, FALSE),
      Field(&AppRunArgs::use_cpu_accel, FALSE),
      Field(&AppRunArgs::console_messages, TRUE),
      Field(&AppRunArgs::use_debug_handler, TRUE),
      Field(&AppRunArgs::show_playground, TRUE),
      Field(&AppRunArgs::show_debug_menu, TRUE),
      Field(&AppRunArgs::stack_trace_mode, GIMP_STACK_TRACE_ALWAYS),
      Field(&AppRunArgs::pdb_compat_mode, GIMP_PDB_COMPAT_OFF),
      Field(&AppRunArgs::backtrace_file, StrEq("crash.txt"))));

  EXPECT_EQ(controller.run("binaryName", "crash.txt"), expectedStatus);
}

TEST_F(UTEST_GIMPController, run_without_the_shared_memory_of_the_platform)
{
  EXPECT_CALL(controller, has_shared_memory()).WillOnce(Return(FALSE));
  expect_the_gimp_run(Field(&AppRunArgs::use_shm, FALSE));

  EXPECT_EQ(controller.run("binaryName", ""), expectedStatus);
}

TEST_F(UTEST_GIMPController, run_attaches_the_console_for_the_output)
{
  controller.set_be_verbose(true);

  EXPECT_CALL(controller, attach_console()).Times(1);
  expect_the_gimp_run(Field(&AppRunArgs::be_verbose, TRUE));

  EXPECT_EQ(controller.run("binaryName", ""), expectedStatus);
}

TEST_F(UTEST_GIMPController, run_without_interface_runs_a_new_instance)
{
  controller.set_no_interface(true);

  EXPECT_CALL(gimp.gdk_display_get_default, Call()).Times(0);
  EXPECT_CALL(controller, attach_console()).Times(1);
  EXPECT_CALL(controller, open_in_running_instance(_, _)).Times(0);
  expect_the_gimp_run(Field(&AppRunArgs::no_interface, TRUE));

  EXPECT_EQ(controller.run("binaryName", ""), expectedStatus);
}

TEST_F(UTEST_GIMPController, run_aborts_gimp_once_the_libraries_are_not_sane)
{
  InSequence sequence;

  EXPECT_CALL(gimp.sanity_check_early, Call()).WillOnce(Return("not sane"));
  EXPECT_CALL(gimp.app_abort, Call(defaultNoInterface, StrEq("not sane")))
      .Times(1);
  // The real app_abort does not return.
  EXPECT_CALL(gimp.app_run, Call(_)).Times(1);

  controller.run("binaryName", "");
}

#ifndef GIMP_CONSOLE_COMPILATION
TEST_F(UTEST_GIMPController, run_fails_without_a_display)
{
  EXPECT_CALL(gimp.gdk_display_get_default, Call()).WillOnce(Return(nullptr));
  EXPECT_CALL(controller, open_in_running_instance(_, _)).Times(0);
  EXPECT_CALL(gimp.app_run, Call(_)).Times(0);

  EXPECT_EQ(controller.run("binaryName", ""), EXIT_FAILURE);
}

TEST_F(UTEST_GIMPController, the_running_instance_takes_the_command_line_over)
{
  ASSERT_TRUE(parse("binaryName --as-new --batch=command image.xcf"));

  InSequence sequence;

  EXPECT_CALL(controller, attach_console()).Times(1);
  EXPECT_CALL(controller,
              open_in_running_instance(strv_of({"image.xcf"}), TRUE))
      .WillOnce(Return(TRUE));
  EXPECT_CALL(gimp.gimp_unique_batch_run, Call(IsNull(), strv_of({"command"})))
      .WillOnce(Return(TRUE));
  EXPECT_CALL(gimp.gdk_notify_startup_complete, Call()).Times(1);
  EXPECT_CALL(gimp.sanity_check_early, Call()).Times(0);
  EXPECT_CALL(gimp.app_run, Call(_)).Times(0);

  EXPECT_EQ(controller.run("binaryName", ""), EXIT_SUCCESS);
}

TEST_F(UTEST_GIMPController, the_failed_batch_of_the_running_instance)
{
  ASSERT_TRUE(parse("binaryName --batch=command"));

  EXPECT_CALL(gimp.gimp_unique_open, Call(IsNull(), FALSE))
      .WillOnce(Return(TRUE));
  EXPECT_CALL(gimp.gimp_unique_batch_run, Call(IsNull(), NotNull()))
      .WillOnce(Return(FALSE));
  EXPECT_CALL(gimp.app_run, Call(_)).Times(0);

  EXPECT_EQ(controller.run("binaryName", ""), EXIT_FAILURE);
}
#else
TEST_F(UTEST_GIMPController, the_console_binary_hides_the_no_interface_option)
{
  GOptionContext* const context = controller.create_option_context();
  gchar* const help = g_option_context_get_help(context, TRUE, nullptr);

  EXPECT_THAT(help, Not(HasSubstr("--no-interface")));
  EXPECT_THAT(help, HasSubstr("--no-splash"));

  g_free(help);
  g_option_context_free(context);
}
#endif
