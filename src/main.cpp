#include <AST/ast.h>
#include <QApplication>

extern int tigerparse();
extern std::unique_ptr<AST::Node> root;

int main(int argc, char *argv[]) {
  // QApplication a(argc, argv);
  // MainWindow w;
  // w.show();
  tigerparse();

  AST::print(root);

  root->codegen();
  // return a.exec();
  return 0;
}
