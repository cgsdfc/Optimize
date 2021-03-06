#include "simplecc/CodeGen/ByteCodeModule.h"
#include "simplecc/CodeGen/ByteCodeCompiler.h"
#include "simplecc/CodeGen/ByteCodeFunction.h"
#include <algorithm> // for_each
#include <iomanip> // setw
#include <memory> // default_delete

using namespace simplecc;

void ByteCodeModule::clear() {
  // Delete the function first.
  std::for_each(begin(), end(), std::default_delete<ByteCodeFunction>());
  FunctionList.clear();
  StringLiterals.clear();
  GlobalVariables.clear();
}

ByteCodeModule::~ByteCodeModule() {
  /// Delete all owned functions.
  std::for_each(begin(), end(), std::default_delete<ByteCodeFunction>());
}

unsigned ByteCodeModule::getStringLiteralID(const std::string &Str) {
  auto ID = static_cast<unsigned int>(StringLiterals.size());
  return StringLiterals.emplace(Str, ID).first->second;
}

// TODO: make the printing better (currently like a shut).
void ByteCodeModule::Format(std::ostream &O) const {
  for (const SymbolEntry &GV : getGlobalVariables()) {
    O << GV << "\n";
  }

  O << "\n";
  for (const auto &Pair : getStringLiteralTable()) {
    O << std::setw(4) << Pair.second << ": " << Pair.first << "\n";
  }

  O << "\n";
  for (const ByteCodeFunction *Fn : *this) {
    O << *Fn << "\n";
  }
}