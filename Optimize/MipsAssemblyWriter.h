#ifndef MIPS_ASSEMBLY_WRITER_H
#define MIPS_ASSEMBLY_WRITER_H
#include "LocalContext.h"
#include "Print.h"

#include <iostream>

namespace simplecompiler {
class ByteCodeFunction;
class ByteCodeModule;

class MipsAssemblyWriter {
  // Return the total bytes consumed by local objects, including
  // variables, arrays and formal arguments.
  unsigned GetLocalObjectsInBytes(const ByteCodeFunction &TheFunction) const;

  void WriteData(Printer &W, const ByteCodeModule &Module);
  void WriteText(Printer &W, const ByteCodeModule &Module);

  void WritePrologue(Printer &W, const ByteCodeFunction &TheFunction);
  void WriteEpilogue(Printer &W, const ByteCodeFunction &TheFunction);
  void WriteFunction(Printer &W, const ByteCodeFunction &TheFunction);

public:
  MipsAssemblyWriter() = default;
  ~MipsAssemblyWriter() = default;

  void Write(const ByteCodeModule &M, std::ostream &O);

private:
  LocalContext TheContext;
};

} // namespace simplecompiler

#endif