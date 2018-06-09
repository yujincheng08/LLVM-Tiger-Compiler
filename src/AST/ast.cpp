#include "ast.h"
#include <llvm/IR/DerivedTypes.h>
#include <utils/symboltable.h>
#include <iostream>

using namespace AST;
using namespace std;

static SymbolTable<AST::Type> typeDecs;
static SymbolTable<llvm::Type> types;

// static llvm::Type *logError(std::string const &msg) {
//  std::cerr << msg << std::endl;
//  return nullptr;
//}

// static llvm::Type *typeOf(std::string const &name,
//                          std::set<std::string> &parentName) {
//  if (auto type = types[name]) return type;
//  auto typeDec = typeDecs[name];
//  if (!typeDec) logError(name + " is not a type");
//  return typeDec->codegen(parentName, *this);
//}

// static llvm::Type *typeOf(const std::string &name) {
//  std::set<std::string> parentName;
//  return typeOf(name, parentName);
//}

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

llvm::Type *Root::traverse(vector<string> &, CodeGenContext &context) {
  typeDecs.reset();
  return root_->traverse(mainVariableTable_, context);
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

llvm::Type *CallExp::traverse(vector<string> &variableTable,
                              CodeGenContext &context) {
  for (auto &exp : args_) {
    exp->traverse(variableTable, context);
  }
}

void BinaryExp::print(int n) {
  blank(n);
  cout << "BinaryExp"
       << " [ op: " << (char)op_ << " ]" << endl;
  left_->print(n + 1);
  right_->print(n + 1);
}

llvm::Type *BinaryExp::traverse(vector<string> &variableTable,
                                CodeGenContext &context) {
  auto left = left_->traverse(variableTable, context);
  auto right = right_->traverse(variableTable, context);
  if (!left || !right) return nullptr;
  if (left->isIntegerTy() && right->isIntegerTy())
    return context.intType;
  else
    return nullptr;
}

void Field::print(int n) {
  blank(n);
  cout << "Field"
       << " [ name: " << name_ << ", type:  [" << typeName_ << " ]" << endl;
}

llvm::Type *Field::traverse(vector<string> &variableTable,
                            CodeGenContext &context) {
  variableTable.push_back(name_);
  type_ = context.typeOf(typeName_);
  return type_;
}

void FieldExp::print(int n) {
  blank(n);
  cout << "FieldExp"
       << " [ name: " << name_ << " ]" << endl;
  exp_->print(n + 1);
}

llvm::Type *FieldExp::traverse(vector<string> &variableTable,
                               CodeGenContext &context) {
  exp_->traverse(variableTable, context);
}

void RecordExp::print(int n) {
  blank(n);
  cout << "RecordExp"
       << " [ type: " << typeName_ << " ]" << endl;
  for (auto &fieldExp : fieldExps_) {
    fieldExp->print(n + 1);
  }
}

llvm::Type *RecordExp::traverse(vector<string> &variableTable,
                                CodeGenContext &context) {
  type_ = context.typeOf(typeName_);
  for (auto &fieldExp : fieldExps_) {
    fieldExp->traverse(variableTable, context);
  }
  return type_;
}

void SequenceExp::print(int n) {
  blank(n);
  cout << "SequenceExp" << endl;

  for (auto &exp : exps_) {
    exp->print(n + 1);
  }
}

llvm::Type *SequenceExp::traverse(vector<string> &variableTable,
                                  CodeGenContext &context) {
  llvm::Type *last;
  for (auto &exp : exps_) {
    last = exp->traverse(variableTable, context);
  }
  return last;
}

void AssignExp::print(int n) {
  blank(n);
  cout << "AssignExp" << endl;

  var_->print(n + 1);
  exp_->print(n + 1);
}

llvm::Type *AssignExp::traverse(vector<string> &variableTable,
                                CodeGenContext &context) {
  auto var = var_->traverse(variableTable, context);
  auto exp = exp_->traverse(variableTable, context);
  // TODO: check type
  return exp;
}

void IfExp::print(int n) {
  blank(n);
  cout << "IfExp" << endl;

  test_->print(n + 1);
  then_->print(n + 1);
  if (else_) else_->print(n + 1);
}

llvm::Type *IfExp::traverse(vector<string> &variableTable,
                            CodeGenContext &context) {
  auto test = test_->traverse(variableTable, context);
  auto then = then_->traverse(variableTable, context);
  auto elsee = else_->traverse(variableTable, context);
  if (!test->isIntegerTy()) return context.logErrorT("Require integer in test");
  if (then != elsee)
    return context.logErrorT("Require same type in both branch");
  return then;
}

void WhileExp::print(int n) {
  blank(n);
  cout << "WhileExp" << endl;

  test_->print(n + 1);
  body_->print(n + 1);
}

llvm::Type *WhileExp::traverse(vector<string> &variableTable,
                               CodeGenContext &context) {
  auto test = test_->traverse(variableTable, context);
  if (!test->isIntegerTy()) return context.logErrorT("Rquire integer");
  body_->traverse(variableTable, context);
  return context.voidType;
}

void ForExp::print(int n) {
  blank(n);
  cout << "ForExp"
       << " [ var: " << var_ << " ]" << endl;

  low_->print(n + 1);
  high_->print(n + 1);
  body_->print(n + 1);
}

llvm::Type *ForExp::traverse(vector<string> &variableTable,
                             CodeGenContext &context) {
  auto low = low_->traverse(variableTable, context);
  auto high = high_->traverse(variableTable, context);
  auto body = body_->traverse(variableTable, context);
  if (!low->isIntegerTy() || !high->isIntegerTy())
    return context.logErrorT("For bounds require integer");
  variableTable.push_back(var_);
  // TODO: check body void
  return context.voidType;
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

llvm::Type *LetExp::traverse(vector<string> &variableTable,
                             CodeGenContext &context) {
  types.enter();
  typeDecs.enter();
  for (auto &dec : decs_) {
    dec->traverse(variableTable, context);
  }
  body_->traverse(variableTable, context);
  types.exit();
  typeDecs.exit();
  return context.voidType;
}

llvm::Type *TypeDec::traverse(vector<string> &, CodeGenContext &context) {
  type_->setName(name_);
  typeDecs[name_] = type_.get();
  return context.voidType;
}

void ArrayExp::print(int n) {
  blank(n);
  cout << "ArrayExp"
       << " type: [ " << typeName_ << " ]" << endl;

  size_->print(n + 1);
  init_->print(n + 1);
}

llvm::Type *ArrayExp::traverse(vector<string> &variableTable,
                               CodeGenContext &context) {
  type_ = context.typeOf(typeName_);
  if (!type_) return nullptr;
  if (!type_->isPointerTy()) return context.logErrorT("Array type required");
  auto eleType = context.getElementType(type_);
  auto init = init_->traverse(variableTable, context);
  if (!init) return nullptr;
  auto size = size_->traverse(variableTable, context);
  if (!size) return nullptr;
  if (!size->isIntegerTy()) return context.logErrorT("Size should be integer");
  if (eleType != init) return context.logErrorT("Initial type not matches");
  return type_;
}

void Prototype::print(int n) {
  blank(n);
  cout << "Prototype"
       << " [ name: " << name_ << ", result: " << result_ << " ]" << endl;

  for (auto &param : params_) {
    param->print(n + 1);
  }
}

llvm::Type *Prototype::traverse(vector<string> &variableTable,
                                CodeGenContext &context) {
  for (auto &field : params_) {
    field->traverse(variableTable, context);
  }
  if (result_.empty()) {
    resultType_ = llvm::Type::getVoidTy(context.context);
  } else {
    resultType_ = context.typeOf(result_);
  }
  return resultType_;
}

void FunctionDec::print(int n) {
  blank(n);
  cout << "FunctionDec"
       << " [ name: " << name_ << " ]" << endl;

  proto_->print(n + 1);
  body_->print(n + 1);
}

llvm::Type *FunctionDec::traverse(vector<string> &, CodeGenContext &context) {
  auto proto = proto_->traverse(variableTable_, context);
  auto body = body_->traverse(variableTable_, context);
  if (!proto->isVoidTy() && proto != body)
    return context.logErrorT("Function retrun type not match");
  return proto;
}

llvm::Type *SimpleVar::traverse(vector<string> &, CodeGenContext &context) {
  // TODO: check
  auto type = context.valueDecs[name_];
  if (!type) return context.logErrorT(name_ + " is not defined");
  return type->getType();
}

void VarDec::print(int n) {
  blank(n);
  cout << "VarDec"
       << " [ name: " << name_ << ", type: [ " << name_ << " ]" << endl;
  init_->print(n + 1);
}

llvm::Type *VarDec::traverse(vector<string> &variableTable,
                             CodeGenContext &context) {
  offset = variableTable.size();
  variableTable.push_back(name_);
  auto init = init_->traverse(variableTable, context);
  if (typeName_.empty()) {
    type_ = init;
  } else {
    type_ = context.typeOf(typeName_);
    if (init != type_) return context.logErrorT("Declare type not match");
  }
  return context.voidType;
}

void TypeDec::print(int n) {
  blank(n);
  cout << "TypeDec" << endl;
  type_->print(n + 1);
}

void NameType::print(int n) {
  blank(n);
  cout << "NameType"
       << " [ name: " << type_ << " ]" << endl;
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
  cout << "ArrayType"
       << " [ type: " << type_ << " ]" << endl;
}

llvm::Type *AST::ArrayType::codegen(std::set<std::string> &parentName,
                                    CodeGenContext &context) {
  if (types[name_]) return types[name_];
  if (parentName.find(name_) != parentName.end())
    return context.logErrorT(name_ + " has an endless loop of type define");
  parentName.insert(name_);
  auto type = context.typeOf(type_, parentName);
  parentName.erase(name_);
  if (!type) return nullptr;
  type = llvm::PointerType::getUnqual(type);
  types[name_] = type;
  return type;
}

llvm::Type *AST::NameType::codegen(std::set<std::string> &parentName,
                                   CodeGenContext &context) {
  if (auto type = types[name_]) return type;
  if (auto type = types[type_]) {
    types[name_] = type;
    return type;
  }
  if (parentName.find(name_) != parentName.end())
    return context.logErrorT(name_ + " has an endless loop of type define");
  parentName.insert(name_);
  auto type = context.typeOf(type_, parentName);
  parentName.erase(name_);
  return type;
}

llvm::Type *AST::RecordType::codegen(std::set<std::string> &parentName,
                                     CodeGenContext &context) {
  if (types[name_]) return types[name_];
  std::vector<llvm::Type *> types;
  if (parentName.find(name_) != parentName.end()) {
    auto type = llvm::PointerType::getUnqual(
        llvm::StructType::create(context.context, name_));
    ::types[name_] = type;
    return type;
  }
  parentName.insert(name_);
  for (auto &field : fields_) {
    auto type = context.typeOf(field->typeName_, parentName);
    if (!type) return nullptr;
    types.push_back(type);
  }
  parentName.erase(name_);
  if (auto type = ::types[name_]) {
    if (!type->isPointerTy()) return nullptr;
    auto eleType = context.getElementType(type);
    if (!eleType->isStructTy()) return nullptr;
    llvm::cast<llvm::StructType>(eleType)->setBody(types);
    return type;
  } else {
    type = llvm::PointerType::getUnqual(
        llvm::StructType::create(context.context, types, name_));
    if (!type) return nullptr;
    ::types[name_] = type;
    return type;
  }
}
