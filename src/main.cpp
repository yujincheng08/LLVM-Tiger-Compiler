#include "mainwindow.h"
#include <AST/ast.h>
#include <QApplication>
#include <iostream>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/TargetSelect.h>

extern int tigerparse();
extern std::unique_ptr<AST::Root> root;

int main(int argc, char *argv[]) {
  // QApplication a(argc, argv);
  // MainWindow w;
  // w.show();
  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmPrinter();
  llvm::InitializeNativeTargetAsmParser();

  tigerparse();
  root->print(0);
  root->codegen();
  // return a.exec();
  return 0;
}
