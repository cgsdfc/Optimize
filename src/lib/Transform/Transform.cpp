#include "simplecc/Transform/Transform.h"
#include "simplecc/Transform/DeadCodeEliminator.h"
#include "simplecc/Transform/TrivialConstantFolder.h"

namespace simplecc {
void TransformProgram(ProgramAST *P, SymbolTable &S) {
  TrivialConstantFolder().Transform(P, S);
  DeadCodeEliminator().Transform(P);
}

} // namespace simplecc