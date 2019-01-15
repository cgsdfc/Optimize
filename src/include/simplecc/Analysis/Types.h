/// @file This file defines the type system of simplecc.
#ifndef SIMPLECC_ANALYSIS_TYPES_H
#define SIMPLECC_ANALYSIS_TYPES_H
#include "simplecc/Parse/AST.h"
#include "simplecc/Support/Macros.h"
#include <cassert>
#include <string>

namespace simplecc {

/// Two level scope.
enum class Scope { Global, Local };
/// Output operator for Scope.
std::ostream &operator<<(std::ostream &os, Scope s);

/// The function type.
/// Function can only be called.
/// Internally, this is a wrapper around a FuncDef node and a view on its
/// signature. It throws aways function name, argument names and the body.
/// It only keeps the return type and the parameter types.
class FuncType {
  const FuncDef *TheFuncDef;

public:
  explicit FuncType(const FuncDef *FD) : TheFuncDef(FD) {}
  BasicTypeKind getReturnType() const { return TheFuncDef->getReturnType(); }
  BasicTypeKind getArgTypeAt(int Idx) const;
  size_t getNumArgs() const { return TheFuncDef->getNumArgs(); }
};

/// The variable type.
/// Variable can be:
/// 1. assigned to.
/// 3. read value.
class VarType {
  BasicTypeKind Type;

public:
  explicit VarType(BasicTypeKind Ty) : Type(Ty) {}
  explicit VarType(const DeclAST *D);
  BasicTypeKind getType() const { return Type; }
};

/// The array type.
/// Array can only be taken subscript.
class ArrayType {
  BasicTypeKind ElemType;
  unsigned Size;

public:
  explicit ArrayType(const VarDecl *VD);
  BasicTypeKind getElementType() const { return ElemType; }
  unsigned getSize() const { return Size; }
};

/// The constant type.
/// Constant can only be read value.
class ConstType {
  int Value;
  BasicTypeKind Type;

public:
  explicit ConstType(const ConstDecl *CD);
  int getValue() const { return Value; }
  BasicTypeKind getType() const { return Type; }
};

/// An entry in the SymbolTable with type and scope information about
/// a name within a block (global or local).
class SymbolEntry {
  Scope TheScope;
  const DeclAST *TheDecl = nullptr;

public:
  SymbolEntry(Scope scope, const DeclAST *decl);
  /// This constructs an invalid SymbolEntry.
  SymbolEntry() = default;

  /// Return if the type is FuncType.
  bool IsFunction() const;
  FuncType AsFunction() const;

  /// Return if the type is ArrayType.
  bool IsArray() const;
  /// Return the ArrayType.
  ArrayType AsArray() const;

  /// Return if it is a formal argument.
  bool IsFormalArgument() const;
  /// Return if it is a VarType.
  bool IsVariable() const;
  /// Return the VarType.
  VarType AsVariable() const;

  /// Return if the type is ConstType.
  bool IsConstant() const;
  /// Return the ConstType.
  ConstType AsConstant() const;

  /// Return the typename of this name.
  const char *getTypeName() const;
  /// Return the location where this name is declared.
  Location getLocation() const;
  /// Return the value of this name.
  const std::string &getName() const;

  /// Return the Scope of this name.
  Scope getScope() const { return TheScope; }
  /// Return if this is a global symbol.
  bool IsGlobal() const { return Scope::Global == getScope(); }
  /// Return if this is a local symbol.
  bool IsLocal() const { return Scope::Local == getScope(); }

  void Format(std::ostream &os) const;
};

DEFINE_INLINE_OUTPUT_OPERATOR(SymbolEntry)

} // namespace simplecc
#endif // SIMPLECC_ANALYSIS_TYPES_H