#ifndef COMPILER_H
#define COMPILER_H

#include <AST/ast.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/TargetSelect.h>
#include <stdlib.h>
#include <unistd.h>
#include <QTreeWidgetItem>
#include <iostream>

extern int tigerparse();
extern std::unique_ptr<AST::Root> root;

class Compiler {
 public:
  //  Compiler(QTreeWidgetItem *view_root) : view_root(view_root) {}

  void compile(QTreeWidgetItem *view_root, std::string s);

  //  void set_view_root(QTreeWidgetItem *view_root) {
  //    this->view_root = view_root;
  //  }

 private:
  //  QTreeWidgetItem *view_root;
};

#endif  // COMPILER_H
