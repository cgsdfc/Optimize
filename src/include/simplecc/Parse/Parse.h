#ifndef SIMPLECC_PARSE_PARSE_H
#define SIMPLECC_PARSE_PARSE_H
#include "simplecc/Parse/AST.h"
#include "simplecc/Parse/Node.h"
#include <memory> // for unique_ptr

namespace simplecc {
std::unique_ptr<Node> BuildCST(const std::vector<TokenInfo> &TheTokens);
std::unique_ptr<Program> BuildAST(const std::vector<TokenInfo> &TheTokens);
} // namespace simplecc
#endif // SIMPLECC_PARSE_PARSE_H
