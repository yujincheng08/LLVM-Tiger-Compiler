#include "compiler.h"
#include "utils/codegencontext.h"

void Compiler::compile(QTreeWidgetItem *view_root, std::string s) {
  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmPrinter();
  llvm::InitializeNativeTargetAsmParser();

  int p[2];
  if (pipe(p) < 0) {
    std::cout << "pipe error" << std::endl;
  }
  dup2(p[0], STDIN_FILENO);
  write(p[1], s.c_str(), s.size());

  close(p[1]);
  tigerparse();

  std::cout << s << std::endl;

  root->print(view_root, 0);
  CodeGenContext codeGenContext;
  root->codegen(codeGenContext);
}
