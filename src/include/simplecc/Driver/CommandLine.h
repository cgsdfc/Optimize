#ifndef SIMPLECOMPILER_COMMANDLINE_H
#define SIMPLECOMPILER_COMMANDLINE_H
#include <memory>
#include <string>
#include <tclap/CmdLine.h>
#include <vector>

namespace simplecc {
namespace tclap = TCLAP;
class CommandLine {
  tclap::CmdLine Parser;
  tclap::UnlabeledValueArg<std::string> InputArg;
  tclap::ValueArg<std::string> OutputArg;
  std::vector<tclap::Arg *> Switches;

public:
  CommandLine();
  ~CommandLine() = default;
  int run(int Argc, char **Argv);
};
} // namespace simplecc

#endif // SIMPLECOMPILER_COMMANDLINE_H