#include "simplecc/LLVM/LLVMValueMap.h"

using namespace simplecc;

Constant *LLVMValueMap::getGlobalInitializer(VarDecl *VD) {
  if (VD->getIsArray()) {
    return llvm::ConstantAggregateZero::get(getTypeFromVarDecl(VD));
  }
  switch (VD->getType()) {
  case BasicTypeKind::Int:return getInt(0);
  case BasicTypeKind::Character:return getChar(0);
  default:llvm_unreachable("Void cannot be");
  }
}

Constant *LLVMValueMap::getConstantFromExpr(Expr *E) const {
  switch (E->GetKind()) {
  case Expr::Num:return getInt(static_cast<Num *>(E)->getN());
  case Expr::Char:return getChar(static_cast<Char *>(E)->getC());
  default:llvm_unreachable("Expr must be Num or Char");
  }
}

Value *LLVMValueMap::getConstant(const ConstType &C) const {
  return ConstantInt::get(getType(C.getType()), C.getValue(), true);
}

Constant *LLVMValueMap::getBool(bool B) const {
  return B ? ConstantInt::getTrue(getContext())
           : ConstantInt::getFalse(getContext());
}

Constant *LLVMValueMap::getInt(int N) const {
  return ConstantInt::get(getType(BasicTypeKind::Int), N, false);
}

Constant *LLVMValueMap::getChar(char C) const {
  return ConstantInt::get(getType(BasicTypeKind::Character), C, false);
}