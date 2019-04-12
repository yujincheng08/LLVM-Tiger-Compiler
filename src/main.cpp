#include "AST/ast.h"
#include <iostream>

extern int tigerparse();
extern std::unique_ptr<AST::Root> root;

int main(int argc, char *argv[]) {
  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmPrinter();
  llvm::InitializeNativeTargetAsmParser();

  tigerparse();

  if (root) {
    CodeGenContext codeGenContext;
    root->codegen(codeGenContext);
  }
  return 0;
}
