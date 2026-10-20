#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <memory>

#include "src/app/ApplicationFactory.h"
#include "src/app/applications/Application.h"
#include "src/app/applications/ApplicationVersionPrinter.h"
#include "src/app/signals-handlers/GimpSignalsHandler.h"
#include "src/gimp-controller/GIMPController.h"

#if defined(G_OS_WIN32)
#include "src/gimp-controller/windows/WindowsGIMPController.h"
#elif defined(__APPLE__)
#include "src/gimp-controller/macos/MacOSGIMPController.h"
#else
#include "src/gimp-controller/linux/LinuxGIMPController.h"
#endif

using namespace app;
using namespace gimp_controller;
using namespace testing;

class UTEST_ApplicationFactory : public Test
{
 public:
  UTEST_ApplicationFactory() : factory{std::make_shared<ApplicationFactory>()}
  {
    // Every run starts the platform and the process through the created GIMP
    // controller, which only some test cases check.
    GIMPController::onMockCreate = [](GIMPController& instance) {
      EXPECT_CALL(instance, init_platform(_, _)).Times(AnyNumber());
      EXPECT_CALL(instance, init_process(_)).Times(AnyNumber());
    };
  }

  ~UTEST_ApplicationFactory() override
  {
    CommandLineParser::onMockCreate = nullptr;
    Application::onMockCreate = nullptr;
    ApplicationVersionPrinter::onMockCreate = nullptr;
    GimpSignalsHandler::onMockCreate = nullptr;
    GIMPController::onMockCreate = nullptr;
  }

  /// @brief Expects the creation of a single GIMP controller, which starts the
  /// platform before the signals handler install and the process after it.
  void expect_the_gimp_controller(gchar** const argvCopy)
  {
    EXPECT_CALL(controllerCreationEnsurer, Call(_))
        .WillOnce([this, argvCopy](GIMPController& instance) {
          InSequence sequence;

          EXPECT_CALL(instance, init_platform(Ref(customArgc), Ref(customArgv)))
              .Times(1);
          EXPECT_CALL(signalsHandlerInstall, Call()).Times(1);
          EXPECT_CALL(instance, init_process(args)).WillOnce(Return(argvCopy));
        });

    GIMPController::onMockCreate = controllerCreationEnsurer.AsStdFunction();
  }

  /// @brief Expects the signals handler install before the process start-up
  /// replaces the arguments with their copy.
  void expect_the_signals_handler_install(const bool result)
  {
    EXPECT_CALL(signalsHandlerCreationEnsurer, Call(_))
        .WillOnce([this, result](GimpSignalsHandler& instance) {
          EXPECT_CALL(instance, install(_))
              .WillOnce(
                  [this, result](std::shared_ptr<ApplicationContext> ctx) {
                    signalsHandlerInstall.Call();
                    EXPECT_EQ(ctx->get_argv(), args);
                    return result;
                  });
        });

    GimpSignalsHandler::onMockCreate =
        signalsHandlerCreationEnsurer.AsStdFunction();
  }

  /// @brief Expects the parsing of the context which refers to the given
  /// arguments.
  void expect_the_parsed_arguments(const bool result, const bool version)
  {
    EXPECT_CALL(parserCreationEnsurer, Call(_))
        .WillOnce([this, result, version](CommandLineParser& instance) {
          EXPECT_CALL(instance, parse_args(_))
              .WillOnce([this, result,
                         version](std::shared_ptr<ApplicationContext> ctx) {
                EXPECT_EQ(&ctx->get_argc(), &customArgc);
                EXPECT_EQ(&ctx->get_argv(), &customArgv);
                ctx->set_print_version_and_exit(version);
                return result;
              });
        });

    CommandLineParser::onMockCreate = parserCreationEnsurer.AsStdFunction();
  }

  /// @brief Expects the run of a single created application of the given
  /// type.
  template <typename ApplicationType>
  void expect_the_run(MockFunction<void(ApplicationType&)>& creationEnsurer,
                      const int status)
  {
    EXPECT_CALL(creationEnsurer, Call(_))
        .WillOnce([status](ApplicationType& instance) {
          EXPECT_CALL(instance, run(NotNull())).WillOnce(Return(status));
        });

    ApplicationType::onMockCreate = creationEnsurer.AsStdFunction();
  }

  MockFunction<void(GIMPController& instance)> controllerCreationEnsurer;
  NiceMock<MockFunction<void()>> signalsHandlerInstall;
  MockFunction<void(GimpSignalsHandler& instance)>
      signalsHandlerCreationEnsurer;
  MockFunction<void(CommandLineParser& instance)> parserCreationEnsurer;
  MockFunction<void(Application& instance)> appCreationEnsurer;
  MockFunction<void(ApplicationVersionPrinter& instance)>
      versionPrinterCreationEnsurer;

  std::shared_ptr<ApplicationFactory> factory;

  char binaryName[11]{"binaryName"};
  char* args[2]{binaryName, nullptr};

  int customArgc{1};
  char** customArgv{args};
  std::shared_ptr<GIMPController> gimpController{
      std::make_shared<NiceMock<GIMPController>>()};
};

TEST_F(UTEST_ApplicationFactory, create_default_context)
{
  std::shared_ptr<ApplicationContext> ctx =
      factory->create_default_context(customArgc, customArgv);

  EXPECT_NE(ctx, nullptr);
  EXPECT_EQ(&ctx->get_argc(), &customArgc);
  EXPECT_EQ(&ctx->get_argv(), &customArgv);
}

TEST_F(UTEST_ApplicationFactory, create_default_context_holds_a_gimp_controller)
{
  GIMPController* createdController{nullptr};

  EXPECT_CALL(controllerCreationEnsurer, Call(_))
      .WillOnce([&createdController](GIMPController& instance) {
        createdController = &instance;
      });
  GIMPController::onMockCreate = controllerCreationEnsurer.AsStdFunction();

  std::shared_ptr<ApplicationContext> ctx =
      factory->create_default_context(customArgc, customArgv);

  ASSERT_NE(ctx, nullptr);
  EXPECT_EQ(&ctx->get_gimp_controller(), createdController);
}

TEST_F(UTEST_ApplicationFactory, create_default_gimp_controller_of_the_platform)
{
  std::shared_ptr<GIMPController> controller =
      factory->create_default_gimp_controller();

#if defined(G_OS_WIN32)
  EXPECT_NE(std::dynamic_pointer_cast<WindowsGIMPController>(controller),
            nullptr);
#elif defined(__APPLE__)
  EXPECT_NE(std::dynamic_pointer_cast<MacOSGIMPController>(controller),
            nullptr);
#else
  EXPECT_NE(std::dynamic_pointer_cast<LinuxGIMPController>(controller),
            nullptr);
#endif
}

TEST_F(UTEST_ApplicationFactory, create_default_arg_parser)
{
  std::shared_ptr<CommandLineParser> parser =
      factory->create_default_arg_parser();

  EXPECT_NE(parser, nullptr);
}

TEST_F(UTEST_ApplicationFactory, create_default_signals_handler)
{
  std::shared_ptr<ISignalsHandler> signalsHandler =
      factory->create_default_signals_handler();

  EXPECT_NE(std::dynamic_pointer_cast<GimpSignalsHandler>(signalsHandler),
            nullptr);
}

TEST_F(UTEST_ApplicationFactory, create_default_application)
{
  std::shared_ptr<IApplication> app = factory->create_default_application();

  EXPECT_NE(std::dynamic_pointer_cast<Application>(app), nullptr);
}

TEST_F(UTEST_ApplicationFactory, create_version_printer)
{
  std::shared_ptr<IApplication> app = factory->create_version_printer();

  EXPECT_NE(std::dynamic_pointer_cast<ApplicationVersionPrinter>(app), nullptr);
}

TEST_F(UTEST_ApplicationFactory, create_application_invalid_context_error)
{
  std::shared_ptr<IApplication> app = factory->create_application({});

  EXPECT_EQ(app, nullptr);
}

TEST_F(UTEST_ApplicationFactory, create_application_default)
{
  std::shared_ptr<ApplicationContext> ctx =
      std::make_shared<ApplicationContext>(customArgc, customArgv,
                                           gimpController);

  std::shared_ptr<IApplication> app = factory->create_application(ctx);

  EXPECT_NE(std::dynamic_pointer_cast<Application>(app), nullptr);
}

TEST_F(UTEST_ApplicationFactory, create_application_version_printer)
{
  std::shared_ptr<ApplicationContext> ctx =
      std::make_shared<ApplicationContext>(customArgc, customArgv,
                                           gimpController);

  ctx->set_print_version_and_exit(true);

  std::shared_ptr<IApplication> app = factory->create_application(ctx);

  EXPECT_NE(std::dynamic_pointer_cast<ApplicationVersionPrinter>(app), nullptr);
}

TEST_F(UTEST_ApplicationFactory, init_logging_success)
{
  EXPECT_TRUE(factory->init_logging());
}

TEST_F(UTEST_ApplicationFactory, factory_run_starts_gimp_in_order)
{
  gchar** const argvCopy = g_strdupv(args);

  expect_the_gimp_controller(argvCopy);
  expect_the_signals_handler_install(true);
  expect_the_parsed_arguments(true, false);
  expect_the_run(appCreationEnsurer, 0);

  EXPECT_EQ(factory->run(customArgc, customArgv), 0);

  // The context adopted the copy, which the parser and the application got.
  EXPECT_EQ(customArgv, argvCopy);
}

TEST_F(UTEST_ApplicationFactory, factory_run_returns_the_application_status)
{
  static constexpr const int expectedStatus{7};

  expect_the_signals_handler_install(true);
  expect_the_parsed_arguments(true, false);
  expect_the_run(appCreationEnsurer, expectedStatus);

  EXPECT_EQ(factory->run(customArgc, customArgv), expectedStatus);
}

TEST_F(UTEST_ApplicationFactory, factory_run_version_app)
{
  EXPECT_CALL(appCreationEnsurer, Call(_)).Times(0);
  Application::onMockCreate = appCreationEnsurer.AsStdFunction();

  expect_the_signals_handler_install(true);
  expect_the_parsed_arguments(true, true);
  expect_the_run(versionPrinterCreationEnsurer, 0);

  EXPECT_EQ(factory->run(customArgc, customArgv), 0);
}

TEST_F(UTEST_ApplicationFactory, factory_run_survives_the_signals_failure)
{
  expect_the_signals_handler_install(false);
  expect_the_parsed_arguments(true, false);
  expect_the_run(appCreationEnsurer, 0);

  EXPECT_EQ(factory->run(customArgc, customArgv), 0);
}

TEST_F(UTEST_ApplicationFactory, factory_run_parse_error)
{
  EXPECT_CALL(appCreationEnsurer, Call(_)).Times(0);
  Application::onMockCreate = appCreationEnsurer.AsStdFunction();

  expect_the_signals_handler_install(true);
  expect_the_parsed_arguments(false, false);

  EXPECT_EQ(factory->run(customArgc, customArgv), IApplication::INVALID);
}

TEST_F(UTEST_ApplicationFactory, factory_execute_default_app)
{
  expect_the_signals_handler_install(true);
  expect_the_parsed_arguments(true, false);
  expect_the_run(appCreationEnsurer, 0);

  EXPECT_EQ(ApplicationFactory::execute(customArgc, customArgv), 0);
}
