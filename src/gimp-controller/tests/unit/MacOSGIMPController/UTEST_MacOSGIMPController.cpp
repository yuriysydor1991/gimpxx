#include <glib/gstdio.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <clocale>
#include <string>

#include "src/gimp-controller/gimp-c-includes.h"
#include "src/gimp-controller/macos/MacOSGIMPController.h"

using namespace gimp_controller;
using namespace testing;

class UTEST_MacOSGIMPController : public Test
{
 public:
  /// @brief The environment variables the test cases change.
  inline static const gchar* const touchedVariables[]{"HOME",
                                                      "GIMP_NO_WRAPPER",
                                                      "XDG_CACHE_HOME",
                                                      "GTK_IM_MODULE",
                                                      "PATH",
                                                      "XDG_DATA_DIRS",
                                                      "GIO_MODULE_DIR",
                                                      "GDK_PIXBUF_MODULE_FILE",
                                                      "GTK_PATH",
                                                      "GTK_IM_MODULE_FILE",
                                                      "FONTCONFIG_PATH",
                                                      "LIBTHAI_DICTDIR",
                                                      "GS_LIB",
                                                      "WMF_FONTDIR",
                                                      "GI_TYPELIB_PATH",
                                                      "PYTHONHOME",
                                                      "LANG",
                                                      "LC_ALL",
                                                      "LC_MONETARY"};

  UTEST_MacOSGIMPController()
      : environment{g_get_environ()},
        installDir{g_dir_make_tmp("UTEST_MacOSGIMPController-XXXXXX", nullptr)}
  {
    gimp.reset();

    ON_CALL(gimp.gimp_installation_directory, Call())
        .WillByDefault(Return(installDir));

    g_unsetenv("GIMP_NO_WRAPPER");
    g_unsetenv("XDG_DATA_DIRS");
    g_unsetenv("PYTHONHOME");
    g_setenv("PATH", "/usr/bin", TRUE);
  }

  ~UTEST_MacOSGIMPController() override
  {
    gimp.reset();

    for (const gchar* const name : touchedVariables) {
      const gchar* const value = g_environ_getenv(environment, name);

      if (value != nullptr) {
        g_setenv(name, value, TRUE);
      } else {
        g_unsetenv(name);
      }
    }

    g_strfreev(environment);
    remove_tree(installDir);
    g_free(installDir);
  }

  /// @brief Creates the given directory of the GIMP installation.
  void make_install_dir(const gchar* const relativePath) const
  {
    gchar* const path = g_build_filename(installDir, relativePath, nullptr);

    g_mkdir_with_parents(path, 0700);
    g_free(path);
  }

  /// @brief Removes the given directory tree.
  static void remove_tree(const gchar* const path)
  {
    GDir* const dir = g_dir_open(path, 0, nullptr);

    if (dir != nullptr) {
      for (const gchar* name = g_dir_read_name(dir); name != nullptr;
           name = g_dir_read_name(dir)) {
        gchar* const child = g_build_filename(path, name, nullptr);

        remove_tree(child);
        g_free(child);
      }

      g_dir_close(dir);
    }

    g_remove(path);
  }

  /// @brief Runs the platform start-up with the given program path.
  void init_platform_with(const gchar* const progName)
  {
    gchar* const programPath = g_strdup(progName);
    char* platformArgs[2]{programPath, nullptr};
    int argc{1};
    char** argv{platformArgs};

    controller.init_platform(argc, argv);

    g_free(programPath);
  }

  /// @brief Gives the path of the GIMP installation the given relative one.
  std::string install_path(const std::string& relativePath) const
  {
    return std::string{installDir} + "/" + relativePath;
  }

  /// @brief Gives the value of the environment variable, "<unset>" for none.
  static std::string env(const gchar* const name)
  {
    const gchar* const value = g_getenv(name);

    return value != nullptr ? value : "<unset>";
  }

  gimpCMock& gimp{gimpCMock::instance()};
  gchar** environment;
  gchar* installDir;
  MacOSGIMPController controller;
};

TEST_F(UTEST_MacOSGIMPController, the_controller_is_a_gimp_controller)
{
  EXPECT_NE(dynamic_cast<GIMPController*>(&controller), nullptr);
}

TEST_F(UTEST_MacOSGIMPController, the_session_identifier_is_removed)
{
  char binaryName[11]{"binaryName"};
  char firstPsn[13]{"-psn_0_12345"};
  char imageName[10]{"image.xcf"};
  char secondPsn[9]{"-psn_1_2"};
  char* args[5]{binaryName, firstPsn, imageName, secondPsn, nullptr};
  int argc{4};
  char** argv{args};

  controller.init_platform(argc, argv);

  EXPECT_EQ(argc, 2);
  EXPECT_STREQ(argv[0], "binaryName");
  EXPECT_STREQ(argv[1], "image.xcf");
  EXPECT_EQ(argv[2], nullptr);
}

TEST_F(UTEST_MacOSGIMPController, the_cache_and_the_input_method_are_set)
{
  g_setenv("HOME", "/Users/tester", TRUE);
  g_setenv("GIMP_NO_WRAPPER", "1", TRUE);

  init_platform_with("/Applications/GIMP.app/Contents/MacOS/gimp");

  EXPECT_EQ(env("XDG_CACHE_HOME"),
            "/Users/tester/Library/Caches/GIMP/" GIMP_APP_VERSION);
  EXPECT_EQ(env("GTK_IM_MODULE"), "quartz");
  // The wrapper sets the bundle environment up on its own.
  EXPECT_EQ(env("PATH"), "/usr/bin");
  EXPECT_EQ(env("GIO_MODULE_DIR"), "<unset>");
}

TEST_F(UTEST_MacOSGIMPController, the_bundle_environment_is_set)
{
  make_install_dir("lib/Python.framework/Versions/" PYTHON_VERSION);

  init_platform_with("/Applications/GIMP.app/Contents/MacOS/gimp");

  EXPECT_EQ(env("PATH"), "/Applications/GIMP.app/Contents/MacOS:/usr/bin");
  EXPECT_EQ(env("XDG_DATA_DIRS"), install_path("share"));
  EXPECT_EQ(env("GIO_MODULE_DIR"), install_path("lib/gio/modules"));
  EXPECT_EQ(env("GDK_PIXBUF_MODULE_FILE"),
            install_path("lib/gdk-pixbuf-2.0/2.10.0/loaders.cache"));
  EXPECT_EQ(env("GTK_PATH"), install_path("lib/gtk-3.0/3.0.0"));
  EXPECT_EQ(env("GTK_IM_MODULE_FILE"),
            install_path("lib/gtk-3.0/3.0.0/immodules.cache"));
  EXPECT_EQ(env("FONTCONFIG_PATH"), install_path("etc/fonts"));
  EXPECT_EQ(env("LIBTHAI_DICTDIR"), install_path("share/libthai"));
  EXPECT_EQ(env("GS_LIB"), install_path("share/ghostscript/Resource/Init"));
  EXPECT_EQ(env("WMF_FONTDIR"), install_path("share/libwmf/fonts"));
  EXPECT_EQ(env("GI_TYPELIB_PATH"), install_path("lib/girepository-1.0"));
  EXPECT_EQ(env("PYTHONHOME"),
            install_path("lib/Python.framework/Versions/" PYTHON_VERSION));
}

TEST_F(UTEST_MacOSGIMPController, the_build_directory_shares_its_root)
{
  make_install_dir("share");
  g_setenv("XDG_DATA_DIRS", "/usr/share", TRUE);

  init_platform_with("/build/app/gimp");

  EXPECT_EQ(env("PATH"), "/build/app:/usr/bin");
  EXPECT_EQ(env("XDG_DATA_DIRS"), std::string{installDir} + ":/usr/share");
  EXPECT_EQ(env("LIBTHAI_DICTDIR"), install_path("libthai"));
  EXPECT_EQ(env("PYTHONHOME"), "<unset>");
}

TEST_F(UTEST_MacOSGIMPController, the_prefix_installation_keeps_the_paths)
{
  init_platform_with("/opt/gimp/bin/gimp");

  EXPECT_EQ(env("PATH"), "/usr/bin");
  EXPECT_EQ(env("XDG_DATA_DIRS"), "<unset>");
  EXPECT_EQ(env("GTK_IM_MODULE"), "quartz");
}

TEST_F(UTEST_MacOSGIMPController, the_monetary_characters_stay_ascii)
{
  const std::string previousLocale{setlocale(LC_ALL, nullptr)};
  char binaryName[11]{"binaryName"};
  char* args[2]{binaryName, nullptr};

  g_unsetenv("LC_ALL");
  g_setenv("LANG", "C.UTF-8", TRUE);
  g_setenv("LC_MONETARY", "C.UTF-8", TRUE);

  char** const argvCopy = controller.init_process(args);

  EXPECT_STREQ(setlocale(LC_MONETARY, nullptr), "C");

  g_strfreev(argvCopy);
  setlocale(LC_ALL, previousLocale.c_str());
}
