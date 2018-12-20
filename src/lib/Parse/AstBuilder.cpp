#include "simplecc/Parse/AstBuilder.h"
#include "simplecc/Parse/AST.h"
#include "simplecc/Support/ErrorManager.h"
#include "simplecc/Parse/Node.h"

#include <cassert>
#include <sstream>
#include <string>
#include <vector>

using namespace simplecc;

Program *AstBuilder::visit_program(Node *N) {
  assert(N->getType() == Symbol::program);
  std::vector<Decl *> Decls;

  for (auto C : N->getChildren()) {
    if (C->getType() == Symbol::const_decl) {
      visit_const_decl(C, Decls);
    } else if (C->getType() == Symbol::declaration) {
      visit_declaration(C, Decls);
    } else {
      assert(C->getType() == Symbol::ENDMARKER);
      break;
    }
  }
  return new Program(Decls);
}

void AstBuilder::visit_const_decl(Node *N, std::vector<Decl *> &Decls) {
  auto TypeName = N->getChild(1);
  auto Ty = visit_type_name(TypeName);

  for (int i = 2, len = N->getNumChildren() - 1; i < len; i += 2) {
    auto Child = N->getChild(i);
    Decls.push_back(visit_const_item(Child, Ty));
  }
}

Decl *AstBuilder::visit_const_item(Node *N, BasicTypeKind Ty) {
  auto name = N->FirstChild();
  auto konstant = N->getChild(2);
  Expr *val;

  if (konstant->getType() == Symbol::CHAR) {
    val = makeChar(konstant);
  } else {
    assert(konstant->getType() == Symbol::integer);
    val = new Num(visit_integer(konstant), konstant->getLocation());
  }
  return new ConstDecl(Ty, val, name->getValue(), name->getLocation());
}

int AstBuilder::visit_integer(Node *N) {
  std::ostringstream os;
  for (auto C : N->getChildren()) {
    os << C->getValue();
  }
  return std::stoi(os.str());
}

void AstBuilder::visit_declaration(Node *N, std::vector<Decl *> &Decls) {
  auto TypeName = N->FirstChild();
  auto name = N->getChild(1);
  if (name->getValue() == "main") {
    std::vector<Decl *> FnDecls;
    std::vector<Stmt *> FnStmts;

    auto Ty = visit_type_name(TypeName);
    visit_compound_stmt(N->LastChild(), FnDecls, FnStmts);
    Decls.push_back(new FuncDef(Ty, {}, FnDecls, FnStmts, "main",
                                TypeName->getLocation()));
  } else {
    auto decl_trailer = N->LastChild();
    assert(decl_trailer->getType() == Symbol::decl_trailer);
    visit_decl_trailer(decl_trailer, TypeName, name, Decls);
  }
}

std::vector<Expr *> AstBuilder::visit_arglist(Node *N) {

  std::vector<Expr *> Args;
  for (auto C : N->getChildren()) {
    if (C->getType() == Symbol::expr) {
      Args.push_back(visit_expr(C));
    }
  }
  return std::move(Args);
}

Expr *AstBuilder::visit_atom_trailer(Node *N, const String &Name,
                                     ExprContextKind Context) {

  auto first = N->FirstChild();
  if (first->getType() == Symbol::arglist) {
    // no empty arglist
    std::vector<Expr *> Args = visit_arglist(first);
    return new Call(Name, Args, N->getLocation());
  } else {
    assert(first->getValue() == "[");
    auto index = visit_expr(N->getChild(1));
    return new Subscript(Name, index, Context, N->getLocation());
  }
}

Expr *AstBuilder::visit_atom(Node *N, ExprContextKind Context) {

  auto first = N->FirstChild();
  if (first->getType() == Symbol::NAME) {
    if (N->getNumChildren() == 1) {
      // single name
      return new Name(first->getValue(), Context, first->getLocation());
    }
    // name with trailer: visit_trailer
    auto trailer = N->getChild(1);
    return visit_atom_trailer(trailer, first->getValue(), Context);
  }
  if (first->getType() == Symbol::NUMBER) {
    return makeNum(first);
  }
  if (first->getType() == Symbol::CHAR) {
    return makeChar(first);
  } else {
    assert(first->getValue() == "(");
    auto value = visit_expr(N->getChild(1));
    return new ParenExpr(value, first->getLocation());
  }
}

Stmt *AstBuilder::visit_write_stmt(Node *N) {
  Expr *S = nullptr;
  Expr *E = nullptr;

  for (auto C : N->getChildren()) {
    if (C->getType() == Symbol::expr)
      E = visit_expr(C);
    else if (C->getType() == Symbol::STRING)
      S = new Str(C->getValue(), C->getLocation());
    // ignore other things
  }
  return new Write(S, E, N->getLocation());
}

void AstBuilder::visit_decl_trailer(Node *N, Node *TypeName, Node *Name,
                                    std::vector<Decl *> &Decls) {

  auto first = N->FirstChild();
  auto Ty = visit_type_name(TypeName);

  if (first->getValue() == ";") {
    Decls.push_back(new VarDecl(Ty, false, 0, Name->getValue(),
                                TypeName->getLocation()));
  } else if (first->getType() == Symbol::paralist ||
      first->getType() == Symbol::compound_stmt) {
    visit_funcdef(Ty, Name->getValue(), N, TypeName->getLocation(),
                  Decls);
  } else {
    if (first->getType() == Symbol::subscript2) {
      auto size = visit_subscript2(first);
      Decls.push_back(
          new VarDecl(Ty, true, size, Name->getValue(), N->getLocation()));
    } else {
      Decls.push_back(
          new VarDecl(Ty, false, 0, Name->getValue(), N->getLocation()));
    }

    for (auto C : N->getChildren()) {
      if (C->getType() != Symbol::var_item)
        continue;
      Decls.push_back(visit_var_item(C, Ty));
    }
  }
}

Stmt *AstBuilder::visit_return_stmt(Node *N) {
  if (N->getNumChildren() == 1)
    return new Return(nullptr, N->getLocation());

  auto expr = visit_expr(N->getChild(2));
  return new Return(expr, N->getLocation());
}

void AstBuilder::visit_stmt(Node *N, std::vector<Stmt *> &Stmts) {

  auto first = N->FirstChild();
  if (first->getType() == Symbol::flow_stmt) {
    Stmts.push_back(visit_flow_stmt(first));
  } else if (first->getType() == Symbol::NAME) {
    if (N->getNumChildren() == 2) {
      auto call = new Call(first->getValue(), {}, first->getLocation());
      Stmts.push_back(new ExprStmt(call, N->getLocation()));
    } else {
      Stmts.push_back(visit_stmt_trailer(N->getChild(1), first));
    }
  } else if (first->getValue() == "{") {
    for (auto C : N->getChildren()) {
      if (C->getType() == Symbol::stmt) {
        visit_stmt(C, Stmts);
      }
    }
  } else {
    // discard the empty stmt -- ';'
    assert(first->getValue() == ";" && N->getNumChildren() == 1);
  }
}

Stmt *AstBuilder::visit_flow_stmt(Node *N) {
  auto first = N->FirstChild();
  auto first_type = first->getType();
  if (first_type == Symbol::if_stmt)
    return visit_if_stmt(first);
  if (first_type == Symbol::for_stmt)
    return visit_for_stmt(first);
  if (first_type == Symbol::while_stmt)
    return visit_while_stmt(first);
  if (first_type == Symbol::return_stmt)
    return visit_return_stmt(first);
  if (first_type == Symbol::read_stmt)
    return visit_read_stmt(first);
  if (first_type == Symbol::write_stmt)
    return visit_write_stmt(first);
  assert(false && "should handle all flow_stmt");
}

Stmt *AstBuilder::visit_if_stmt(Node *N) {
  auto condition = N->getChild(2);
  auto stmt = N->getChild(4);
  auto test = visit_condition(condition);
  std::vector<Stmt *> body, orelse;
  visit_stmt(stmt, body);
  if (N->getNumChildren() > 5)
    visit_stmt(N->LastChild(), orelse);
  return new If(test, body, orelse, N->getLocation());
}

Expr *AstBuilder::visit_binop(Node *N, ExprContextKind Context) {
  auto result = visit_expr(N->FirstChild(), Context);
  auto nops = (N->getNumChildren() - 1) / 2;

  for (int i = 0; i < nops; i++) {
    auto next_oper = N->getChild(i * 2 + 1);
    auto op = OperatorKindFromString(next_oper->getValue());
    auto tmp = visit_expr(N->getChild(i * 2 + 2), Context);
    auto tmp_result = new BinOp(result, op, tmp, next_oper->getLocation());
    result = tmp_result;
  }
  return result;
}

void AstBuilder::visit_funcdef(BasicTypeKind RetTy, const String &Name,
                               Node *decl_trailer, const Location &location,
                               std::vector<Decl *> &Decls) {
  std::vector<Decl *> ParamList;
  std::vector<Decl *> FnDecls;
  std::vector<Stmt *> FnStmts;

  if (decl_trailer->getNumChildren() > 1) {
    visit_paralist(decl_trailer->FirstChild(), ParamList);
  }

  visit_compound_stmt(decl_trailer->LastChild(), FnDecls, FnStmts);
  Decls.push_back(
      new FuncDef(RetTy, ParamList, FnDecls, FnStmts, Name, location));
}

Expr *AstBuilder::visit_condition(Node *N) {
  bool has_cmpop = N->getNumChildren() == 3;
  return new BoolOp(visit_expr(N), has_cmpop, N->getLocation());
}

Stmt *AstBuilder::visit_for_stmt(Node *N) {
  // initial: stmt
  auto Nn = N->getChild(2);
  auto expr = N->getChild(4);
  auto Initial = new Assign(
      /* target */ new Name(Nn->getValue(), ExprContextKind::Store, Nn->getLocation()),
      /* value */ visit_expr(expr), /* loc */ Nn->getLocation());

  // condition: expr
  auto Cond = visit_condition(N->getChild(6));

  // step: stmt
  auto target = N->getChild(8);
  auto name2 = N->getChild(10);
  auto op = N->getChild(11);
  auto num = N->getChild(12);
  assert(num->getType() == Symbol::NUMBER);
  auto L =
      new class Name(name2->getValue(), ExprContextKind::Load, name2->getLocation());
  auto R = makeNum(num);
  auto BO = new BinOp(/* left */ L, /* op */ OperatorKindFromString(op->getValue()), /* right */ R,
                                 name2->getLocation());
  auto Step = new Assign(/* target */ new Name(target->getValue(), ExprContextKind::Store,
                                               target->getLocation()),
      /* value */ BO, /* loc */ target->getLocation());

  // body: stmt*
  std::vector<Stmt *> Body;
  visit_stmt(N->LastChild(), Body);
  return new For(Initial, Cond, Step, Body, N->getLocation());
}

void AstBuilder::visit_paralist(Node *N, std::vector<Decl *> &ParamList) {

  int n_items = (N->getNumChildren() - 1) / 3;

  for (int i = 0; i < n_items; i++) {
    auto TypeName = N->getChild(1 + i * 3);
    auto Name = N->getChild(2 + i * 3);

    ParamList.push_back(new ArgDecl(/* type */ visit_type_name(TypeName), /* name */ Name->getValue(),
                                               TypeName->getLocation()));
  }
}

Expr *AstBuilder::visit_factor(Node *N, ExprContextKind Context) {

  if (N->getNumChildren() == 1) {
    return visit_atom(N->FirstChild(), Context);

  } else {
    auto first = N->FirstChild();
    auto op = UnaryopKindFromString(first->getValue());
    auto operand = visit_factor(N->getChild(1), Context);
    return new UnaryOp(op, operand, first->getLocation());
  }
}

Stmt *AstBuilder::visit_stmt_trailer(Node *N, Node *Name) {

  auto first = N->FirstChild();
  if (first->getType() == Symbol::arglist) {
    std::vector<Expr *> Args = visit_arglist(first);
    auto C = new Call(Name->getValue(), Args, Name->getLocation());
    return new ExprStmt(C, Name->getLocation());

  } else if (first->getValue() == "[") {
    auto Idx = visit_expr(N->getChild(1));
    auto Val = visit_expr(N->LastChild());
    auto SB = new Subscript(Name->getValue(), Idx,
                            ExprContextKind::Store, N->getLocation());
    return new Assign(SB, Val, Name->getLocation());

  } else {
    assert(first->getValue() == "=");
    auto Val = visit_expr(N->LastChild());
    auto Target =
        new class Name(Name->getValue(), ExprContextKind::Store, Name->getLocation());
    return new Assign(Target, Val, Name->getLocation());
  }
}

void AstBuilder::visit_compound_stmt(Node *N, std::vector<Decl *> &FnDecls,
                                     std::vector<Stmt *> &FnStmts) {

  for (auto C : N->getChildren()) {
    if (C->getType() == Symbol::const_decl) {
      visit_const_decl(C, FnDecls);
    } else if (C->getType() == Symbol::var_decl) {
      visit_var_decl(C, FnDecls);
    } else if (C->getType() == Symbol::stmt) {
      visit_stmt(C, FnStmts);
    }
  }
}

Stmt *AstBuilder::visit_read_stmt(Node *N) {

  std::vector<Expr *> Names;
  for (int i = 1, len = N->getNumChildren(); i < len; i++) {
    auto Child = N->getChild(i);
    if (Child->getType() == Symbol::NAME) {
      Names.push_back(new Name(Child->getValue(), ExprContextKind::Store,
                               Child->getLocation()));
    }
  }
  return new Read(Names, N->getLocation());
}

Expr *AstBuilder::visit_expr(Node *N, ExprContextKind Context) {

  if (N->getType() == Symbol::term || N->getType() == Symbol::expr ||
      N->getType() == Symbol::condition) {
    return visit_binop(N, Context);
  } else {
    // factor
    assert(N->getType() == Symbol::factor);
    return visit_factor(N, Context);
  }
}

void AstBuilder::visit_var_decl(Node *N, std::vector<Decl *> &Decls) {
  auto TypeName = N->FirstChild();
  auto Ty = visit_type_name(TypeName);

  for (auto C : N->getChildren()) {
    if (C->getType() != Symbol::var_item)
      continue;
    Decls.push_back(visit_var_item(C, Ty));
  }
}

Decl *AstBuilder::visit_var_item(Node *N, BasicTypeKind Ty) {
  auto name = N->FirstChild();
  bool IsArray = N->getNumChildren() > 1;
  int Size = IsArray ? visit_subscript2(N->getChild(1)) : 0;
  return new VarDecl(Ty, /* is_array */ IsArray, /* size */ Size,
      /* name */ name->getValue(), name->getLocation());
}

Stmt *AstBuilder::visit_while_stmt(Node *N) {
  auto Cond = visit_condition(N->getChild(2));
  std::vector<Stmt *> Body;
  visit_stmt(N->LastChild(), Body);
  return new While(Cond, Body, N->getLocation());
}

BasicTypeKind AstBuilder::visit_type_name(Node *N) {
  return BasicTypeKindFromString(N->FirstChild()->getValue());
}

int AstBuilder::visit_subscript2(Node *N) {
  return std::stoi(N->getChild(1)->getValue());
}

Expr *AstBuilder::makeChar(Node *N) {
  return new Char(static_cast<int>(N->getValue()[1]), N->getLocation());
}

Expr *AstBuilder::makeNum(Node *N) {
  return new Num(std::stoi(N->getValue()), N->getLocation());
}

namespace simplecc {
Program *BuildAstFromNode(const Node *N) { return AstBuilder().Build(N); }
} // namespace simplecc