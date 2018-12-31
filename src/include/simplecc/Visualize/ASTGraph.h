#ifndef SIMPLECC_VISUALIZE_ASTGRAPH_H
#define SIMPLECC_VISUALIZE_ASTGRAPH_H
#include "simplecc/Parse/AST.h"
#include "simplecc/Visualize/ASTNode.h"
#include "simplecc/Visualize/TreeLikeIterator.h"
#include <llvm/ADT/STLExtras.h>
#include <cassert>
#include <map>
#include <memory>
#include <vector>

namespace simplecc {
/// A class that keeps all the edges of an ASTNode.
class AstGraph {
public:
  explicit AstGraph(Program *P) : TheProgram(P), Edges(), Nodes() {}

  using EdgeType = std::vector<ASTNode *>;
  using ChildIteratorType = EdgeType::const_iterator;

  /// Iterator to all nodes of a graph.
  class AstIterator :
      public TreeLikeIterator<AstIterator, ASTNode *, ChildIteratorType> {
  public:
    AstIterator(Program *P, AstGraph *G) : TreeLikeIterator(), Parent(G) {
      Initialize(Parent->getNodeOrCreate(P));
    }
    AstIterator() = default;

    EdgeRange getEdges(value_type N) {
      const auto &LazyEdges = Parent->getEdgeOrCreate(*N);
      return llvm::make_range(LazyEdges.begin(), LazyEdges.end());
    }
  private:
    AstGraph *Parent = nullptr;
  };
  using NodeIterator = AstIterator;

  /// Iterate over all nodes.
  NodeIterator nodes_begin() const {
    return NodeIterator(TheProgram, const_cast<AstGraph *>(this));
  }
  NodeIterator nodes_end() const { return NodeIterator(); }

  llvm::iterator_range<NodeIterator> nodes() {
    return llvm::make_range(nodes_begin(), nodes_end());
  }

  /// Iterate all children of one node.
  ChildIteratorType child_begin(const ASTNode &R) {
    return std::begin(getEdgeOrCreate(R));
  }
  ChildIteratorType child_end(const ASTNode &R) {
    return std::end(getEdgeOrCreate(R));
  }

  /// Lazily create edges for a Node.
  const std::vector<ASTNode *> &getEdgeOrCreate(const ASTNode &R);

  /// Lazily create a node, namely an AstRef.
  ASTNode *getNodeOrCreate(AST *Ptr);

  const Program *getProgram() const { return TheProgram; }
private:
  /// Root of AST.
  Program *TheProgram;
  /// Lazily created edges for each node.
  std::map<AST *, EdgeType> Edges;
  /// Lazily created nodes.
  std::map<AST *, std::unique_ptr<ASTNode>> Nodes;
};
} // namespace simplecc

#endif