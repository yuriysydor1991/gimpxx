#ifndef GIMPXX_PROJECT_COMMANDLINEPARAMETERNAMES_CLASS_H
#define GIMPXX_PROJECT_COMMANDLINEPARAMETERNAMES_CLASS_H

#include <string>

namespace app
{

/**
 * @brief Class for encapsulating the command line parameters the
 * CommandLineParser looks for before the GIMP option parsing, which handles
 * all of them, the help ones included.
 */
class CMDParamNames
{
 public:
  virtual ~CMDParamNames() = default;
  CMDParamNames() = default;

  inline static const std::string VERBOSEW{"--verbose"};
  inline static const std::string NO_INTERFACEW{"--no-interface"};
  inline static const std::string NO_INTERFACE{"-i"};
  inline static const std::string VERSIONW{"--version"};
  /// @brief Both the version and the verbose output, as in GIMP.
  inline static const std::string VERSION{"-v"};
  inline static const std::string HELPW{"--help"};
  inline static const std::string HELP{"-?"};
  /// @brief The prefix of the help options of the GIMP option groups.
  inline static const std::string HELP_GROUP_PREFIX{"--help-"};
};

}  // namespace app

#endif  // GIMPXX_PROJECT_COMMANDLINEPARAMETERNAMES_CLASS_H
