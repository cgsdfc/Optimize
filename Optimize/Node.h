#ifndef NODE_H
#define NODE_H

#include "tokenize.h"

namespace simplecompiler {
class Node {
  String FormatValue() const;

public:
  Symbol type;
  String value;
  std::vector<Node *> children;
  Location location;

  Node(Symbol type, const String &value, const Location &location)
      : type(type), value(value), children(), location(location) {}

  ~Node();

  void AddChild(Node *child) { children.push_back(child); }

  Node *FirstChild() { return children[0]; }

  Node *LastChild() { return *(children.end() - 1); }

  void Format(std::ostream &os) const;

  const char *GetTypeName() const { return GetSymbolName(type); }
  const Location &GetLocation() const { return location; }
  const String &GetValue() const { return value; }
};

inline std::ostream &operator<<(std::ostream &os, const Node &node) {
  node.Format(os);
  return os;
}
} // namespace simplecompiler

#endif
