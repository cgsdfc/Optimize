#include "SymbolTable.h"
#include "Visitor.h"
#include "error.h"
#include <algorithm>

namespace {
using namespace simplecompiler;
// Define a declaration globally.
void DefineGlobalDecl(Decl *decl, TableType &global, ErrorManager &e) {
  if (global.count(decl->name)) {
    e.NameError(decl->loc, "redefinition of identifier", Quote(decl->name),
                "in <module>");
    return;
  }
  global.emplace(decl->name, SymbolEntry(Scope::Global, decl));
}

// Define a declaration locally.
// DeclLike takes Decl or Arg
template <typename DeclLike>
void DefineLocalDecl(DeclLike *decl, TableType &local, const TableType &global,
                     ErrorManager &e, const String &funcname) {
  auto where = "in function " + Quote(funcname);
  if (local.count(decl->name)) {
    e.NameError(decl->loc, "redefinition of identifier", Quote(decl->name),
                where);
    return;
  }
  if (auto iter = global.find(decl->name);
      iter != global.end() && iter->second.IsFunction()) {
    e.NameError(decl->loc, "local identifier", Quote(decl->name), where,
                "shallows a global function name");
    return;
  }
  local.emplace(decl->name, SymbolEntry(Scope::Local, decl));
}

// Enter all global const/var declarations into dict
void MakeGlobal(Program *prog, TableType &dict, ErrorManager &e) {
  for (auto decl : prog->decls) {
    if (IsInstance<FuncDef>(decl))
      continue; // not now
    DefineGlobalDecl(decl, dict, e);
  }
}

// Visitor that resolves local names for a function
class LocalResolver : public ChildrenVisitor<LocalResolver> {
public:
  // The Ast of the function
  FuncDef *fun;
  // Global namespace to fail back
  const TableType &global;
  // Local namespace with local declarations and function arguments
  TableType &local;
  // Report errors
  ErrorManager &e;

  LocalResolver(FuncDef *fun, const TableType &global, TableType &local,
                ErrorManager &e)
      : fun(fun), global(global), local(local), e(e) {}

  // Resolve one name
  void ResolveName(const String &name, const Location &loc) {
    if (local.find(name) != local.end())
      return; // already defined locally
    // defined globally
    if (auto x = global.find(name); x != global.end()) {
      local.emplace(name, x->second);
    } else {
      e.NameError(loc, "undefined identifier", Quote(name), "in function",
                  Quote(fun->name));
    }
  }

  // pull in VisitorBase's methods
  void visitStmt(Stmt *s) { return VisitorBase::visitStmt<void>(s); }

  void visitExpr(Expr *s) { return VisitorBase::visitExpr<void>(s); }

  void visitCall(Call *x) {
    ResolveName(x->func, x->loc);
    ChildrenVisitor::visitCall(x);
  }

  // these do not have identifiers
  void visitNum(Num *x) {}
  void visitStr(Str *x) {}
  void visitChar(Char *x) {}

  void visitSubscript(Subscript *x) {
    ResolveName(x->name, x->loc);
    ChildrenVisitor::visitSubscript(x);
  }

  void visitName(Name *x) { ResolveName(x->id, x->loc); }

  // public interface
  void Resolve() {
    for (auto stmt : fun->stmts) {
      visitStmt(stmt);
    }
  }
};

// define and resolve names of a function
void MakeLocal(FuncDef *fun, TableType &top, TableType &local,
               ErrorManager &e) {
  // define fun itself in global first
  DefineGlobalDecl(fun, top, e);

  // define arguments of a function
  for (auto arg : fun->args) {
    DefineLocalDecl(arg, local, top, e, fun->name);
  }

  // define const/var declarations of a function
  auto where = "function " + Quote(fun->name);
  for (auto decl : fun->decls) {
    DefineLocalDecl(decl, local, top, e, fun->name);
  }

  // resolve local names
  LocalResolver resolver(fun, top, local, e);
  resolver.Resolve();
  // errors left in ErrorManager
}

// For each '\\' in string, make it doubled
String DoubleBackslashes(const String &string) {
  String result;
  result.reserve(string.size());
  for (auto c : string) {
    result.push_back(c);
    if (c == '\\') {
      result.push_back(c);
    }
  }
  return std::move(result);
}

// Visitor that build string table.
class StringLiteralVisitor : public ChildrenVisitor<StringLiteralVisitor> {
  StringLiteralTable &table;

public:
  StringLiteralVisitor(StringLiteralTable &table) : table(table) {}

  void visitStr(Str *node) {
    assert(node->s.size() >= 2);
    node->s = DoubleBackslashes(node->s);
    table.emplace(node->s, table.size());
  }

  void visitDecl(Decl *node) {
    if (auto x = subclass_cast<FuncDef>(node)) {
      ChildrenVisitor::visitFuncDef(x);
    }
  }

  void visitStmt(Stmt *node) { return VisitorBase::visitStmt<void>(node); }

  void visitExpr(Expr *node) { return VisitorBase::visitExpr<void>(node); }
};

void CheckTable(const TableType &table) {
  for (const auto &item : table) {
    assert(item.first == item.second.GetName());
  }
}

} // namespace

namespace simplecompiler {
// Overloads to print various data structures
std::ostream &operator<<(std::ostream &os, Scope s) {
  switch (s) {
  case Scope::Global:
    return os << "Scope::Global";
  case Scope::Local:
    return os << "Scope::Local";
  }
}

// Generic map printer
template <typename Key, typename Value>
std::ostream &operator<<(std::ostream &os,
                         const std::unordered_map<Key, Value> &t) {
  int i = 0;
  int size = t.size();

  os << "{";
  for (const auto &b : t) {
    os << b.first << ": " << b.second;
    if (i != size - 1) {
      os << ", ";
    }
    i++;
  }
  return os << "}";
}

std::ostream &operator<<(std::ostream &os, const TypeEntry &t) {
  return os << "TypeEntry(" << t.GetType() << ", " << t.GetLocation() << ")";
}

// Specialized version for String key
template <typename Value>
std::ostream &operator<<(std::ostream &os,
                         const std::unordered_map<String, Value> &t) {
  int i = 0;
  int size = t.size();

  os << "{";
  for (const auto &b : t) {
    os << Quote(b.first) << ": " << b.second;
    if (i != size - 1) {
      os << ", ";
    }
    i++;
  }
  return os << "}";
}
} // namespace simplecompiler

void SymbolEntry::Format(std::ostream &os) const {
  os << "SymbolEntry(";
  os << "type=" << GetTypeName() << ", "
     << "scope=" << GetScope() << ", "
     << "location=" << GetLocation() << ", "
     << "name=" << Quote(GetName()) << ")";
}

void SymbolTable::Format(std::ostream &os) const {
  os << "SymbolTable(";
  os << "global=" << global << ",\n"
     << "\nlocals=" << locals << ",\n"
     << "\nstring_literals=" << string_literals << ",\n"
     << "\nexpr_types=" << expr_types;
  os << ")";
}

// consistency check
void SymbolTable::Check() const {
  CheckTable(global);
  for (const auto &kv : locals) {
    CheckTable(kv.second);
    for (const auto &kv2 : kv.second) {
      assert(kv2.second.GetScope() == Scope::Local ||
             global.find(kv2.first) != global.end() &&
                 "entry in local with global scope must be present in global");
    }
  }
}

// public interface
bool SymbolTable::Build(Program *prog) {
  ErrorManager e;
  // build global table first
  MakeGlobal(prog, global, e);

  // visit all FuncDef and build their local tables
  for (auto decl : prog->decls) {
    if (auto fun = subclass_cast<FuncDef>(decl)) {
      TableType local;
      MakeLocal(fun, global, local, e);
      if (e.IsOk()) {
        locals.emplace(reinterpret_cast<uintptr_t>(fun), std::move(local));
      }
    }
  }
  if (e.IsOk()) {
    StringLiteralVisitor(string_literals).visitProgram(prog);
  }
  return e.IsOk();
}