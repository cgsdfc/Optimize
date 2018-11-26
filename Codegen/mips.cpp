#include "mips.h"
#include "OpcodeDispatcher.h"
#include "compile.h"
#include "error.h"

#include <sstream>
#include <unordered_set>

// Return the bytes for n entries
inline int BytesFromEntries(int n_entries) {
  return 4 * n_entries;
}

class AssemblyWriter {
  std::ostream &os;
public:
  AssemblyWriter(std::ostream &os): os(os) {}

  template <typename... Args>
  void WriteLine(Args&&... args) {
    Print(os, std::forward<Args>(args)...);
  }

};

// Capture global information
class GlobalContext {
public:
  static String GetGlobalLabel(const String &name, bool colon) {
    return colon ? name + ":" : name;
  }

  static String GetStringLiteralLabel(int ID, bool colon) {
    std::ostringstream os;
    os << "string_" << ID;
    if (colon) os << ":";
    return os.str();
  }

};

// Provide information for ByteCodeToMipsTranslator
class LocalContext {
public:
  std::unordered_map<const char*, int> local_offsets;
  std::unordered_set<int> target_index;
  const String &name;

  LocalContext(const CompiledFunction &fun): name(fun.GetName()) {



  }

  // Return if an offset is a jump target
  bool IsTarget(int offset) {
    return target_index.count(offset);
  }

  // Return the offset of local name relatited to frame pointer
  int GetLocalOffset(const char *name) const {
    return 0;
  }

  // Return the label denoting a global object.
  String GetGlobalLabel(const char *name) const {
    return GlobalContext::GetGlobalLabel(name, false);
  }

  // Return the label for this function.
  String GetFuncLabel() const {
    return name + ":";
  }

  // Return the label denoting a string literal.
  String GetStringLiteralLabel(int ID) const {
    return GlobalContext::GetStringLiteralLabel(ID, false);
  }

  // Return the label denoting the epilogue of a function.
  String GetReturnLabel(bool colon) const {
    return "";
  }

  // Return the label of a jump target of a function.
  String GetTargetLabel(int offset, bool colon = false) const {
    std::ostringstream os;
    os << name << "_" << offset;
    if (colon) os << ":";
    return os.str();
  }

  // Return the number of local entries
  unsigned GetLocalEntries() const {
    return 0;
  }

};

// Serve as a template translating one ByteCode to MIPS instructions
class ByteCodeToMipsTranslator: public OpcodeDispatcher<ByteCodeToMipsTranslator> { // {{{
  AssemblyWriter &w;
  const LocalContext &context;

public:
  ByteCodeToMipsTranslator(AssemblyWriter &w, const LocalContext &context):
    w(w), context(context) {
      MakePrologue();
    }

  ~ByteCodeToMipsTranslator() {
    MakeEpilogue();
  }

  // Push a register onto the stack
  template <int N>
  void PUSH(const char (&r)[N]) {
    /* # PUSH the item in $t0: */
    /* subu $sp,$sp,4      #   point to the place for the new item, */
    /* sw   $t0,($sp)      #   store the contents of $t0 as the new top. */
    static_assert(N);
    assert(r[0] == '$');
    w.WriteLine("subu $sp, $sp, 4");
    w.WriteLine("sw", r, "0($sp)");
  }

  // Pop the stack, optionally taking the tos value
  void POP(const char *r = nullptr) {
    /* lw   $t0,($sp)      #   Copy top item to $t0. */
    /* addu $sp,$sp,4      #   Point to the item beneath the old top. */
    if (r) {
      assert(r[0] == '$');
      w.WriteLine("lw", r, "0($sp)");
    }
    w.WriteLine("addu $sp, $sp, 4");
  }

  void HandleLoadLocal(const ByteCode &code) {
    auto offset = context.GetLocalOffset(code.GetStrOperand());
    w.WriteLine("lw $t0,", offset, "($fp)");
    PUSH("$t0");
  }

  void HandleLoadGlobal(const ByteCode &code) {
    auto label = context.GetGlobalLabel(code.GetStrOperand());
    w.WriteLine("la $t0,", label);
    w.WriteLine("lw $t0, 0($t0)");
    PUSH("$t0");
  }

  void HandleLoadConst(const ByteCode &code) {
    w.WriteLine("li $t0,", code.GetIntOperand());
    PUSH("$t0");
  }

  void HandleLoadString(const ByteCode &code) {
    w.WriteLine("la $t0,", context.GetStringLiteralLabel(code.GetIntOperand()));
    PUSH("$t0");
  }

  void HandleStoreLocal(const ByteCode &code) {

  }

  void HandleStoreGlobal(const ByteCode &code) {

  }

  void HandleBinary(const char *op) {
    POP("$t0");
    POP("$t1");
    w.WriteLine(op, "$t2, $t0, $t1");
    PUSH("$t2");
  }

  void HandleBinaryAdd(const ByteCode &code) {
    HandleBinary("add");
  }

  void HandleBinarySub(const ByteCode &code) {
    HandleBinary("sub");
  }

  void HandleBinaryMultiply(const ByteCode &code) {
    HandleBinary("mul");
  }

  void HandleBinaryDivide(const ByteCode &code) {
    HandleBinary("div");
  }

  void HandleUnaryPositive(const ByteCode &code) {
    // Nop
  }

  void HandleUnaryNegative(const ByteCode &code) {
    w.WriteLine("lw $t0, 0($sp)");
    w.WriteLine("sub $t0, $zero, $t0");
    w.WriteLine("sw, $t0, 0($sp)");
  }

  void HandleCallFunction(const ByteCode &code) {
    auto label = context.GetGlobalLabel(code.GetStrOperand());
    w.WriteLine("jal", label);
    auto bytes = BytesFromEntries(code.GetIntOperand());
    w.WriteLine("addi $sp, $sp", bytes);
    PUSH("$v0");
  }

  void HandleReturn() {
    auto label = context.GetReturnLabel(false);
    w.WriteLine("j", label);
  }

  void HandleReturnValue(const ByteCode &code) {
    POP("$v0");
    HandleReturn();
  }

  void HandleReturnNone(const ByteCode &code) {
    HandleReturn();
  }

  void HandlePrint(int syscall_code) {
    w.WriteLine("li $v0,", syscall_code);
    POP("$a0");
    w.WriteLine("syscall");
  }

  void HandlePrintString(const ByteCode &code) {
    /* print string */
    /* v0 = 4 */
    /* $a0 = address of null-terminated string to print */
    HandlePrint(4);
  }

  void HandlePrintCharacter(const ByteCode &code) {
    /* print character */
    /* 11 */
    /* $a0 = character to print */
    HandlePrint(11);
  }

  void HandlePrintInteger(const ByteCode &code) {
    /* print integer */
    /* 1 */
    /* $a0 = integer to print */
    HandlePrint(1);
  }

  void HandleRead(int syscall_code) {
    w.WriteLine("li $v0,", syscall_code);
    w.WriteLine("syscall");
    PUSH("$v0");
  }

  void HandleReadInteger(const ByteCode &code) {
    /* read integer */
    /* 5 */
    /* $v0 contains integer read */
    HandleRead(5);
  }

  void HandleReadCharacter(const ByteCode &code) {
    /* read character */
    /* 12 */
    /* $v0 contains character read */
    HandleRead(12);
  }

  void HandleBinarySubscr(const ByteCode &code) {

  }

  void HandleStoreSubscr(const ByteCode &code) {

  }

  void HandleUnaryJumpIf(const char *op, const ByteCode &code) {
    POP("$t0");
    auto label = context.GetTargetLabel(code.GetIntOperand());
    w.WriteLine(op, "$t0, $zero,", label);
  }

  void HandleJumpIfTrue(const ByteCode &code) {
    HandleUnaryJumpIf("beq", code);
  }

  void HandleJumpIfFalse(const ByteCode &code) {
    HandleUnaryJumpIf("bne", code);
  }

  void HandleJumpForward(const ByteCode &code) {
    auto label = context.GetTargetLabel(code.GetIntOperand());
    w.WriteLine("j", label);
  }

  void HandleBinaryJumpIf(const char *op, const ByteCode &code) {
    POP("$t0");
    POP("$t1");
    auto label = context.GetTargetLabel(code.GetIntOperand());
    w.WriteLine(op, "$t0, $t1,", label);
  }

  void HandleJumpIfNotEqual(const ByteCode &code) {
    HandleBinaryJumpIf("bne", code);
  }

  void HandleJumpIfEqual(const ByteCode &code) {
    HandleBinaryJumpIf("beq", code);
  }

  void HandleJumpIfGreater(const ByteCode &code) {
    HandleBinaryJumpIf("bge", code);
  }

  void HandleJumpIfGreaterEqual(const ByteCode &code) {
    HandleBinaryJumpIf("bge", code);
  }

  void HandleJumpIfLess(const ByteCode &code) {
    HandleBinaryJumpIf("blt", code);
  }

  void HandleJumpIfLessEqual(const ByteCode &code) {
    HandleBinaryJumpIf("ble", code);
  }

  void HandlePopTop(const ByteCode &code) {
    POP();
  }

  void MakePrologue() {
    w.WriteLine("sw $ra, 0($sp)");
    /* w.WriteLine("sw $sp, -4($sp)"); */
    w.WriteLine("sw $fp, -8($sp)");
    w.WriteLine("move $fp, $sp");
    w.WriteLine("sub $sp, $sp, 24");
    // copy arguments here
    // copy the n-1 th argument:
    // lw $t0, 4($fp)
    // sw $t0, 0($sp)
    // in general case:
    // lw $t0, n*4($fp)
    // sw $t0, (n-1)*4($sp)
    auto offset = BytesFromEntries(context.GetLocalEntries());
    w.WriteLine("sub $sp, $sp,", offset);
    // now $fp points to the bottom of stack,
    // $sp points to the top of stack.
  }

  void MakeEpilogue() {
    w.WriteLine(context.GetReturnLabel(true));
    w.WriteLine("lw $ra, 0($fp)");
    /* w.WriteLine("lw $sp, -4($fp)"); */
    w.WriteLine("move $sp, $fp");
    w.WriteLine("lw $fp, -8($fp)");
    w.WriteLine("jr $ra");
  }

};
// }}}

class FunctionAssembler {
  const CompiledFunction &source;
  AssemblyWriter &w;
  LocalContext context;


public:
  FunctionAssembler(const CompiledFunction &source, AssemblyWriter &w):
    source(source), w(w), context(source) {}

  void Assemble() {
    w.WriteLine(context.GetFuncLabel());
    ByteCodeToMipsTranslator translator(w, context);
    for (const auto &byteCode: source.GetCode()) {
      auto offset = byteCode.GetOffset();
      if (context.IsTarget(offset)) {
        w.WriteLine(context.GetTargetLabel(offset, true));
      }
      translator.dispatch(byteCode);
    }
  }

};

class ModuleAssembler {
  const CompiledModule &module;
  AssemblyWriter w;

  void MakeDataSegment() {
    w.WriteLine(".data");
    w.WriteLine("# Global variables and arrays");
    for (const auto &item: module.GetSymbols()) {
      const auto &entry = item.second;
      if (entry.IsConstant() || entry.IsFunction())
        continue;
      auto &&label = GlobalContext::GetGlobalLabel(item.first, true);
      if (entry.IsArray()) {
        auto bytes = BytesFromEntries(entry.AsArray().GetSize());
        w.WriteLine(label, ".space", bytes);
      }
      else {
        w.WriteLine(label, ".word", 0);
      }
    }
    w.WriteLine();
    w.WriteLine("# String literals");
    for (const auto &item: module.GetStringLiteralTable()) {
      auto &&label = GlobalContext::GetStringLiteralLabel(item.second, true);
      w.WriteLine(label, ".asciiz", item.first);
    }
    w.WriteLine("# End of data segment");
  }

  void MakeTextSegment() {
    w.WriteLine(".text");
    w.WriteLine("# Boilerplate");
    w.WriteLine(".globl main");
    w.WriteLine("jal main");
    w.WriteLine("li $v0, 10");
    w.WriteLine("syscall");
    w.WriteLine();
    w.WriteLine("# User defined functions");

    for (const auto &fun: module.GetFunctions()) {
      FunctionAssembler(fun, w).Assemble();
      w.WriteLine("# End of", fun.GetName());
      w.WriteLine();
    }
    w.WriteLine("# End of text segment");
  }

public:
  ModuleAssembler(const CompiledModule &module, std::ostream &os):
    module(module), w(os) {}

  void Assemble() {
    MakeDataSegment();
    MakeTextSegment();
  }
};

void AssembleMips(const CompiledModule &module, std::ostream &os) {
  ModuleAssembler(module, os).Assemble();
}

// vim: set foldmethod=marker
