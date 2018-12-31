#include "simplecc/Parse/Enums.h"
#include <cassert>

namespace simplecc {

std::ostream &operator<<(std::ostream &os, OperatorKind val) {
  switch (val) {
  default: assert(false && "Invalid Enum Value");
#define HANDLE_OPERATOR(VAL, STR, FUNC)  \
case OperatorKind::VAL: return os << #VAL;
#include "simplecc/Parse/Enums.def"
  }
}

std::ostream &operator<<(std::ostream &os, UnaryopKind val) {
  switch (val) {
  default: assert(false && "Invalid Enum Value");
#define HANDLE_UNARYOP(VAL, STR)  \
case UnaryopKind::VAL: return os << #VAL;
#include "simplecc/Parse/Enums.def"
  }
}

std::ostream &operator<<(std::ostream &os, ExprContextKind val) {
  switch (val) {
  default: assert(false && "Invalid Enum Value");
#define HANDLE_EXPRCONTEXT(VAL, STR)  \
case ExprContextKind::VAL: return os << #VAL;
#include "simplecc/Parse/Enums.def"
  }
}

std::ostream &operator<<(std::ostream &os, BasicTypeKind val) {
  switch (val) {
  default: assert(false && "Invalid Enum Value");
#define HANDLE_BASICTYPE(VAL, STR)  \
case BasicTypeKind::VAL: return os << #VAL;
#include "simplecc/Parse/Enums.def"
  }
}

OperatorKind OperatorKindFromString(const std::string &s) {
#define HANDLE_OPERATOR(Val, Str, FUNC) \
if (s == Str) return OperatorKind::Val;
#include "simplecc/Parse/Enums.def"
  assert(false && "Invalid String Conversion");
}

const char *CStringFromOperatorKind(OperatorKind val) {
  switch (val) {
  default: assert(false && "Invalid Enum Value");
#define HANDLE_OPERATOR(Val, Str, FUNC) case OperatorKind::Val: return Str;
#include "simplecc/Parse/Enums.def"
  }
}

UnaryopKind UnaryopKindFromString(const std::string &s) {
#define HANDLE_UNARYOP(Val, Str) \
if (s == Str) return UnaryopKind::Val;
#include "simplecc/Parse/Enums.def"
  assert(false && "Invalid String Conversion");
}

const char *CStringFromUnaryopKind(UnaryopKind val) {
  switch (val) {
  default: assert(false && "Invalid Enum Value");
#define HANDLE_UNARYOP(Val, Str) case UnaryopKind::Val: return Str;
#include "simplecc/Parse/Enums.def"
  }
}

BasicTypeKind BasicTypeKindFromString(const std::string &s) {
#define HANDLE_BASICTYPE(Val, Str) \
if (s == Str) return BasicTypeKind::Val;
#include "simplecc/Parse/Enums.def"
  assert(false && "Invalid String Conversion");
}

const char *CStringFromBasicTypeKind(BasicTypeKind val) {
  switch (val) {
  default: assert(false && "Invalid Enum Value");
#define HANDLE_BASICTYPE(Val, Str) case BasicTypeKind::Val: return Str;
#include "simplecc/Parse/Enums.def"
  }
}

}