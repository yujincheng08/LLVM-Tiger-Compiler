#include "mainwindow.h"
#include <AST/ast.h>
#include <QApplication>
#include <iostream>
#include <llvm/ADT/StringRef.h>

extern int tigerparse();
extern std::unique_ptr<AST::Node> root;

int main(int argc, char *argv[]) {
  // QApplication a(argc, argv);
  // MainWindow w;
  // w.show();
  tigerparse();
  auto value = root->codegen();
  std::cout << value->getName().str();
  // return a.exec();
  return 0;
}
