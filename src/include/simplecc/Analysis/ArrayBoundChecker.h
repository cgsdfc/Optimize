#ifndef SIMPLECOMPILER_ARRAYBOUNDCHECKER_H
#define SIMPLECOMPILER_ARRAYBOUNDCHECKER_H
#include "simplecc/Analysis/SymbolTable.h"
#include "simplecc/Analysis/Visitor.h"
#include "simplecc/Support/ErrorManager.h"
#include <utility> // for pair

namespace simplecc {
class ArrayBoundChecker : ChildrenVisitor<ArrayBoundChecker> {
  std::pair<bool, int> getIndex(Expr *E) const;
  void visitSubscript(Subscript *SB);
  void visitFuncDef(FuncDef *FD);

public:
  ArrayBoundChecker() = default;
  bool Check(Program *P, const SymbolTable &S);

private:
  friend class ChildrenVisitor<ArrayBoundChecker>;
  friend class VisitorBase<ArrayBoundChecker>;

  const SymbolTable *TheTable = nullptr;
  SymbolTableView TheLocalTable;
  ErrorManager EM;
};
} // namespace simplecc

#endif // SIMPLECOMPILER_ARRAYBOUNDCHECKER_H