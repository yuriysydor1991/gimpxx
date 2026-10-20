#include "src/app/ApplicationContext.h"

#include <cassert>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace app
{

ApplicationContext::ApplicationContext(
    int& gargc, char**& gargv,
    std::shared_ptr<gimp_controller::GIMPController> gimpController)
    : margc{gargc}, margv{gargv}, mgimp_controller{std::move(gimpController)}
{
  assert(mgimp_controller != nullptr);
}

ApplicationContext::~ApplicationContext()
{
  if (margv_adopted) {
    g_strfreev(margv);
  }

  if (moption_context != nullptr) {
    g_option_context_free(moption_context);
  }
}

int& ApplicationContext::get_argc() const { return margc; }

char**& ApplicationContext::get_argv() const { return margv; }

void ApplicationContext::adopt_argv(char** argvCopy)
{
  if (margv_adopted) {
    g_strfreev(margv);
  }

  margv = argvCopy;
  margv_adopted = true;
}

gimp_controller::GIMPController& ApplicationContext::get_gimp_controller() const
{
  return *mgimp_controller;
}

void ApplicationContext::adopt_option_context(GOptionContext* optionContext)
{
  if (moption_context != nullptr) {
    g_option_context_free(moption_context);
  }

  moption_context = optionContext;
}

const std::string& ApplicationContext::get_backtrace_file() const
{
  return mbacktrace_file;
}

void ApplicationContext::set_backtrace_file(const std::string& backtraceFile)
{
  mbacktrace_file = backtraceFile;
}

bool ApplicationContext::get_print_version_and_exit() const
{
  return mprint_version_and_exit;
}

void ApplicationContext::set_print_version_and_exit(const bool newValue)
{
  mprint_version_and_exit = newValue;
}

const std::vector<std::string>& ApplicationContext::get_errors() const
{
  return merrors;
}

void ApplicationContext::push_error(const std::string& errorDescription)
{
  merrors.emplace_back(errorDescription);
}

bool ApplicationContext::get_stop() const { return mstop.load(); }

void ApplicationContext::set_stop(const bool newValue)
{
  mstop.store(newValue);
}

bool ApplicationContext::get_pause() const { return mpause.load(); }

void ApplicationContext::set_pause(const bool newValue)
{
  mpause.store(newValue);
}

bool ApplicationContext::get_reload() const { return mreload.load(); }

void ApplicationContext::set_reload(const bool newValue)
{
  mreload.store(newValue);
}

bool ApplicationContext::get_first_user_request() const
{
  return mfirst_user_request.load();
}

void ApplicationContext::set_first_user_request(const bool newValue)
{
  mfirst_user_request.store(newValue);
}

bool ApplicationContext::get_second_user_request() const
{
  return msecond_user_request.load();
}

void ApplicationContext::set_second_user_request(const bool newValue)
{
  msecond_user_request.store(newValue);
}

}  // namespace app
