#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <memory>
#include <string>

#include "src/app/CommandLineParser.h"
#include "src/gimp/app/main.h"

using namespace app;
using namespace testing;

class UTEST_CommandLineParser : public Test
{
 public:
  inline static const std::string expectedError{
      "Fail to parse the command line"};

  UTEST_CommandLineParser()
      : parser{std::make_shared<CommandLineParser>()},
        appctx{std::make_shared<ApplicationContext>(argc, argv, options)}
  {
    seam.reset();
  }

  ~UTEST_CommandLineParser() override { seam.reset(); }

  /// @brief Makes the space separated command line the context one, a GLib
  /// allocated copy as the GIMP start-up gives it.
  void set_args(const gchar* const commandLine)
  {
    gchar** const args = g_strsplit(commandLine, " ", -1);

    argc = static_cast<int>(g_strv_length(args));
    appctx->adopt_argv(args);
  }

  void expect_the_parsed_options(const int times)
  {
    EXPECT_CALL(seam.gimp_main_option_context_new, Call()).Times(times);
    EXPECT_CALL(*appctx, adopt_option_context(NotNull())).Times(times);
    EXPECT_CALL(*appctx, push_error(_)).Times(0);
  }

  gimpMainMock& seam{gimpMainMock::instance()};

  int argc{0};
  char** argv{nullptr};
  GimpMainOptions options{};

  std::shared_ptr<CommandLineParser> parser;
  std::shared_ptr<ApplicationContext> appctx;
};

TEST_F(UTEST_CommandLineParser, no_context_error)
{
  EXPECT_CALL(seam.gimp_main_option_context_new, Call()).Times(0);

  EXPECT_FALSE(parser->parse_args({}));
}

TEST_F(UTEST_CommandLineParser, empty_context)
{
  expect_the_parsed_options(1);

  EXPECT_TRUE(parser->parse_args(appctx));

  EXPECT_FALSE(appctx->get_print_version_and_exit());
  EXPECT_FALSE(options.be_verbose);
  EXPECT_FALSE(options.no_interface);
}

TEST_F(UTEST_CommandLineParser, verbose)
{
  set_args("binaryName --verbose");
  expect_the_parsed_options(1);

  EXPECT_TRUE(parser->parse_args(appctx));

  EXPECT_TRUE(options.be_verbose);
  EXPECT_FALSE(appctx->get_print_version_and_exit());
}

TEST_F(UTEST_CommandLineParser, no_interface_short)
{
  set_args("binaryName -i");
  expect_the_parsed_options(1);

  EXPECT_TRUE(parser->parse_args(appctx));

  EXPECT_TRUE(options.no_interface);
  EXPECT_FALSE(options.be_verbose);
}

TEST_F(UTEST_CommandLineParser, no_interface_long)
{
  set_args("binaryName --no-interface");
  expect_the_parsed_options(1);

  EXPECT_TRUE(parser->parse_args(appctx));

  EXPECT_TRUE(options.no_interface);
  EXPECT_FALSE(options.be_verbose);
}

TEST_F(UTEST_CommandLineParser, version_short_is_verbose_too)
{
  set_args("binaryName -v");
  expect_the_parsed_options(0);

  EXPECT_TRUE(parser->parse_args(appctx));

  EXPECT_TRUE(appctx->get_print_version_and_exit());
  EXPECT_TRUE(options.be_verbose);
}

TEST_F(UTEST_CommandLineParser, version_long)
{
  set_args("binaryName --version");
  expect_the_parsed_options(0);

  EXPECT_TRUE(parser->parse_args(appctx));

  EXPECT_TRUE(appctx->get_print_version_and_exit());
  EXPECT_FALSE(options.be_verbose);
}

TEST_F(UTEST_CommandLineParser, the_verbose_after_the_version_counts)
{
  set_args("binaryName --version --verbose");
  expect_the_parsed_options(0);

  EXPECT_TRUE(parser->parse_args(appctx));

  EXPECT_TRUE(appctx->get_print_version_and_exit());
  EXPECT_TRUE(options.be_verbose);
}

TEST_F(UTEST_CommandLineParser, the_options_after_the_version_are_skipped)
{
  set_args("binaryName --version --no-interface");
  expect_the_parsed_options(0);

  EXPECT_TRUE(parser->parse_args(appctx));

  EXPECT_TRUE(appctx->get_print_version_and_exit());
  EXPECT_FALSE(options.no_interface);
}

TEST_F(UTEST_CommandLineParser, help_short_attaches_the_console)
{
  set_args("binaryName -?");
  expect_the_parsed_options(1);

  EXPECT_CALL(seam.gimp_main_attach_console, Call()).Times(1);

  EXPECT_TRUE(parser->parse_args(appctx));
}

TEST_F(UTEST_CommandLineParser, help_long_attaches_the_console)
{
  set_args("binaryName --help");
  expect_the_parsed_options(1);

  EXPECT_CALL(seam.gimp_main_attach_console, Call()).Times(1);

  EXPECT_TRUE(parser->parse_args(appctx));
}

TEST_F(UTEST_CommandLineParser, help_group_attaches_the_console)
{
  set_args("binaryName --help-gtk");
  expect_the_parsed_options(1);

  EXPECT_CALL(seam.gimp_main_attach_console, Call()).Times(1);

  EXPECT_TRUE(parser->parse_args(appctx));
}

TEST_F(UTEST_CommandLineParser, other_options_do_not_attach_the_console)
{
  set_args("binaryName --verbose -i image.xcf");
  expect_the_parsed_options(1);

  EXPECT_CALL(seam.gimp_main_attach_console, Call()).Times(0);

  EXPECT_TRUE(parser->parse_args(appctx));
}

TEST_F(UTEST_CommandLineParser, unknown_option_error)
{
  set_args("binaryName --unknown");

  EXPECT_CALL(seam.gimp_main_option_context_new, Call()).WillOnce([]() {
    return g_option_context_new(nullptr);
  });
  EXPECT_CALL(*appctx, adopt_option_context(NotNull())).Times(1);
  EXPECT_CALL(seam.gimp_main_report_parse_error,
              Call(Field(&GError::message, HasSubstr("--unknown"))))
      .Times(1);
  EXPECT_CALL(*appctx, push_error(expectedError)).Times(1);

  EXPECT_FALSE(parser->parse_args(appctx));

  EXPECT_THAT(appctx->get_errors(), ElementsAre(expectedError));
  EXPECT_FALSE(appctx->get_print_version_and_exit());
}
