#include "CompilerInstance.h"
#include "ByteCodePrinter.h"
#include "Node.h"
#include "assemble.h"
#include "compile.h"
#include "cst.h"
#include "error.h"
#include "parser.h"
#include "syntax_check.h"
#include "type_check.h"

#include "CSTGraph.h"
#include "ASTGraph.h"

#include <llvm/Support/raw_ostream.h>

using namespace simplecompiler;
CompilerInstance::CompilerInstance(std::istream &input, std::ostream &output)
    : input(input), output(output), Options(), tokens(), symbolTable(),
      cst_node(nullptr), ast_node(nullptr) {}

CompilerInstance::~CompilerInstance() {
  delete cst_node;
  delete ast_node;
}

bool CompilerInstance::Invoke() {
  CompilationPhrase phrase = Options.getPhrase();
  Tokenize(input, tokens);
  if (phrase == CompilationPhrase::Tokenize) {
    PrintTokens(tokens, output);
    return true;
  }

  cst_node = ParseTokens(tokens);
  if (!cst_node) {
    return false;
  }
  if (phrase == CompilationPhrase::BuildCst) {
    switch (Options.getOutputFormat()) {
    case OutputFormat::DOT:
      WriteCSTGraph(cst_node, llvm::outs());
      return true;
    case OutputFormat::RawDump:
      output << *cst_node << "\n";
      return true;
    }
  }

  ast_node = NodeToAst(cst_node);
  if (!ast_node) {
    return false;
  }

  if (phrase == CompilationPhrase::BuildAst) {
    PrintAllAstNodes(*ast_node, output);
    /* output << *ast_node << "\n"; */
    return true;
  }

  if (!CheckSyntax(ast_node)) {
    return false;
  }
  if (phrase == CompilationPhrase::SyntaxCheck) {
    return true;
  }

  if (!symbolTable.Build(ast_node)) {
    return false;
  }
  symbolTable.Check();
  if (phrase == CompilationPhrase::BuildSymbolTable) {
    output << symbolTable << "\n";
    return true;
  }

  if (!CheckType(ast_node, symbolTable)) {
    return false;
  }
  if (phrase == CompilationPhrase::TypeCheck) {
    output << symbolTable << "\n";
    return true;
  }

  if (phrase == CompilationPhrase::PrintByteCode) {
    PrintByteCode(ast_node, output);
    return true;
  }

  auto &&module = CompileProgram(ast_node, symbolTable);
  if (phrase == CompilationPhrase::Compile) {
    output << module << "\n";
    return true;
  }

  if (phrase == CompilationPhrase::Assemble) {
    AssembleMips(module, output);
    return true;
  }

  return false;
}