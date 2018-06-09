#include "ast.h"
#include <iostream>

using namespace AST;
using namespace std;

static void blank(int n) {
  for (int i = 0; i < n; i++) {
    cout << "___";
  }
}

QTreeWidgetItem *add_node(QTreeWidgetItem *parent, std::string description,
                          QString icon) {
  QTreeWidgetItem *itm = new QTreeWidgetItem();
  itm->setText(0, QString::fromStdString(description));
  itm->setIcon(0, QIcon(icon));
  parent->addChild(itm);
  return itm;
}

void Root::print(QTreeWidgetItem *parent, int n) {
  printf("AST_begin\n");
  cout << "Root:" << endl;
  root_->print(parent, n + 1);
  printf("AST_end\n");
}

void SimpleVar::print(QTreeWidgetItem *parent, int n) {
  blank(n);
  cout << "SimpleVar:" << endl;
  QTreeWidgetItem *cur = add_node(parent, "SimpleVar", icon);

  blank(n + 1);
  cout << "name: " << name_ << endl;
  add_node(cur, "[name: " + name_ + "]", icon);
}

void FieldVar::print(QTreeWidgetItem *parent, int n) {
  blank(n);
  cout << "FieldVar:" << endl;
  QTreeWidgetItem *cur = add_node(parent, "FieldVar", icon);

  blank(n + 1);
  cout << "filed: " << field_ << endl;
  add_node(cur, "[field: " + field_ + "]", icon);

  var_->print(cur, n + 1);
}

void SubscriptVar::print(QTreeWidgetItem *parent, int n) {
  blank(n);
  cout << "SubscriptVar:" << endl;
  QTreeWidgetItem *cur = add_node(parent, "SubscriptVar", icon);

  var_->print(cur, n + 1);
  exp_->print(cur, n + 1);
}

void VarExp::print(QTreeWidgetItem *parent, int n) {
  blank(n);
  cout << "VarExp:" << endl;
  QTreeWidgetItem *cur = add_node(parent, "VarExp", icon);
  var_->print(cur, n + 1);
}

void NilExp::print(QTreeWidgetItem *parent, int n) {
  QTreeWidgetItem *cur = add_node(parent, "NilExp", icon);
  blank(n);
  cout << "NilExp:''" << endl;
}

void IntExp::print(QTreeWidgetItem *parent, int n) {
  blank(n);
  cout << "IntExp:" << endl;
  QTreeWidgetItem *cur = add_node(parent, "IntExp", icon);

  blank(n + 1);
  cout << "val: " << val_ << endl;
  add_node(cur, "[val: " + std::to_string(val_) + "]", icon);
}

void StringExp::print(QTreeWidgetItem *parent, int n) {
  blank(n);
  cout << "StringExp:" << endl;
  QTreeWidgetItem *cur = add_node(parent, "StringExp", icon);

  blank(n + 1);
  cout << "val: " << val_ << endl;
  add_node(cur, "[val: " + val_ + "]", icon);
}

void CallExp::print(QTreeWidgetItem *parent, int n) {
  blank(n);
  cout << "CallExp:" << endl;
  QTreeWidgetItem *cur = add_node(parent, "CallExp", icon);

  blank(n + 1);
  cout << "func: " << func_ << endl;
  add_node(cur, "[func: " + func_ + "]", icon);

  for (auto &arg : args_) {
    arg->print(cur, n + 1);
  }
}

void BinaryExp::print(QTreeWidgetItem *parent, int n) {
  blank(n);
  cout << "BinaryExp:" << endl;
  QTreeWidgetItem *cur = add_node(parent, "BinaryExp", icon);

  blank(n + 1);
  cout << "op: '" << (char)op_ << "'" << endl;
  add_node(cur, "[op: " + std::to_string((char)op_) + "]", icon);

  left_->print(cur, n + 1);
  right_->print(cur, n + 1);
}

void Field::print(QTreeWidgetItem *parent, int n) {
  blank(n);
  cout << "Field:" << endl;
  QTreeWidgetItem *cur = add_node(parent, "Field", icon);

  blank(n + 1);
  cout << "name: " << name_ << endl;
  add_node(cur, "[name: " + name_ + "]", icon);
  type_->print(cur, n + 1);
}

void FieldExp::print(QTreeWidgetItem *parent, int n) {
  blank(n);
  cout << "FieldExp:" << endl;
  QTreeWidgetItem *cur = add_node(parent, "FieldExp", icon);

  blank(n + 1);
  cout << "name: " << name_ << endl;
  add_node(cur, "[name: " + name_ + "]", icon);
  exp_->print(cur, n + 1);
}

void RecordExp::print(QTreeWidgetItem *parent, int n) {
  blank(n);
  cout << "RecordExp:" << endl;
  QTreeWidgetItem *cur = add_node(parent, "RecordExp", icon);

  type_->print(cur, n + 1);
  for (auto &fieldExp : fieldExps_) {
    fieldExp->print(cur, n + 1);
  }
}

void SequenceExp::print(QTreeWidgetItem *parent, int n) {
  blank(n);
  cout << "SequenceExp:" << endl;
  QTreeWidgetItem *cur = add_node(parent, "SequenceExp", icon);

  for (auto &exp : exps_) {
    exp->print(cur, n + 1);
  }
}

void AssignExp::print(QTreeWidgetItem *parent, int n) {
  blank(n);
  cout << "AssignExp:" << endl;
  QTreeWidgetItem *cur = add_node(parent, "AssignExp", icon);

  var_->print(cur, n + 1);
  exp_->print(cur, n + 1);
}

void IfExp::print(QTreeWidgetItem *parent, int n) {
  blank(n);
  cout << "IfExp:" << endl;
  QTreeWidgetItem *cur = add_node(parent, "IfExp", icon);

  test_->print(cur, n + 1);
  then_->print(cur, n + 1);
  if (else_) else_->print(cur, n + 1);
}

void WhileExp::print(QTreeWidgetItem *parent, int n) {
  blank(n);
  cout << "WhileExp:" << endl;
  QTreeWidgetItem *cur = add_node(parent, "WhileExp", icon);

  test_->print(cur, n + 1);
  body_->print(cur, n + 1);
}

void ForExp::print(QTreeWidgetItem *parent, int n) {
  blank(n);
  cout << "ForExp:" << endl;
  QTreeWidgetItem *cur = add_node(parent, "ForExp", icon);

  blank(n + 1);
  cout << "var: " << var_ << endl;
  add_node(cur, "[var: " + var_ + "]", icon);

  low_->print(cur, n + 1);
  high_->print(cur, n + 1);
  body_->print(cur, n + 1);
}

void BreakExp::print(QTreeWidgetItem *parent, int n) {
  blank(n);
  cout << "Break:''" << endl;
  QTreeWidgetItem *cur = add_node(parent, "Break", icon);
}

void LetExp::print(QTreeWidgetItem *parent, int n) {
  blank(n);
  cout << "LetExp:" << endl;
  QTreeWidgetItem *cur = add_node(parent, "LetExp", icon);
  for (auto &dec : decs_) {
    dec->print(cur, n + 1);
  }
  body_->print(cur, n + 1);
}

void ArrayExp::print(QTreeWidgetItem *parent, int n) {
  blank(n);
  cout << "ArrayExp:" << endl;
  QTreeWidgetItem *cur = add_node(parent, "ArrayExp", icon);

  type_->print(cur, n + 1);
  size_->print(cur, n + 1);
  init_->print(cur, n + 1);
}

void Prototype::print(QTreeWidgetItem *parent, int n) {
  blank(n);
  cout << "Prototype:" << endl;
  QTreeWidgetItem *cur = add_node(parent, "Prototype", icon);

  blank(n + 1);
  cout << "name: " << name_ << endl;
  add_node(cur, "[name: " + name_ + "]", icon);
  result_->print(cur, n + 1);

  for (auto &param : params_) {
    param->print(cur, n + 1);
  }
}

void FunctionDec::print(QTreeWidgetItem *parent, int n) {
  blank(n);
  cout << "FunctionDec:" << endl;
  QTreeWidgetItem *cur = add_node(parent, "FunctionDec", icon);

  blank(n + 1);
  cout << "name: " << name_ << endl;
  add_node(cur, "[name: " + name_ + "]", icon);

  proto_->print(cur, n + 1);
  body_->print(cur, n + 1);
}

void VarDec::print(QTreeWidgetItem *parent, int n) {
  blank(n);
  cout << "VarDec:" << endl;
  QTreeWidgetItem *cur = add_node(parent, "VarDec", icon);

  blank(n + 1);
  cout << "name: " << name_ << endl;
  add_node(cur, "[name:" + name_ + "]", icon);
  if (type_) type_->print(cur, n + 1);
  init_->print(cur, n + 1);
}

void TypeDec::print(QTreeWidgetItem *parent, int n) {
  blank(n);
  cout << "TypeDec:" << endl;
  QTreeWidgetItem *cur = add_node(parent, "TypeDec", icon);
  type_->print(cur, n + 1);
}

void NameType::print(QTreeWidgetItem *parent, int n) {
  blank(n);
  cout << "NameType:" << endl;
  QTreeWidgetItem *cur = add_node(parent, "NameType", icon);

  blank(n + 1);
  cout << "name: " << name_ << endl;
  add_node(cur, "[name: " + name_ + "]", icon);
}

void RecordType::print(QTreeWidgetItem *parent, int n) {
  blank(n);
  cout << "RecordType:" << endl;
  QTreeWidgetItem *cur = add_node(parent, "RecordType", icon);

  for (auto &field : fields_) {
    field->print(cur, n + 1);
  }
}

void ArrayType::print(QTreeWidgetItem *parent, int n) {
  blank(n);
  cout << "ArrayType:" << endl;
  QTreeWidgetItem *cur = add_node(parent, "ArrayType", icon);
  type_->print(cur, n + 1);
}
