#ifndef SIMPLECC_TRANSFORM_TRIVIALCONSTANTFOLDER_H
#define SIMPLECC_TRANSFORM_TRIVIALCONSTANTFOLDER_H
#include "simplecc/Transform/ExpressionTransformer.h"

namespace simplecc {

/// This class performs trivial constant folding on Expr nodes.
class TrivialConstantFolder : ExpressionTransformer<TrivialConstantFolder> {
  friend ExpressionTransformer;
  /// Declare all methods that perform constant folding.
  /// Generate things like ``ExprAST *FoldBinOpExpr(BinOpExpr *E);``.
#define HANDLE_CONST_FOLD(Class) ExprAST *Fold##Class(Class *E);
#include "simplecc/Transform/TrivialConstantFolder.def"
  ExprAST *FoldExprAST(ExprAST *E);
  ExprAST *TransformExpr(ExprAST *E, AST *Parent);

public:
  TrivialConstantFolder() = default;
  using ExpressionTransformer::Transform;
};

} // namespace simplecc
#endif // SIMPLECC_TRANSFORM_TRIVIALCONSTANTFOLDER_H