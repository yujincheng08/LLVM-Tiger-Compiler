#include "ast.h"

using namespace AST;

void blank(int n) {
  for (int i = 0; i < n; i++) {
    printf("___");
  }
}

void line() { printf("\n"); }

void Var::print(int n) {
  //  blank(n);
  //  printf("Var");
  //  line();
}

void Exp::print(int n) {
  //  blank(n);
  //  printf("Exp");
  //  line();
}

void Dec::print(int n) {
  //  blank(n);
  //  printf("Dec");
  //  line();

  //  blank(n + 1);
  //  printf("name_:%s", name_.c_str());
  //  line();
}

void Type::print(int n) {
  //  blank(n);
  //  printf("Type");
  //  line();
}

void SimpleVar::print(int n) {
  blank(n);
  printf("SimpleVar");
  line();

  blank(n + 1);
  printf("name_:%s", name_.c_str());
  line();
}

void FieldVar::print(int n) {
  blank(n);
  printf("FieldVar");
  line();

  var_->print(n + 1);

  blank(n + 1);
  printf("field_:%s", field_.c_str());
  line();
}

void SubscriptVar::print(int n) {
  blank(n);
  printf("SubscriptVar");
  line();
  var_->print(n + 1);
  exp_->print(n + 1);
}

void VarExp::print(int n) {
  blank(n);
  printf("VarExp");
  line();
  var_->print(n + 1);
}

void NilExp::print(int n) {
  //  blank(n);
  //  printf("NilExp");
  //  line();
}

void IntExp::print(int n) {
  blank(n);
  printf("IntExp");
  line();

  blank(n + 1);
  printf("val_:%d", val_);
  line();
}

void StringExp::print(int n) {
  blank(n);
  printf("StringExp");
  line();

  blank(n + 1);
  printf("val_:%s", val_.c_str());
  line();
}

void CallExp::print(int n) {
  blank(n);
  printf("CallExp");
  line();

  blank(n + 1);
  printf("func_:%s", func_.c_str());
  line();

  for (auto &arg : args_) {
    arg->print(n + 1);
  }
}

void BinaryExp::print(int n) {
  blank(n);
  printf("BinaryExp");
  line();

  blank(n + 1);
  printf("op_:%c", op_);
  line();

  left_->print(n + 1);
  right_->print(n + 1);
}

void Field::print(int n) {
  blank(n);
  printf("Field");
  line();

  blank(n + 1);
  printf("name_:%s", name_.c_str());
  line();

  type_->print(n + 1);
}

void FieldExp::print(int n) {
  blank(n);
  printf("FieldExp");
  line();

  blank(n + 1);
  printf("name_:%s", name_.c_str());
  line();

  exp_->print(n + 1);
}

void RecordExp::print(int n) {
  blank(n);
  printf("RecordExp");
  line();

  blank(n + 1);
  printf("name_:%s", name_.c_str());
  line();

  for (auto &fieldExp : fieldExps_) {
    fieldExp->print(n + 1);
  }
}

void SequenceExp::print(int n) {
  blank(n);
  printf("SequenceExp");
  line();

  for (auto &exp : exps_) {
    exp->print(n + 1);
  }
}

void AssignExp::print(int n) {
  blank(n);
  printf("AssignExp");
  line();

  var_->print(n + 1);
  exp_->print(n + 1);
}

void IfExp::print(int n) {
  blank(n);
  printf("IfExp");
  line();

  test_->print(n + 1);
  then_->print(n + 1);
  else_->print(n + 1);
}

void WhileExp::print(int n) {
  blank(n);
  printf("WhileExp");
  line();

  test_->print(n + 1);
  body_->print(n + 1);
}

void ForExp::print(int n) {
  blank(n);
  printf("ForExp");
  line();

  blank(n + 1);
  printf("var_:%s", var_.c_str());
  line();

  low_->print(n + 1);
  high_->print(n + 1);
  body_->print(n + 1);
}

void BreakExp::print(int n) {
  blank(n);
  printf("Break");
  line();
}

void LetExp::print(int n) {
  blank(n);
  printf("LetExp");
  line();
  for (auto &dec : decs_) {
    dec->print(n + 1);
  }
  body_->print(n + 1);
}

void ArrayExp::print(int n) {
  blank(n);
  printf("ArrayExp");
  line();

  blank(n + 1);
  printf("type_:%s", type_.c_str());
  line();

  size_->print(n + 1);
  init_->print(n + 1);
}

void Prototype::print(int n) {
  blank(n);
  printf("Prototype");
  line();

  blank(n + 1);
  printf("name_:%s", name_.c_str());
  line();

  for (auto &param : params_) {
    param->print(n + 1);
  }
  blank(n + 1);
  printf("result_:%s", result_.c_str());
  line();
}

void FunctionDec::print(int n) {
  blank(n);
  printf("FunctionDec");
  line();

  blank(n + 1);
  printf("name_:%s", name_.c_str());
  line();

  proto_->print(n + 1);
  body_->print(n + 1);
}

void VarDec::print(int n) {
  blank(n);
  printf("VarDec");
  line();

  blank(n + 1);
  printf("type_:%s", type_.c_str());
  line();

  init_->print(n + 1);
}

void TypeDec::print(int n) {
  blank(n);
  printf("TypeDec");
  line();

  type_->print(n + 1);
}

void NameType::print(int n) {
  blank(n);
  printf("NameType");
  line();

  blank(n + 1);
  printf("name_:%s", name_.c_str());
  line();
}

void RecordType::print(int n) {
  blank(n);
  printf("RecordType");
  line();
  for (auto &field : fields_) {
    field->print(n + 1);
  }
}

void ArrayType::print(int n) {
  blank(n);
  printf("ArrayType");
  line();

  blank(n + 1);
  printf("name_:%s", name_.c_str());
  line();
}
