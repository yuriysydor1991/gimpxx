#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdlib>

#include "src/gimp-controller/gimp-c-includes.h"
#include "src/gimp-controller/linux/LinuxGIMPController.h"

using namespace gimp_controller;
using namespace testing;

class UTEST_LinuxGIMPController : public Test
{
 public:
  UTEST_LinuxGIMPController() : environment{g_get_environ()} { gimp.reset(); }

  ~UTEST_LinuxGIMPController() override
  {
    gimp.reset();
    g_strfreev(environment);
  }

  /// @brief Tells if the process environment is still the one the test case
  /// started with.
  bool environment_is_kept() const
  {
    gchar** const currentEnvironment = g_get_environ();
    const bool kept = g_strv_equal(environment, currentEnvironment);

    g_strfreev(currentEnvironment);

    return kept;
  }

  gimpCMock& gimp{gimpCMock::instance()};
  gchar** environment;
  LinuxGIMPController controller;

  char binaryName[11]{"binaryName"};
  char psnArgument[13]{"-psn_0_12345"};
  char* args[3]{binaryName, psnArgument, nullptr};
};

TEST_F(UTEST_LinuxGIMPController, the_controller_is_a_gimp_controller)
{
  EXPECT_NE(dynamic_cast<GIMPController*>(&controller), nullptr);
}

TEST_F(UTEST_LinuxGIMPController, init_platform_keeps_the_process_as_it_is)
{
  int argc{2};
  char** argv{args};

  controller.init_platform(argc, argv);

  EXPECT_EQ(argc, 2);
  EXPECT_EQ(argv, args);
  EXPECT_STREQ(argv[1], "-psn_0_12345");
  EXPECT_TRUE(environment_is_kept());
}

TEST_F(UTEST_LinuxGIMPController, init_process_copies_the_arguments)
{
  InSequence sequence;

  EXPECT_CALL(gimp.gimp_env_init, Call(FALSE)).Times(1);
  EXPECT_CALL(gimp.gimp_log_init, Call()).Times(1);
  EXPECT_CALL(gimp.gimp_bind_text_domain, Call(_, _)).Times(3);

  char** const argvCopy = controller.init_process(args);

  ASSERT_NE(argvCopy, nullptr);
  EXPECT_NE(argvCopy, args);
  EXPECT_STREQ(argvCopy[0], "binaryName");
  EXPECT_STREQ(argvCopy[1], "-psn_0_12345");

  g_strfreev(argvCopy);
}

TEST_F(UTEST_LinuxGIMPController, the_early_configuration_keeps_the_environment)
{
  GOptionContext* const context = controller.create_option_context();
  gchar** parsedArgs = g_strsplit("binaryName image.xcf", " ", -1);

  EXPECT_CALL(gimp.gimp_early_rc_new, Call(IsNull(), IsNull(), FALSE)).Times(1);

  EXPECT_TRUE(g_option_context_parse_strv(context, &parsedArgs, nullptr));
  EXPECT_TRUE(environment_is_kept());

  g_strfreev(parsedArgs);
  g_option_context_free(context);
}

TEST_F(UTEST_LinuxGIMPController, run_asks_the_running_instance_over_dbus)
{
  InSequence sequence;

  EXPECT_CALL(gimp.gimp_unique_open, Call(IsNull(), FALSE))
      .WillOnce(Return(FALSE));
  EXPECT_CALL(gimp.sanity_check_early, Call()).Times(1);
  EXPECT_CALL(gimp.app_run, Call(Field(&AppRunArgs::use_shm, TRUE)))
      .WillOnce(Return(EXIT_SUCCESS));
  EXPECT_CALL(gimp.gimp_env_exit, Call(FALSE)).Times(1);

  EXPECT_EQ(controller.run("binaryName", ""), EXIT_SUCCESS);
}

TEST_F(UTEST_LinuxGIMPController, the_running_instance_takes_the_files_over)
{
  EXPECT_CALL(gimp.gimp_unique_open, Call(IsNull(), FALSE))
      .WillOnce(Return(TRUE));
  EXPECT_CALL(gimp.gdk_notify_startup_complete, Call()).Times(1);
  EXPECT_CALL(gimp.app_run, Call(_)).Times(0);

  EXPECT_EQ(controller.run("binaryName", ""), EXIT_SUCCESS);
}
