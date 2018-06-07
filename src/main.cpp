#include <QApplication>
#include "mainwindow.h"
#include <AST/ast.h>

extern int tigerparse();
extern std::unique_ptr<AST::Node> root;

int main(int argc, char *argv[]) {
  //QApplication a(argc, argv);
  //MainWindow w;
  //w.show();
  tigerparse();
  root->codegen();
  //return a.exec();
  return 0;
}
