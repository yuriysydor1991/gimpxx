#include <glib.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <thread>

#include "src/app/ApplicationContext.h"

using namespace app;
using namespace testing;

class UTEST_ApplicationContext : public Test
{
 public:
  inline static constexpr const int expectedRandomInt{123};
  inline static const std::string expectedError{"random error description"};

  UTEST_ApplicationContext() : appCtx{create_context(argc, argv)} {}

  std::shared_ptr<ApplicationContext> create_context(int& gargc, char**& gargv)
  {
    return std::make_shared<ApplicationContext>(gargc, gargv);
  }

  int argc{0};
  char** argv{nullptr};

  std::shared_ptr<ApplicationContext> appCtx;
};

TEST_F(UTEST_ApplicationContext, empty_context)
{
  EXPECT_EQ(appCtx->get_argc(), 0);
  EXPECT_EQ(appCtx->get_argv(), nullptr);

  EXPECT_TRUE(appCtx->get_errors().empty());
  EXPECT_FALSE(appCtx->get_print_version_and_exit());
  EXPECT_FALSE(appCtx->get_stop());
  EXPECT_FALSE(appCtx->get_pause());
  EXPECT_FALSE(appCtx->get_reload());
  EXPECT_FALSE(appCtx->get_first_user_request());
  EXPECT_FALSE(appCtx->get_second_user_request());
  EXPECT_FALSE(appCtx->get_be_verbose());
  EXPECT_FALSE(appCtx->get_no_interface());
  EXPECT_FALSE(appCtx->get_print_help());
}

TEST_F(UTEST_ApplicationContext, custom_argc)
{
  int margc{expectedRandomInt};

  auto customCtx = create_context(margc, argv);

  EXPECT_EQ(customCtx->get_argc(), expectedRandomInt);
  EXPECT_EQ(customCtx->get_argv(), nullptr);

  EXPECT_TRUE(appCtx->get_errors().empty());
  EXPECT_FALSE(appCtx->get_print_version_and_exit());
  EXPECT_FALSE(appCtx->get_stop());
  EXPECT_FALSE(appCtx->get_pause());
  EXPECT_FALSE(appCtx->get_reload());
  EXPECT_FALSE(appCtx->get_first_user_request());
  EXPECT_FALSE(appCtx->get_second_user_request());
}

TEST_F(UTEST_ApplicationContext, custom_argv)
{
  char* pcustomArgv{nullptr};
  char** customArgv{&pcustomArgv};

  auto customCtx = create_context(argc, customArgv);

  EXPECT_EQ(customCtx->get_argc(), argc);
  EXPECT_EQ(customCtx->get_argv(), customArgv);

  EXPECT_TRUE(appCtx->get_errors().empty());
  EXPECT_FALSE(appCtx->get_print_version_and_exit());
  EXPECT_FALSE(appCtx->get_stop());
  EXPECT_FALSE(appCtx->get_pause());
  EXPECT_FALSE(appCtx->get_reload());
  EXPECT_FALSE(appCtx->get_first_user_request());
  EXPECT_FALSE(appCtx->get_second_user_request());
}

TEST_F(UTEST_ApplicationContext, args_are_referenced_and_not_copied)
{
  char* pcustomArgv{nullptr};
  char** customArgv{&pcustomArgv};

  auto customCtx = create_context(argc, customArgv);

  EXPECT_EQ(&customCtx->get_argc(), &argc);
  EXPECT_EQ(&customCtx->get_argv(), &customArgv);

  argc = expectedRandomInt;
  customArgv = nullptr;

  EXPECT_EQ(customCtx->get_argc(), expectedRandomInt);
  EXPECT_EQ(customCtx->get_argv(), nullptr);
}

TEST_F(UTEST_ApplicationContext, adopting_the_argv_replaces_the_referenced_one)
{
  gchar** const argvCopy = g_strsplit("binaryName --verbose", " ", -1);

  appCtx->adopt_argv(argvCopy);

  EXPECT_EQ(argv, argvCopy);
  EXPECT_EQ(appCtx->get_argv(), argvCopy);

  // The previously adopted copy is freed, the latest one by the destructor.
  appCtx->adopt_argv(g_strsplit("otherName", " ", -1));

  EXPECT_STREQ(argv[0], "otherName");
  EXPECT_EQ(argv[1], nullptr);
}

TEST_F(UTEST_ApplicationContext, pushing_an_error)
{
  EXPECT_EQ(appCtx->get_argc(), 0);
  EXPECT_EQ(appCtx->get_argv(), nullptr);

  appCtx->push_error(expectedError);

  EXPECT_FALSE(appCtx->get_errors().empty());

  EXPECT_EQ(appCtx->get_errors().front(), expectedError);

  EXPECT_FALSE(appCtx->get_print_version_and_exit());
}

TEST_F(UTEST_ApplicationContext, pushing_multiple_errors)
{
  EXPECT_EQ(appCtx->get_argc(), 0);
  EXPECT_EQ(appCtx->get_argv(), nullptr);

  appCtx->push_error(expectedError);
  appCtx->push_error(expectedError);

  EXPECT_FALSE(appCtx->get_errors().empty());

  for (const auto& error : appCtx->get_errors()) {
    EXPECT_EQ(error, expectedError);
  }

  EXPECT_FALSE(appCtx->get_print_version_and_exit());
}

TEST_F(UTEST_ApplicationContext, setting_the_print_version_and_exit_flag)
{
  EXPECT_FALSE(appCtx->get_print_version_and_exit());

  appCtx->set_print_version_and_exit(true);

  EXPECT_TRUE(appCtx->get_print_version_and_exit());

  appCtx->set_print_version_and_exit(false);

  EXPECT_FALSE(appCtx->get_print_version_and_exit());
}

TEST_F(UTEST_ApplicationContext, setting_the_gimp_option_flags)
{
  appCtx->set_be_verbose(true);
  appCtx->set_no_interface(true);
  appCtx->set_print_help(true);

  EXPECT_TRUE(appCtx->get_be_verbose());
  EXPECT_TRUE(appCtx->get_no_interface());
  EXPECT_TRUE(appCtx->get_print_help());

  appCtx->set_be_verbose(false);
  appCtx->set_no_interface(false);
  appCtx->set_print_help(false);

  EXPECT_FALSE(appCtx->get_be_verbose());
  EXPECT_FALSE(appCtx->get_no_interface());
  EXPECT_FALSE(appCtx->get_print_help());
}

TEST_F(UTEST_ApplicationContext, setting_the_stop_flag)
{
  EXPECT_FALSE(appCtx->get_stop());

  appCtx->set_stop(true);

  EXPECT_TRUE(appCtx->get_stop());

  appCtx->set_stop(false);

  EXPECT_FALSE(appCtx->get_stop());
}

TEST_F(UTEST_ApplicationContext, raising_the_stop_flag_from_another_thread)
{
  EXPECT_FALSE(appCtx->get_stop());

  std::thread stopper{[this]() { appCtx->set_stop(true); }};

  stopper.join();

  EXPECT_TRUE(appCtx->get_stop());

  EXPECT_TRUE(appCtx->get_errors().empty());
  EXPECT_FALSE(appCtx->get_print_version_and_exit());
}

TEST_F(UTEST_ApplicationContext, setting_the_pause_flag)
{
  EXPECT_FALSE(appCtx->get_pause());

  appCtx->set_pause(true);

  EXPECT_TRUE(appCtx->get_pause());
  EXPECT_FALSE(appCtx->get_stop());

  appCtx->set_pause(false);

  EXPECT_FALSE(appCtx->get_pause());
}

TEST_F(UTEST_ApplicationContext, raising_the_pause_flag_from_another_thread)
{
  EXPECT_FALSE(appCtx->get_pause());

  std::thread pauser{[this]() { appCtx->set_pause(true); }};

  pauser.join();

  EXPECT_TRUE(appCtx->get_pause());

  EXPECT_TRUE(appCtx->get_errors().empty());
  EXPECT_FALSE(appCtx->get_print_version_and_exit());
  EXPECT_FALSE(appCtx->get_stop());
}

TEST_F(UTEST_ApplicationContext, setting_the_reload_flag)
{
  EXPECT_FALSE(appCtx->get_reload());

  appCtx->set_reload(true);

  EXPECT_TRUE(appCtx->get_reload());
  EXPECT_FALSE(appCtx->get_stop());
  EXPECT_FALSE(appCtx->get_pause());

  appCtx->set_reload(false);

  EXPECT_FALSE(appCtx->get_reload());
}

TEST_F(UTEST_ApplicationContext, setting_the_user_request_flags)
{
  EXPECT_FALSE(appCtx->get_first_user_request());
  EXPECT_FALSE(appCtx->get_second_user_request());

  appCtx->set_first_user_request(true);

  EXPECT_TRUE(appCtx->get_first_user_request());
  EXPECT_FALSE(appCtx->get_second_user_request());

  appCtx->set_second_user_request(true);

  EXPECT_TRUE(appCtx->get_first_user_request());
  EXPECT_TRUE(appCtx->get_second_user_request());

  appCtx->set_first_user_request(false);
  appCtx->set_second_user_request(false);

  EXPECT_FALSE(appCtx->get_first_user_request());
  EXPECT_FALSE(appCtx->get_second_user_request());

  EXPECT_FALSE(appCtx->get_stop());
  EXPECT_FALSE(appCtx->get_pause());
  EXPECT_FALSE(appCtx->get_reload());
}

TEST_F(UTEST_ApplicationContext, raising_the_reload_flag_from_another_thread)
{
  EXPECT_FALSE(appCtx->get_reload());

  std::thread reloader{[this]() { appCtx->set_reload(true); }};

  reloader.join();

  EXPECT_TRUE(appCtx->get_reload());

  EXPECT_TRUE(appCtx->get_errors().empty());
  EXPECT_FALSE(appCtx->get_stop());
  EXPECT_FALSE(appCtx->get_pause());
}
