#ifndef SIMPLECC_CODEGEN_BYTECODEPRINTER_H
#define SIMPLECC_CODEGEN_BYTECODEPRINTER_H
#include "simplecc/Analysis/Visitor.h"
#include "simplecc/Support/Print.h"

#include <cassert>
#include <iostream>

namespace simplecc {
/// This class handles the formatting of the result of an expression
/// It is a discriminated union of a factor Expr and a temporary.
class ExprValue {
  ExprAST *Factor;
  int Temporary;
  bool Check() const;

public:
  ExprValue() = default;
  ~ExprValue() = default;

  ExprValue(const ExprValue &) = default;
  ExprValue &operator=(const ExprValue &) = default;

  explicit ExprValue(int Temp);
  explicit ExprValue(ExprAST *Factor);

  void Format(std::ostream &O) const;
};

inline std::ostream &operator<<(std::ostream &O, const ExprValue &Val) {
  Val.Format(O);
  return O;
}

/// This class handles the formatting of a label of two form:
/// inline form like GOTO Label_1 and non-inline form like:
/// Label_1:
/// printf t0
class LineLabel {
  /// No of this label
  unsigned No;
  /// Which form it takes
  bool IsInline = false;

public:
  explicit LineLabel(unsigned No) : No(No) {}
  LineLabel(const LineLabel &) = default;
  LineLabel &operator=(const LineLabel &) = default;

  /// Setter of inline, for use in operator<<()
  LineLabel &Inline(bool B) {
    this->IsInline = B;
    return *this;
  }
  void Format(std::ostream &O) const;
};

inline std::ostream &operator<<(std::ostream &O, const LineLabel &L) {
  L.Format(O);
  return O;
}

/// This class prints a program in the form required by the school.
class ByteCodePrinter : ChildrenVisitor<ByteCodePrinter> {
  void visitConstDecl(ConstDecl *CD);
  void visitVarDecl(VarDecl *VD);
  void visitArgDecl(ArgDecl *AD);
  void visitFuncDef(FuncDef *FD);
  void visitRead(ReadStmt *RD);

  void visitWrite(WriteStmt *WR);
  void visitAssign(AssignStmt *A);
  void visitReturn(ReturnStmt *R);
  ExprValue visitCall(CallExpr *C);
  LineLabel CompileBoolOp(BoolOpExpr *B);
  void visitFor(ForStmt *F);
  void visitIf(IfStmt *I);
  void visitWhile(WhileStmt *W);
  ExprValue visitBinOp(BinOpExpr *B);
  ExprValue visitUnaryOp(UnaryOpExpr *U);
  ExprValue visitSubscript(SubscriptExpr *SB);

  ExprValue visitExpr(ExprAST *E) {
    return ChildrenVisitor::visitExpr<ExprValue>(E);
  }
  ExprValue visitParenExpr(ParenExpr *PE) { return visitExpr(PE->getValue()); }
  ExprValue visitName(NameExpr *N) { return ExprValue(N); }
  ExprValue visitChar(CharExpr *C) { return ExprValue(C); }
  ExprValue visitNum(NumExpr *N) { return ExprValue(N); }
  ExprValue visitStr(StrExpr *S) { return ExprValue(S); }

  ExprValue visitBoolOp(BoolOpExpr *) {
    assert(false && "BoolOpExpr should be handled by CompileBoolOp()");
    return ExprValue();
  }

  ExprValue MakeTemporary() { return ExprValue(TempCounter++); }
  LineLabel MakeLineLabel() { return LineLabel(LabelCounter++); }
  unsigned getTempCounter() const { return TempCounter; }

public:
  explicit ByteCodePrinter(std::ostream &O) : w(O) {}
  ~ByteCodePrinter() = default;

  void PrintByteCode(Program *P) { visitProgram(P); }

private:
  friend class ChildrenVisitor<ByteCodePrinter>;
  friend class VisitorBase<ByteCodePrinter>;

  Printer w;
  unsigned TempCounter = 0;
  unsigned LabelCounter = 0;
};

} // namespace simplecc
#endif
