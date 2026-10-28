#ifndef GIMPXX_PROJECT_APPLICATIONCONTEXT_CLASS_H
#define GIMPXX_PROJECT_APPLICATIONCONTEXT_CLASS_H

#include <glib.h>
#include <gmock/gmock.h>

#include <atomic>
#include <memory>
#include <string>
#include <vector>

namespace app
{

class ApplicationContext
{
 public:
  ApplicationContext(int& gargc, char**& gargv);

  int& get_argc() const { return margc; }
  char**& get_argv() const { return margv; }

  void adopt_argv(char** argvCopy)
  {
    madopted_argv.reset(argvCopy);
    margv = argvCopy;
  }

  bool get_be_verbose() const { return mbe_verbose; }
  void set_be_verbose(const bool newValue) { mbe_verbose = newValue; }

  bool get_no_interface() const { return mno_interface; }
  void set_no_interface(const bool newValue) { mno_interface = newValue; }

  bool get_print_help() const { return mprint_help; }
  void set_print_help(const bool newValue) { mprint_help = newValue; }

  bool get_print_version_and_exit() const { return mprint_version_and_exit; }
  void set_print_version_and_exit(const bool newValue)
  {
    mprint_version_and_exit = newValue;
  }

  const std::vector<std::string>& get_errors() const { return merrors; }

  bool get_stop() const { return mstop.load(); }
  void set_stop(const bool newValue) { mstop.store(newValue); }

  bool get_pause() const { return mpause.load(); }
  void set_pause(const bool newValue) { mpause.store(newValue); }

  bool get_reload() const { return mreload.load(); }
  void set_reload(const bool newValue) { mreload.store(newValue); }

  bool get_first_user_request() const { return mfirst_user_request.load(); }
  void set_first_user_request(const bool newValue)
  {
    mfirst_user_request.store(newValue);
  }

  bool get_second_user_request() const { return msecond_user_request.load(); }
  void set_second_user_request(const bool newValue)
  {
    msecond_user_request.store(newValue);
  }

  MOCK_METHOD(void, push_error, (const std::string& errorDescription));

 private:
  int& margc;
  char**& margv;
  std::unique_ptr<char*, decltype(&g_strfreev)> madopted_argv{nullptr,
                                                              &g_strfreev};
  bool mbe_verbose{false};
  bool mno_interface{false};
  bool mprint_help{false};
  bool mprint_version_and_exit{false};
  std::vector<std::string> merrors;
  std::atomic_bool mstop{false};
  std::atomic_bool mpause{false};
  std::atomic_bool mreload{false};
  std::atomic_bool mfirst_user_request{false};
  std::atomic_bool msecond_user_request{false};
};

}  // namespace app

#endif  // GIMPXX_PROJECT_APPLICATIONCONTEXT_CLASS_H
