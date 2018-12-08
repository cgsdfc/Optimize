#ifndef TOKENINFO_H
#define TOKENINFO_H
#include "Grammar.h"

#include <iostream>
#include <string>

namespace simplecompiler {
using String = std::string;

inline bool IsTerminal(Symbol type) {
  return static_cast<int>(type) < NT_OFFSET;
}

inline bool IsNonterminal(Symbol type) { return !IsTerminal(type); }

inline const char *GetSymbolName(Symbol s) {
  auto sym = static_cast<int>(s);
  return IsTerminal(s) ? TokenNames[sym] : SymbolNames[sym - NT_OFFSET];
}

class Location {
public:
  unsigned lineno;
  unsigned col_offset;

  Location() : lineno(0), col_offset(0) {}
  Location(unsigned lineno, unsigned col_offset)
      : lineno(lineno), col_offset(col_offset) {}

  unsigned getLineNo() const { return lineno; }
  unsigned getColOffset() const { return col_offset; }

  void Format(std::ostream &os) const {
    os << "Location(" << lineno << ", " << col_offset << ")";
  }
};

inline std::ostream &operator<<(std::ostream &os, const Location &loc) {
  loc.Format(os);
  return os;
}

class TokenInfo {
public:
  Symbol type;
  String string;
  Location start;
  String line;

  TokenInfo(Symbol type, const String &string, const Location &start,
            const String &line)
      : type(type), string(string), start(start), line(line) {}

  TokenInfo(TokenInfo &&other)
      : type(other.type), string(std::move(other.string)), start(other.start),
        line(std::move(other.line)) {}

  void Format(std::ostream &os) const;
  const char *GetTypeName() const;
  const Location &GetLocation() const { return start; }
  const String &GetString() const { return string; }
  const String &GetLine() const { return line; }
};

}
#endif
