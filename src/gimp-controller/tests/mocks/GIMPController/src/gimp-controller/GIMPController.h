#ifndef GIMPXX_PROJECT_GIMPCONTROLLER_CLASS_H
#define GIMPXX_PROJECT_GIMPCONTROLLER_CLASS_H

#include <glib.h>
#include <gmock/gmock.h>

#include <functional>
#include <string>

namespace gimp_controller
{

class GIMPController
{
 public:
  virtual ~GIMPController() = default;

  /// @brief Gives a GLib copy of the arguments, an option context which
  /// accepts every option without printing the help and frees the reported
  /// parse errors, the way the real controller does, by default.
  GIMPController()
  {
    ON_CALL(*this, init_process(testing::_)).WillByDefault(&g_strdupv);
    ON_CALL(*this, create_option_context()).WillByDefault([]() {
      GOptionContext* const context = g_option_context_new(nullptr);

      g_option_context_set_help_enabled(context, FALSE);
      g_option_context_set_ignore_unknown_options(context, TRUE);

      return context;
    });
    ON_CALL(*this, report_parse_error(testing::_))
        .WillByDefault([](GError* error) { g_clear_error(&error); });

    if (onMockCreate) {
      onMockCreate(*this);
    }
  }

  inline static std::function<void(GIMPController&)> onMockCreate;

  MOCK_METHOD(void, init_platform, (int& argc, char**& argv));
  MOCK_METHOD(std::string, init_signals, ());
  MOCK_METHOD(char**, init_process, (char** argv));
  MOCK_METHOD(void, set_be_verbose, (const bool newValue));
  MOCK_METHOD(void, set_no_interface, (const bool newValue));
  MOCK_METHOD(void, attach_console, ());
  MOCK_METHOD(void, show_version, ());
  MOCK_METHOD(GOptionContext*, create_option_context, ());
  MOCK_METHOD(void, report_parse_error, (GError * error));
  MOCK_METHOD(int, run,
              (const gchar* fullProgName, const std::string& backtraceFile));
};

}  // namespace gimp_controller

#endif  // GIMPXX_PROJECT_GIMPCONTROLLER_CLASS_H
