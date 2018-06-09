#include <AST/ast.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/TargetSelect.h>
#include <QApplication>
#include <iostream>
#include "mainwindow.h"

extern int tigerparse();
extern std::unique_ptr<AST::Root> root;

int main() {
  // QApplication a(argc, argv);
  // MainWindow w;
  // w.show();
  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmPrinter();
  llvm::InitializeNativeTargetAsmParser();

  tigerparse();
  CodeGenContext context;
  if (root) {
    root->print(0);
    root->codegen(context);
  }
  // return a.exec();
  return 0;
}
