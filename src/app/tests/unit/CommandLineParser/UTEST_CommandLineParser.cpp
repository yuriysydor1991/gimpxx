#include <glib.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <memory>

#include "src/app/CommandLineParser.h"

using namespace app;
using namespace testing;

class UTEST_CommandLineParser : public Test
{
 public:
  UTEST_CommandLineParser()
      : parser{std::make_shared<CommandLineParser>()},
        appctx{std::make_shared<ApplicationContext>(argc, argv)}
  {
    // The scan reports no errors, the GIMP option parsing does.
    EXPECT_CALL(*appctx, push_error(_)).Times(0);
  }

  /// @brief Makes the space separated command line the context one.
  void set_args(const gchar* const commandLine)
  {
    gchar** const args = g_strsplit(commandLine, " ", -1);

    argc = static_cast<int>(g_strv_length(args));
    appctx->adopt_argv(args);
  }

  /// @brief Expects the flags the scan of the command line gives.
  void expect_the_flags(const bool beVerbose, const bool noInterface,
                        const bool printVersion, const bool printHelp)
  {
    EXPECT_EQ(appctx->get_be_verbose(), beVerbose);
    EXPECT_EQ(appctx->get_no_interface(), noInterface);
    EXPECT_EQ(appctx->get_print_version_and_exit(), printVersion);
    EXPECT_EQ(appctx->get_print_help(), printHelp);
  }

  int argc{0};
  char** argv{nullptr};

  std::shared_ptr<CommandLineParser> parser;
  std::shared_ptr<ApplicationContext> appctx;
};

TEST_F(UTEST_CommandLineParser, no_context_error)
{
  EXPECT_FALSE(parser->parse_args({}));
}

TEST_F(UTEST_CommandLineParser, empty_context)
{
  EXPECT_TRUE(parser->parse_args(appctx));

  expect_the_flags(false, false, false, false);
}

TEST_F(UTEST_CommandLineParser, verbose)
{
  set_args("binaryName --verbose");

  EXPECT_TRUE(parser->parse_args(appctx));

  expect_the_flags(true, false, false, false);
}

TEST_F(UTEST_CommandLineParser, no_interface_short)
{
  set_args("binaryName -i");

  EXPECT_TRUE(parser->parse_args(appctx));

  expect_the_flags(false, true, false, false);
}

TEST_F(UTEST_CommandLineParser, no_interface_long)
{
  set_args("binaryName --no-interface");

  EXPECT_TRUE(parser->parse_args(appctx));

  expect_the_flags(false, true, false, false);
}

TEST_F(UTEST_CommandLineParser, version_short_is_verbose_too)
{
  set_args("binaryName -v");

  EXPECT_TRUE(parser->parse_args(appctx));

  expect_the_flags(true, false, true, false);
}

TEST_F(UTEST_CommandLineParser, version_long)
{
  set_args("binaryName --version");

  EXPECT_TRUE(parser->parse_args(appctx));

  expect_the_flags(false, false, true, false);
}

TEST_F(UTEST_CommandLineParser, the_verbose_after_the_version_counts)
{
  set_args("binaryName --version --verbose");

  EXPECT_TRUE(parser->parse_args(appctx));

  expect_the_flags(true, false, true, false);
}

TEST_F(UTEST_CommandLineParser, the_options_after_the_version_are_skipped)
{
  set_args("binaryName --version --no-interface --help");

  EXPECT_TRUE(parser->parse_args(appctx));

  expect_the_flags(false, false, true, false);
}

TEST_F(UTEST_CommandLineParser, the_options_before_the_version_count)
{
  set_args("binaryName --no-interface --help --version");

  EXPECT_TRUE(parser->parse_args(appctx));

  expect_the_flags(false, true, true, true);
}

TEST_F(UTEST_CommandLineParser, help_short)
{
  set_args("binaryName -?");

  EXPECT_TRUE(parser->parse_args(appctx));

  expect_the_flags(false, false, false, true);
}

TEST_F(UTEST_CommandLineParser, help_long)
{
  set_args("binaryName --help");

  EXPECT_TRUE(parser->parse_args(appctx));

  expect_the_flags(false, false, false, true);
}

TEST_F(UTEST_CommandLineParser, help_group)
{
  set_args("binaryName --help-gtk");

  EXPECT_TRUE(parser->parse_args(appctx));

  expect_the_flags(false, false, false, true);
}

TEST_F(UTEST_CommandLineParser, the_arguments_stay_for_the_gimp_option_parsing)
{
  set_args("binaryName --verbose -i image.xcf");

  char** const args = appctx->get_argv();

  EXPECT_TRUE(parser->parse_args(appctx));

  expect_the_flags(true, true, false, false);
  EXPECT_EQ(appctx->get_argv(), args);
  EXPECT_EQ(appctx->get_argc(), 4);
  EXPECT_STREQ(appctx->get_argv()[3], "image.xcf");
}
