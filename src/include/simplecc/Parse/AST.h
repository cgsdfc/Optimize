#ifndef SIMPLECC_PARSE_AST_H
#define SIMPLECC_PARSE_AST_H

#include "simplecc/Lex/TokenInfo.h"

#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace simplecc {
class AST {
public:
  virtual ~AST() = default;
  virtual const char *GetClassName() const = 0;
  virtual void Format(std::ostream &os) const = 0;
};

inline std::ostream &operator<<(std::ostream &os, const AST &ast) {
  ast.Format(os);
  return os;
}

// ForwardDecl
class Program;
class Decl;
class ConstDecl;
class VarDecl;
class FuncDef;
class ArgDecl;
class Stmt;
class ReadStmt;
class WriteStmt;
class AssignStmt;
class ForStmt;
class WhileStmt;
class ReturnStmt;
class IfStmt;
class ExprStmt;
class Expr;
class BinOpExpr;
class ParenExpr;
class BoolOpExpr;
class UnaryOpExpr;
class CallExpr;
class NumExpr;
class StrExpr;
class CharExpr;
class SubscriptExpr;
class NameExpr;

// EnumClass

enum class OperatorKind { Add, Sub, Mult, Div, Eq, NotEq, Lt, LtE, Gt, GtE };

std::ostream &operator<<(std::ostream &os, OperatorKind val);

enum class UnaryopKind { UAdd, USub };

std::ostream &operator<<(std::ostream &os, UnaryopKind val);

enum class ExprContextKind { Load, Store };

std::ostream &operator<<(std::ostream &os, ExprContextKind val);

enum class BasicTypeKind { Int, Character, Void };

std::ostream &operator<<(std::ostream &os, BasicTypeKind val);

// AbstractNode

class Decl : public AST {
  int Kind;

public:
  int GetKind() const { return Kind; }
  std::string name;
  Location loc;

  Decl(int Kind, std::string name, const Location &loc)
      : AST(), Kind(Kind), name(std::move(name)), loc(loc) {}

  enum DeclKind { ConstDecl, VarDecl, FuncDef, ArgDecl };

  const std::string &getName() const { return name; }

  const Location &getLoc() const { return loc; }
};

class Stmt : public AST {
  int Kind;

public:
  int GetKind() const { return Kind; }
  Location loc;

  Stmt(int Kind, const Location &loc) : AST(), Kind(Kind), loc(loc) {}

  enum StmtKind { Read, Write, Assign, For, While, Return, If, ExprStmt };

  const Location &getLoc() const { return loc; }
};

class Expr : public AST {
  int Kind;

public:
  int GetKind() const { return Kind; }
  Location loc;

  Expr(int Kind, const Location &loc) : AST(), Kind(Kind), loc(loc) {}

  enum ExprKind {
    BinOp,
    ParenExpr,
    BoolOp,
    UnaryOp,
    Call,
    Num,
    Str,
    Char,
    Subscript,
    Name
  };

  const Location &getLoc() const { return loc; }
};

// ConcreteNode

class ConstDecl : public Decl {
public:
  BasicTypeKind type;
  Expr *value;

  ConstDecl(BasicTypeKind type, Expr *value, std::string name,
            const Location &loc)
      : Decl(Decl::ConstDecl, std::move(name), loc), type(type), value(value) {}

  // Disable copy and move.
  ConstDecl(const ConstDecl &) = delete;
  ConstDecl(ConstDecl &&) = delete;
  ConstDecl &operator=(const ConstDecl &) = delete;
  ConstDecl &operator=(ConstDecl &&) = delete;

  ~ConstDecl() override;

  const char *GetClassName() const override { return "ConstDecl"; }

  void Format(std::ostream &os) const override;

  static bool InstanceCheck(Decl *x) { return x->GetKind() == Decl::ConstDecl; }

  BasicTypeKind getType() const { return type; }
  Expr *getValue() const { return value; }
};

class VarDecl : public Decl {
public:
  BasicTypeKind type;
  int is_array;
  int size;

  VarDecl(BasicTypeKind type, int is_array, int size, std::string name,
          const Location &loc)
      : Decl(Decl::VarDecl, std::move(name), loc), type(type),
        is_array(is_array), size(size) {}

  // Disable copy and move.
  VarDecl(const VarDecl &) = delete;
  VarDecl(VarDecl &&) = delete;
  VarDecl &operator=(const VarDecl &) = delete;
  VarDecl &operator=(VarDecl &&) = delete;

  ~VarDecl() override;

  const char *GetClassName() const override { return "VarDecl"; }

  void Format(std::ostream &os) const override;

  static bool InstanceCheck(Decl *x) { return x->GetKind() == Decl::VarDecl; }

  BasicTypeKind getType() const { return type; }

  int getIsArray() const { return is_array; }

  int getSize() const { return size; }
};

class FuncDef : public Decl {
public:
  BasicTypeKind return_type;
  std::vector<Decl *> args;
  std::vector<Decl *> decls;
  std::vector<Stmt *> stmts;

  FuncDef(BasicTypeKind return_type, std::vector<Decl *> args,
          std::vector<Decl *> decls, std::vector<Stmt *> stmts,
          std::string name, const Location &loc)
      : Decl(Decl::FuncDef, std::move(name), loc), return_type(return_type),
        args(std::move(args)), decls(std::move(decls)),
        stmts(std::move(stmts)) {}

  // Disable copy and move.
  FuncDef(const FuncDef &) = delete;
  FuncDef(FuncDef &&) = delete;
  FuncDef &operator=(const FuncDef &) = delete;
  FuncDef &operator=(FuncDef &&) = delete;

  ~FuncDef() override;

  const char *GetClassName() const override { return "FuncDef"; }

  void Format(std::ostream &os) const override;

  static bool InstanceCheck(Decl *x) { return x->GetKind() == Decl::FuncDef; }

  BasicTypeKind getReturnType() const { return return_type; }

  const std::vector<Decl *> &getArgs() const { return args; }

  const std::vector<Decl *> &getDecls() const { return decls; }

  const std::vector<Stmt *> &getStmts() const { return stmts; }
};

class ArgDecl : public Decl {
public:
  BasicTypeKind type;

  ArgDecl(BasicTypeKind type, std::string name, const Location &loc)
      : Decl(Decl::ArgDecl, std::move(name), loc), type(type) {}

  // Disable copy and move.
  ArgDecl(const ArgDecl &) = delete;
  ArgDecl(ArgDecl &&) = delete;
  ArgDecl &operator=(const ArgDecl &) = delete;
  ArgDecl &operator=(ArgDecl &&) = delete;

  ~ArgDecl() override;

  const char *GetClassName() const override { return "ArgDecl"; }

  void Format(std::ostream &os) const override;

  static bool InstanceCheck(Decl *x) { return x->GetKind() == Decl::ArgDecl; }

  BasicTypeKind getType() const { return type; }
};

class ReadStmt : public Stmt {
public:
  std::vector<Expr *> names;

  ReadStmt(std::vector<Expr *> names, const Location &loc)
      : Stmt(Stmt::Read, loc), names(std::move(names)) {}

  // Disable copy and move.
  ReadStmt(const ReadStmt &) = delete;
  ReadStmt(ReadStmt &&) = delete;
  ReadStmt &operator=(const ReadStmt &) = delete;
  ReadStmt &operator=(ReadStmt &&) = delete;

  ~ReadStmt() override;

  const char *GetClassName() const override { return "ReadStmt"; }

  void Format(std::ostream &os) const override;

  static bool InstanceCheck(Stmt *x) { return x->GetKind() == Stmt::Read; }

  const std::vector<Expr *> &getNames() const { return names; }
};

class WriteStmt : public Stmt {
public:
  Expr *str;
  Expr *value;

  WriteStmt(Expr *str, Expr *value, const Location &loc)
      : Stmt(Stmt::Write, loc), str(str), value(value) {}

  // Disable copy and move.
  WriteStmt(const WriteStmt &) = delete;
  WriteStmt(WriteStmt &&) = delete;
  WriteStmt &operator=(const WriteStmt &) = delete;
  WriteStmt &operator=(WriteStmt &&) = delete;

  ~WriteStmt() override;

  const char *GetClassName() const override { return "WriteStmt"; }

  void Format(std::ostream &os) const override;

  static bool InstanceCheck(Stmt *x) { return x->GetKind() == Stmt::Write; }

  Expr *getStr() const { return str; }
  Expr *getValue() const { return value; }
};

class AssignStmt : public Stmt {
public:
  Expr *target;
  Expr *value;

  AssignStmt(Expr *target, Expr *value, const Location &loc)
      : Stmt(Stmt::Assign, loc), target(target), value(value) {}

  // Disable copy and move.
  AssignStmt(const AssignStmt &) = delete;
  AssignStmt(AssignStmt &&) = delete;
  AssignStmt &operator=(const AssignStmt &) = delete;
  AssignStmt &operator=(AssignStmt &&) = delete;

  ~AssignStmt() override;

  const char *GetClassName() const override { return "AssignStmt"; }

  void Format(std::ostream &os) const override;

  static bool InstanceCheck(Stmt *x) { return x->GetKind() == Stmt::Assign; }

  Expr *getTarget() const { return target; }
  Expr *getValue() const { return value; }
};

class ForStmt : public Stmt {
public:
  Stmt *initial;
  Expr *condition;
  Stmt *step;
  std::vector<Stmt *> body;

  ForStmt(Stmt *initial, Expr *condition, Stmt *step, std::vector<Stmt *> body,
          const Location &loc)
      : Stmt(Stmt::For, loc), initial(initial), condition(condition),
        step(step), body(std::move(body)) {}

  // Disable copy and move.
  ForStmt(const ForStmt &) = delete;
  ForStmt(ForStmt &&) = delete;
  ForStmt &operator=(const ForStmt &) = delete;
  ForStmt &operator=(ForStmt &&) = delete;

  ~ForStmt() override;

  const char *GetClassName() const override { return "ForStmt"; }

  void Format(std::ostream &os) const override;

  static bool InstanceCheck(Stmt *x) { return x->GetKind() == Stmt::For; }

  Stmt *getInitial() const { return initial; }
  Expr *getCondition() const { return condition; }
  Stmt *getStep() const { return step; }

  const std::vector<Stmt *> &getBody() const { return body; }
};

class WhileStmt : public Stmt {
public:
  Expr *condition;
  std::vector<Stmt *> body;

  WhileStmt(Expr *condition, std::vector<Stmt *> body, const Location &loc)
      : Stmt(Stmt::While, loc), condition(condition), body(std::move(body)) {}

  // Disable copy and move.
  WhileStmt(const WhileStmt &) = delete;
  WhileStmt(WhileStmt &&) = delete;
  WhileStmt &operator=(const WhileStmt &) = delete;
  WhileStmt &operator=(WhileStmt &&) = delete;

  ~WhileStmt() override;

  const char *GetClassName() const override { return "WhileStmt"; }

  void Format(std::ostream &os) const override;

  static bool InstanceCheck(Stmt *x) { return x->GetKind() == Stmt::While; }

  Expr *getCondition() const { return condition; }

  const std::vector<Stmt *> &getBody() const { return body; }
};

class ReturnStmt : public Stmt {
public:
  Expr *value;

  ReturnStmt(Expr *value, const Location &loc)
      : Stmt(Stmt::Return, loc), value(value) {}

  // Disable copy and move.
  ReturnStmt(const ReturnStmt &) = delete;
  ReturnStmt(ReturnStmt &&) = delete;
  ReturnStmt &operator=(const ReturnStmt &) = delete;
  ReturnStmt &operator=(ReturnStmt &&) = delete;

  ~ReturnStmt() override;

  const char *GetClassName() const override { return "ReturnStmt"; }

  void Format(std::ostream &os) const override;

  static bool InstanceCheck(Stmt *x) { return x->GetKind() == Stmt::Return; }

  Expr *getValue() const { return value; }
};

class IfStmt : public Stmt {
public:
  Expr *test;
  std::vector<Stmt *> body;
  std::vector<Stmt *> orelse;

  IfStmt(Expr *test, std::vector<Stmt *> body, std::vector<Stmt *> orelse,
         const Location &loc)
      : Stmt(Stmt::If, loc), test(test), body(std::move(body)),
        orelse(std::move(orelse)) {}

  // Disable copy and move.
  IfStmt(const IfStmt &) = delete;
  IfStmt(IfStmt &&) = delete;
  IfStmt &operator=(const IfStmt &) = delete;
  IfStmt &operator=(IfStmt &&) = delete;

  ~IfStmt() override;

  const char *GetClassName() const override { return "IfStmt"; }

  void Format(std::ostream &os) const override;

  static bool InstanceCheck(Stmt *x) { return x->GetKind() == Stmt::If; }

  Expr *getTest() const { return test; }

  const std::vector<Stmt *> &getBody() const { return body; }

  const std::vector<Stmt *> &getOrelse() const { return orelse; }
};

class ExprStmt : public Stmt {
public:
  Expr *value;

  ExprStmt(Expr *value, const Location &loc)
      : Stmt(Stmt::ExprStmt, loc), value(value) {}

  // Disable copy and move.
  ExprStmt(const ExprStmt &) = delete;
  ExprStmt(ExprStmt &&) = delete;
  ExprStmt &operator=(const ExprStmt &) = delete;
  ExprStmt &operator=(ExprStmt &&) = delete;

  ~ExprStmt() override;

  const char *GetClassName() const override { return "ExprStmt"; }

  void Format(std::ostream &os) const override;

  static bool InstanceCheck(Stmt *x) { return x->GetKind() == Stmt::ExprStmt; }

  Expr *getValue() const { return value; }
};

class BinOpExpr : public Expr {
public:
  Expr *left;
  OperatorKind op;
  Expr *right;

  BinOpExpr(Expr *left, OperatorKind op, Expr *right, const Location &loc)
      : Expr(Expr::BinOp, loc), left(left), op(op), right(right) {}

  // Disable copy and move.
  BinOpExpr(const BinOpExpr &) = delete;
  BinOpExpr(BinOpExpr &&) = delete;
  BinOpExpr &operator=(const BinOpExpr &) = delete;
  BinOpExpr &operator=(BinOpExpr &&) = delete;

  ~BinOpExpr() override;

  const char *GetClassName() const override { return "BinOpExpr"; }

  void Format(std::ostream &os) const override;

  static bool InstanceCheck(Expr *x) { return x->GetKind() == Expr::BinOp; }

  Expr *getLeft() const { return left; }
  OperatorKind getOp() const { return op; }
  Expr *getRight() const { return right; }
};

class ParenExpr : public Expr {
public:
  Expr *value;

  ParenExpr(Expr *value, const Location &loc)
      : Expr(Expr::ParenExpr, loc), value(value) {}

  // Disable copy and move.
  ParenExpr(const ParenExpr &) = delete;
  ParenExpr(ParenExpr &&) = delete;
  ParenExpr &operator=(const ParenExpr &) = delete;
  ParenExpr &operator=(ParenExpr &&) = delete;

  ~ParenExpr() override;

  const char *GetClassName() const override { return "ParenExpr"; }

  void Format(std::ostream &os) const override;

  static bool InstanceCheck(Expr *x) { return x->GetKind() == Expr::ParenExpr; }

  Expr *getValue() const { return value; }
};

class BoolOpExpr : public Expr {
public:
  Expr *value;
  int has_cmpop;

  BoolOpExpr(Expr *value, int has_cmpop, const Location &loc)
      : Expr(Expr::BoolOp, loc), value(value), has_cmpop(has_cmpop) {}

  // Disable copy and move.
  BoolOpExpr(const BoolOpExpr &) = delete;
  BoolOpExpr(BoolOpExpr &&) = delete;
  BoolOpExpr &operator=(const BoolOpExpr &) = delete;
  BoolOpExpr &operator=(BoolOpExpr &&) = delete;

  ~BoolOpExpr() override;

  const char *GetClassName() const override { return "BoolOpExpr"; }

  void Format(std::ostream &os) const override;

  static bool InstanceCheck(Expr *x) { return x->GetKind() == Expr::BoolOp; }

  Expr *getValue() const { return value; }

  int getHasCmpop() const { return has_cmpop; }
};

class UnaryOpExpr : public Expr {
public:
  UnaryopKind op;
  Expr *operand;

  UnaryOpExpr(UnaryopKind op, Expr *operand, const Location &loc)
      : Expr(Expr::UnaryOp, loc), op(op), operand(operand) {}

  // Disable copy and move.
  UnaryOpExpr(const UnaryOpExpr &) = delete;
  UnaryOpExpr(UnaryOpExpr &&) = delete;
  UnaryOpExpr &operator=(const UnaryOpExpr &) = delete;
  UnaryOpExpr &operator=(UnaryOpExpr &&) = delete;

  ~UnaryOpExpr() override;

  const char *GetClassName() const override { return "UnaryOpExpr"; }

  void Format(std::ostream &os) const override;

  static bool InstanceCheck(Expr *x) { return x->GetKind() == Expr::UnaryOp; }

  UnaryopKind getOp() const { return op; }
  Expr *getOperand() const { return operand; }
};

class CallExpr : public Expr {
public:
  std::string func;
  std::vector<Expr *> args;

  CallExpr(std::string func, std::vector<Expr *> args, const Location &loc)
      : Expr(Expr::Call, loc), func(std::move(func)), args(std::move(args)) {}

  // Disable copy and move.
  CallExpr(const CallExpr &) = delete;
  CallExpr(CallExpr &&) = delete;
  CallExpr &operator=(const CallExpr &) = delete;
  CallExpr &operator=(CallExpr &&) = delete;

  ~CallExpr() override;

  const char *GetClassName() const override { return "CallExpr"; }

  void Format(std::ostream &os) const override;

  static bool InstanceCheck(Expr *x) { return x->GetKind() == Expr::Call; }

  const std::string &getFunc() const { return func; }

  const std::vector<Expr *> &getArgs() const { return args; }
};

class NumExpr : public Expr {
public:
  int n;

  NumExpr(int n, const Location &loc) : Expr(Expr::Num, loc), n(n) {}

  // Disable copy and move.
  NumExpr(const NumExpr &) = delete;
  NumExpr(NumExpr &&) = delete;
  NumExpr &operator=(const NumExpr &) = delete;
  NumExpr &operator=(NumExpr &&) = delete;

  ~NumExpr() override;

  const char *GetClassName() const override { return "NumExpr"; }

  void Format(std::ostream &os) const override;

  static bool InstanceCheck(Expr *x) { return x->GetKind() == Expr::Num; }

  int getN() const { return n; }
};

class StrExpr : public Expr {
public:
  std::string s;

  StrExpr(std::string s, const Location &loc)
      : Expr(Expr::Str, loc), s(std::move(s)) {}

  // Disable copy and move.
  StrExpr(const StrExpr &) = delete;
  StrExpr(StrExpr &&) = delete;
  StrExpr &operator=(const StrExpr &) = delete;
  StrExpr &operator=(StrExpr &&) = delete;

  ~StrExpr() override;

  const char *GetClassName() const override { return "StrExpr"; }

  void Format(std::ostream &os) const override;

  static bool InstanceCheck(Expr *x) { return x->GetKind() == Expr::Str; }

  const std::string &getS() const { return s; }
};

class CharExpr : public Expr {
public:
  int c;

  CharExpr(int c, const Location &loc) : Expr(Expr::Char, loc), c(c) {}

  // Disable copy and move.
  CharExpr(const CharExpr &) = delete;
  CharExpr(CharExpr &&) = delete;
  CharExpr &operator=(const CharExpr &) = delete;
  CharExpr &operator=(CharExpr &&) = delete;

  ~CharExpr() override;

  const char *GetClassName() const override { return "CharExpr"; }

  void Format(std::ostream &os) const override;

  static bool InstanceCheck(Expr *x) { return x->GetKind() == Expr::Char; }

  int getC() const { return c; }
};

class SubscriptExpr : public Expr {
public:
  std::string name;
  Expr *index;
  ExprContextKind ctx;

  SubscriptExpr(std::string name, Expr *index, ExprContextKind ctx,
                const Location &loc)
      : Expr(Expr::Subscript, loc), name(std::move(name)), index(index),
        ctx(ctx) {}

  // Disable copy and move.
  SubscriptExpr(const SubscriptExpr &) = delete;
  SubscriptExpr(SubscriptExpr &&) = delete;
  SubscriptExpr &operator=(const SubscriptExpr &) = delete;
  SubscriptExpr &operator=(SubscriptExpr &&) = delete;

  ~SubscriptExpr() override;

  const char *GetClassName() const override { return "SubscriptExpr"; }

  void Format(std::ostream &os) const override;

  static bool InstanceCheck(Expr *x) { return x->GetKind() == Expr::Subscript; }

  const std::string &getName() const { return name; }
  Expr *getIndex() const { return index; }
  ExprContextKind getCtx() const { return ctx; }
};

class NameExpr : public Expr {
public:
  std::string id;
  ExprContextKind ctx;

  NameExpr(std::string id, ExprContextKind ctx, const Location &loc)
      : Expr(Expr::Name, loc), id(std::move(id)), ctx(ctx) {}

  // Disable copy and move.
  NameExpr(const NameExpr &) = delete;
  NameExpr(NameExpr &&) = delete;
  NameExpr &operator=(const NameExpr &) = delete;
  NameExpr &operator=(NameExpr &&) = delete;

  ~NameExpr() override;

  const char *GetClassName() const override { return "NameExpr"; }

  void Format(std::ostream &os) const override;

  static bool InstanceCheck(Expr *x) { return x->GetKind() == Expr::Name; }

  const std::string &getId() const { return id; }
  ExprContextKind getCtx() const { return ctx; }
};

// LeafNode

class Program : public AST {
public:
  std::vector<Decl *> decls;

  explicit Program(std::vector<Decl *> decls)
      : AST(), decls(std::move(decls)) {}

  // Disable copy and move.
  Program(const Program &) = delete;
  Program(Program &&) = delete;
  Program &operator=(const Program &) = delete;
  Program &operator=(Program &&) = delete;

  ~Program() override;

  const char *GetClassName() const override { return "Program"; }

  void Format(std::ostream &os) const override;

  const std::vector<Decl *> &getDecls() const { return decls; }
};

// EnumFromString
OperatorKind OperatorKindFromString(const String &s);
const char *CStringFromOperatorKind(OperatorKind val);

UnaryopKind UnaryopKindFromString(const String &s);
const char *CStringFromUnaryopKind(UnaryopKind val);

BasicTypeKind BasicTypeKindFromString(const String &s);
const char *CStringFromBasicTypeKind(BasicTypeKind val);

} // namespace simplecc
#endif
