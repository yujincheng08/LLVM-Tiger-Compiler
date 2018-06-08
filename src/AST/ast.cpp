#include "ast.h"
#include <iostream>

using namespace AST;
using namespace std;

static void blank(int n) {
  for (int i = 0; i < n; i++) {
    cout << "___";
  }
}

void Root::print(int n) {
  blank(n);
  cout << "Root" << endl;

  root_->print(n + 1);
}

void SimpleVar::print(int n) {
  blank(n);
  cout << "SimpleVar"
       << " [ name: " << name_ << " ]" << endl;
}

void FieldVar::print(int n) {
  blank(n);
  cout << "FieldVar"
       << " [ filed: " << field_ << " ]" << endl;
  var_->print(n + 1);
}

void SubscriptVar::print(int n) {
  blank(n);
  cout << "SubscriptVar" << endl;
  var_->print(n + 1);
  exp_->print(n + 1);
}

void VarExp::print(int n) {
  blank(n);
  cout << "VarExp" << endl;
  var_->print(n + 1);
}

void NilExp::print(int n) {
  blank(n);
  cout << "Nil" << endl;
}

void IntExp::print(int n) {
  blank(n);
  cout << "IntExp"
       << " [ val: " << val_ << " ]" << endl;
}

void StringExp::print(int n) {
  blank(n);
  cout << "StringExp"
       << " [ val: " << val_ << " ]" << endl;
}

void CallExp::print(int n) {
  blank(n);
  cout << "CallExp"
       << " [ func: " << func_ << " ]" << endl;

  for (auto &arg : args_) {
    arg->print(n + 1);
  }
}

void BinaryExp::print(int n) {
  blank(n);
  cout << "BinaryExp"
       << " [ op: " << (char)op_ << " ]" << endl;
  left_->print(n + 1);
  right_->print(n + 1);
}

void Field::print(int n) {
  blank(n);
  cout << "Field"
       << " [ name: " << name_ << " ]" << endl;
  type_->print(n + 1);
}

void FieldExp::print(int n) {
  blank(n);
  cout << "FieldExp"
       << " [ name: " << name_ << " ]" << endl;
  exp_->print(n + 1);
}

void RecordExp::print(int n) {
  blank(n);
  cout << "RecordExp"
       << " [ name: " << name_ << " ]" << endl;
  for (auto &fieldExp : fieldExps_) {
    fieldExp->print(n + 1);
  }
}

void SequenceExp::print(int n) {
  blank(n);
  cout << "SequenceExp" << endl;

  for (auto &exp : exps_) {
    exp->print(n + 1);
  }
}

void AssignExp::print(int n) {
  blank(n);
  cout << "AssignExp" << endl;

  var_->print(n + 1);
  exp_->print(n + 1);
}

void IfExp::print(int n) {
  blank(n);
  cout << "IfExp" << endl;

  test_->print(n + 1);
  then_->print(n + 1);
  if (else_) else_->print(n + 1);
}

void WhileExp::print(int n) {
  blank(n);
  cout << "WhileExp" << endl;

  test_->print(n + 1);
  body_->print(n + 1);
}

void ForExp::print(int n) {
  blank(n);
  cout << "ForExp"
       << " [ var: " << var_ << " ]" << endl;

  low_->print(n + 1);
  high_->print(n + 1);
  body_->print(n + 1);
}

void BreakExp::print(int n) {
  blank(n);
  cout << "Break" << endl;
}

void LetExp::print(int n) {
  blank(n);
  cout << "LetExp" << endl;
  for (auto &dec : decs_) {
    dec->print(n + 1);
  }
  body_->print(n + 1);
}

void ArrayExp::print(int n) {
  blank(n);
  cout << "ArrayExp" << endl;

  type_->print(n + 1);
  size_->print(n + 1);
  init_->print(n + 1);
}

void Prototype::print(int n) {
  blank(n);
  cout << "Prototype"
       << " [ name: " << name_ << " ]" << endl;
  result_->print(n + 1);

  for (auto &param : params_) {
    param->print(n + 1);
  }
}

void FunctionDec::print(int n) {
  blank(n);
  cout << "FunctionDec"
       << " [ name: " << name_ << " ]" << endl;

  proto_->print(n + 1);
  body_->print(n + 1);
}

void VarDec::print(int n) {
  blank(n);
  cout << "VarDec"
       << " [ name: " << name_ << " ]" << endl;
  if (type_) type_->print(n + 1);
  init_->print(n + 1);
}

void TypeDec::print(int n) {
  blank(n);
  cout << "TypeDec" << endl;
  type_->print(n + 1);
}

void NameType::print(int n) {
  blank(n);
  cout << "NameType"
       << " [ name: " << name_ << " ]" << endl;
}

void RecordType::print(int n) {
  blank(n);
  cout << "RecordType";
  for (auto &field : fields_) {
    field->print(n + 1);
  }
}

void ArrayType::print(int n) {
  blank(n);
  cout << "ArrayType" << endl;
  type_->print(n + 1);
}
