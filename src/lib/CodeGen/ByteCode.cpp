#include "simplecc/CodeGen/ByteCode.h"
#include "simplecc/Support/ErrorManager.h"

#include <cassert>
#include <iomanip>
#include <iostream>
#include <simplecc/CodeGen/ByteCode.h>

namespace simplecc {

ByteCode ByteCode::Create(Opcode Op, int Val) {
  ByteCode B(Op);
  B.setIntOperand(Val);
  return B;
}

ByteCode ByteCode::Create(Opcode Op, const char *Val) {
  ByteCode B(Op);
  B.setStrOperand(Val);
  return B;
}

ByteCode ByteCode::Create(Opcode Op, const char *Str, int Int) {
  ByteCode B(Op);
  B.setStrOperand(Str);
  B.setIntOperand(Int);
  return B;
}

bool ByteCode::IsJumpXXX(Opcode Op) {
  switch (Op) {
  case Opcode::JUMP_IF_TRUE:
  case Opcode::JUMP_IF_FALSE:
  case Opcode::JUMP_IF_NOT_EQUAL:
  case Opcode::JUMP_IF_EQUAL:
  case Opcode::JUMP_IF_GREATER:
  case Opcode::JUMP_IF_GREATER_EQUAL:
  case Opcode::JUMP_IF_LESS:
  case Opcode::JUMP_IF_LESS_EQUAL:
  case Opcode::JUMP_FORWARD:return true;
  default:return false;
  }
}

bool ByteCode::HasIntOperand(Opcode Op) {
  switch (Op) {
  case Opcode::JUMP_IF_TRUE:
  case Opcode::JUMP_IF_FALSE:
  case Opcode::JUMP_FORWARD:
  case Opcode::JUMP_IF_NOT_EQUAL:
  case Opcode::JUMP_IF_EQUAL:
  case Opcode::JUMP_IF_GREATER:
  case Opcode::JUMP_IF_GREATER_EQUAL:
  case Opcode::JUMP_IF_LESS:
  case Opcode::JUMP_IF_LESS_EQUAL:
  case Opcode::CALL_FUNCTION:
  case Opcode::LOAD_STRING:
  case Opcode::LOAD_CONST:return true;
  default:return false;
  }
}

bool ByteCode::HasStrOperand(Opcode Op) {
  switch (Op) {
  case Opcode::LOAD_LOCAL:
  case Opcode::LOAD_GLOBAL:
  case Opcode::STORE_LOCAL:
  case Opcode::STORE_GLOBAL:
  case Opcode::CALL_FUNCTION:return true;
  default:return false;
  }
}

bool ByteCode::HasNoOperand(Opcode Op) {
  switch (Op) {
  case Opcode::BINARY_ADD:
  case Opcode::BINARY_SUB:
  case Opcode::BINARY_MULTIPLY:
  case Opcode::BINARY_DIVIDE:
  case Opcode::UNARY_POSITIVE:
  case Opcode::UNARY_NEGATIVE:
  case Opcode::POP_TOP:
  case Opcode::PRINT_NEWLINE:return true;
  default:return false;
  }
}

void ByteCode::Format(std::ostream &O) const {
  /* O << std::setw(4) << GetSourceLineno(); */
  O << std::left << std::setw(4) << getByteCodeOffset();
  O << std::left << std::setw(25) << getOpcode();

  if (HasIntOperand()) {
    O << std::setw(10) << getIntOperand();
  }

  if (HasStrOperand()) {
    O << std::setw(20) << getStrOperand();
  }
}

const char *ByteCode::getOpcodeName(unsigned Op) {
  switch (Op) {
  default: assert(false && "Invalid Opcode");
#define HANDLE_OPCODE(opcode, camelName) case Opcode::opcode: return #opcode;
#include "simplecc/CodeGen/Opcode.def"
  }
}

} // namespace simplecc
