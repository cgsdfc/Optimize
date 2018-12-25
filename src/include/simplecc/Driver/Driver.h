#ifndef SIMPLECC_DRIVER_DRIVER_H
#define SIMPLECC_DRIVER_DRIVER_H
#include "simplecc/Analysis/AnalysisManager.h"
#include "simplecc/Codegen/ByteCodeModule.h"
#include "simplecc/Lex/TokenInfo.h"
#include "simplecc/Support/ErrorManager.h"

#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <utility>

namespace llvm {
class raw_ostream;
}

namespace simplecc {
class Node;
/// This class exposes a high-level interface to the system in terms of
/// InputFile and OutputFile. It exposes different parts of the system through
/// member functions. It provides both do-all style methods and low level
/// functions such as tokenizing a file. Driver D; D.setInputFile("aaa");
/// D.runAssembleMips();
/// return D.status();
///
class Driver {
  /// Return a ptr to the output stream. Nullptr on failure.
  std::ostream *getStdOstream();

  /// Return a ptr to the input stream. Nullptr on failure.
  std::istream *getStdIstream();

  bool doTokenize();
  std::unique_ptr<Node> doBuildCST();
  bool doParse();
  bool doAnalyses();
  bool doCodeGen();
  bool doAssemble();

public:
  Driver() = default;

  void setInputFile(std::string Filename) { InputFile = std::move(Filename); }
  void setOutputFile(std::string Filename) { OutputFile = std::move(Filename); }

  std::string getInputFile() const { return InputFile; }
  std::string getOutputFile() const { return OutputFile; }

  void runAssembleMips();
  void runPrintTokens();
  void runPrintByteCode();
  void runDumpByteCodeModule();
  void runAnalysisOnly();
  void runDumpSymbolTable();
  void runDumpAst();
  void runDumpCst();

#ifdef SIMPLE_COMPILER_USE_LLVM
  std::unique_ptr<llvm::raw_ostream> getLLVMRawOstream();
  void runEmitLLVMIR();
  void runWriteAstGraph();
  void runWriteCstGraph();
#endif

  int status() const { return !EM.IsOk(); }

  void clear();

private:
  std::string InputFile;
  std::string OutputFile;

  std::ifstream StdIFStream;
  std::ofstream StdOFStream;

  std::vector<TokenInfo> TheTokens;
  AnalysisManager AM;
  std::unique_ptr<Program> TheProgram;
  ByteCodeModule TheModule;
  ErrorManager EM;
};

} // namespace simplecc
#endif // SIMPLECC_DRIVER_DRIVER_H
