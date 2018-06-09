#include "ast.h"
#include <llvm/IR/DerivedTypes.h>
#include <utils/symboltable.h>
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

llvm::Type *Root::traverse(vector<string> &, CodeGenContext &context) {
  context.typeDecs.reset();
  return root_->traverse(mainVariableTable_, context);
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

llvm::Type *AST::FieldVar::traverse(vector<std::string> &variableTable,
                                    CodeGenContext &context) {
  //
  auto var = var_->traverse(variableTable, context);
  if (!var) return nullptr;
  if (!context.isRecord(var))
    return context.logErrorT("field reference is only for struct type.");
  llvm::StructType *eleType =
      llvm::cast<llvm::StructType>(context.getElementType(var));

  auto typeDec =
      dynamic_cast<RecordType *>(context.typeDecs[eleType->getStructName()]);
  assert(typeDec);
  idx_ = 0u;
  for (auto &field : typeDec->fields_)
    if (field->getName() == field_)
      break;
    else
      ++idx_;
  type_ = typeDec->fields_[idx_]->getType();
  return type_;
}

void SubscriptVar::print(QTreeWidgetItem *parent, int n) {
  blank(n);
  cout << "SubscriptVar:" << endl;
  QTreeWidgetItem *cur = add_node(parent, "SubscriptVar", icon);

  var_->print(cur, n + 1);
  exp_->print(cur, n + 1);
}

llvm::Type *AST::SubscriptVar::traverse(vector<std::string> &variableTable,
                                        CodeGenContext &context) {
  auto var = var_->traverse(variableTable, context);
  if (!var) return nullptr;
  if (!var->isPointerTy() || !context.getElementType(var)->isPointerTy() ||
      context.getElementType(context.getElementType(var))->isStructTy())
    return context.logErrorT("Subscript is only for array type.");
  // Ptr to element type
  auto type = context.getElementType(var);
  type_ = context.getElementType(type);
  auto exp = exp_->traverse(variableTable, context);
  if (!exp->isIntegerTy())
    return context.logErrorT("Subscript should be integer");
  return type;
}

void VarExp::print(QTreeWidgetItem *parent, int n) {
  blank(n);
  cout << "VarExp:" << endl;
  QTreeWidgetItem *cur = add_node(parent, "VarExp", icon);
  var_->print(cur, n + 1);
}

llvm::Type *AST::VarExp::traverse(vector<std::string> &variableTable,
                                  CodeGenContext &context) {
  return var_->traverse(variableTable, context);
}

void NilExp::print(QTreeWidgetItem *parent, int n) {
  QTreeWidgetItem *cur = add_node(parent, "NilExp", icon);
  blank(n);
  cout << "NilExp:''" << endl;
  Q_UNUSED(cur);
}

llvm::Type *AST::NilExp::traverse(vector<std::string> &,
                                  CodeGenContext &context) {
  return context.nilType;
}

void IntExp::print(QTreeWidgetItem *parent, int n) {
  blank(n);
  cout << "IntExp:" << endl;
  QTreeWidgetItem *cur = add_node(parent, "IntExp", icon);

  blank(n + 1);
  cout << "val: " << val_ << endl;
  add_node(cur, "[val: " + std::to_string(val_) + "]", icon);
}

llvm::Type *AST::IntExp::traverse(vector<std::string> &,
                                  CodeGenContext &context) {
  return context.intType;
}

void StringExp::print(QTreeWidgetItem *parent, int n) {
  blank(n);
  cout << "StringExp:" << endl;
  QTreeWidgetItem *cur = add_node(parent, "StringExp", icon);

  blank(n + 1);
  cout << "val: " << val_ << endl;
  add_node(cur, "[val: " + val_ + "]", icon);
}

llvm::Type *AST::StringExp::traverse(vector<std::string> &,
                                     CodeGenContext &context) {
  return context.stringType;
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

llvm::Type *CallExp::traverse(vector<string> &variableTable,
                              CodeGenContext &context) {
  auto functionDec = context.functionDecs[func_];
  if (!functionDec) return context.logErrorT("Function undeclared");
  auto params = functionDec->params();
  if (args_.size() != params.size())
    return context.logErrorT("Incorrect # arguments passed");

  size_t i = 0u;
  for (auto &exp : args_) {
    auto type = exp->traverse(variableTable, context);
    auto arg = params[i++];
    if (arg != type) return context.logErrorT("Params type not match");
  }
  return functionDec->getReturnType();
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

llvm::Type *Field::traverse(vector<string> &variableTable,
                            CodeGenContext &context) {
  variableTable.push_back(name_);
  type_ = context.typeOf(typeName_);
  return type_;
}

void Field::print(QTreeWidgetItem *parent, int n) {
  blank(n);
  cout << "Field:" << endl;
  QTreeWidgetItem *cur = add_node(parent, "Field", icon);

  blank(n + 1);
  cout << "name: " << name_ << endl;
  add_node(cur, "[name: " + name_ + "]", icon);
  // TODO:
  // type_->print(cur, n + 1);
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

llvm::Type *FieldExp::traverse(vector<string> &variableTable,
                               CodeGenContext &context) {
  type_ = exp_->traverse(variableTable, context);
  return type_;
}

void RecordExp::print(QTreeWidgetItem *parent, int n) {
  blank(n);
  cout << "RecordExp:" << endl;
  QTreeWidgetItem *cur = add_node(parent, "RecordExp", icon);

  // TODO: Print type
  // type_->print(cur, n + 1);
  for (auto &fieldExp : fieldExps_) {
    fieldExp->print(cur, n + 1);
  }
}

llvm::Type *RecordExp::traverse(vector<string> &variableTable,
                                CodeGenContext &context) {
  type_ = context.typeOf(typeName_);
  if (!type_->isPointerTy()) return context.logErrorT("Require a struct type");
  auto eleType = context.getElementType(type_);
  if (!eleType->isStructTy()) return context.logErrorT("Require a struct type");
  auto typeDec = dynamic_cast<RecordType *>(context.typeDecs[typeName_]);
  assert(typeDec);
  if (typeDec->fields_.size() != fieldExps_.size())
    return context.logErrorT("Wrong number of fields");
  size_t idx = 0u;
  for (auto &fieldDec : typeDec->fields_) {
    auto &field = fieldExps_[idx];
    if (field->getName() != fieldDec->getName())
      return context.logErrorT(
          field->getName() +
          " is not a field or not on the right position of " + typeName_);
    auto exp = field->traverse(variableTable, context);
    if (exp != fieldDec->getType())
      return context.logErrorT("Field type not match");
    ++idx;
  }
  // CHECK NIL
  return type_;
}

void SequenceExp::print(QTreeWidgetItem *parent, int n) {
  blank(n);
  cout << "SequenceExp:" << endl;
  QTreeWidgetItem *cur = add_node(parent, "SequenceExp", icon);

  for (auto &exp : exps_) {
    exp->print(cur, n + 1);
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

void AssignExp::print(QTreeWidgetItem *parent, int n) {
  blank(n);
  cout << "AssignExp:" << endl;
  QTreeWidgetItem *cur = add_node(parent, "AssignExp", icon);

  var_->print(cur, n + 1);
  exp_->print(cur, n + 1);
}

llvm::Type *AssignExp::traverse(vector<string> &variableTable,
                                CodeGenContext &context) {
  auto var = var_->traverse(variableTable, context);
  if (!var) return nullptr;
  auto exp = exp_->traverse(variableTable, context);
  if (!exp) return nullptr;
  if (var->isPointerTy() && context.getElementType(var)->isStructTy() &&
      exp->isPointerTy() && context.getElementType(exp)->isStructTy()) {
    if (!llvm::cast<llvm::StructType>(context.getElementType(var))
             ->isLayoutIdentical(
                 llvm::cast<llvm::StructType>(context.getElementType(exp))))
      return context.logErrorT("Two record are not identical");
  } else if (var != exp)
    return context.logErrorT("Assign types do not match");
  // TODO: check type
  return exp;
}

void IfExp::print(QTreeWidgetItem *parent, int n) {
  blank(n);
  cout << "IfExp:" << endl;
  QTreeWidgetItem *cur = add_node(parent, "IfExp", icon);

  test_->print(cur, n + 1);
  then_->print(cur, n + 1);
  if (else_) else_->print(cur, n + 1);
}

llvm::Type *IfExp::traverse(vector<string> &variableTable,
                            CodeGenContext &context) {
  auto test = test_->traverse(variableTable, context);
  if (!test) return nullptr;
  auto then = then_->traverse(variableTable, context);
  if (!then) return nullptr;
  if (!test->isIntegerTy()) return context.logErrorT("Require integer in test");
  if (else_) {
    auto elsee = else_->traverse(variableTable, context);
    if (!elsee) return nullptr;
    if (then != elsee)
      return context.logErrorT("Require same type in both branch");
  } else {
    if (!then->isVoidTy())
      return context.logErrorT("One branch if should be void");
  }
  return then;
}

void WhileExp::print(QTreeWidgetItem *parent, int n) {
  blank(n);
  cout << "WhileExp:" << endl;
  QTreeWidgetItem *cur = add_node(parent, "WhileExp", icon);

  test_->print(cur, n + 1);
  body_->print(cur, n + 1);
}

llvm::Type *WhileExp::traverse(vector<string> &variableTable,
                               CodeGenContext &context) {
  auto test = test_->traverse(variableTable, context);
  if (!test) return nullptr;
  if (!test->isIntegerTy()) return context.logErrorT("Rquire integer");
  body_->traverse(variableTable, context);
  return context.voidType;
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

llvm::Type *ForExp::traverse(vector<string> &variableTable,
                             CodeGenContext &context) {
  auto low = low_->traverse(variableTable, context);
  if (!low) return nullptr;
  auto high = high_->traverse(variableTable, context);
  if (!high) return nullptr;
  auto body = body_->traverse(variableTable, context);
  if (!body) return nullptr;
  if (!low->isIntegerTy() || !high->isIntegerTy())
    return context.logErrorT("For bounds require integer");
  if (!body->isVoidTy()) return context.logErrorT("Loop body should be void");
  variableTable.push_back(var_);
  // TODO: check body void
  return context.voidType;
}

void BreakExp::print(QTreeWidgetItem *parent, int n) {
  blank(n);
  cout << "Break:''" << endl;
  QTreeWidgetItem *cur = add_node(parent, "Break", icon);
  Q_UNUSED(cur);
}

llvm::Type *AST::BreakExp::traverse(vector<std::string> &,
                                    CodeGenContext &context) {
  return context.voidType;
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

llvm::Type *LetExp::traverse(vector<string> &variableTable,
                             CodeGenContext &context) {
  context.types.enter();
  context.typeDecs.enter();
  context.valueDecs.enter();
  context.functionDecs.enter();
  for (auto &dec : decs_) {
    dec->traverse(variableTable, context);
  }
  body_->traverse(variableTable, context);
  context.functionDecs.exit();
  context.valueDecs.exit();
  context.types.exit();
  context.typeDecs.exit();
  return context.voidType;
}

llvm::Type *TypeDec::traverse(vector<string> &, CodeGenContext &context) {
  type_->setName(name_);
  context.typeDecs[name_] = type_.get();
  return context.voidType;
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

void ArrayExp::print(QTreeWidgetItem *parent, int n) {
  blank(n);
  cout << "ArrayExp:" << endl;
  QTreeWidgetItem *cur = add_node(parent, "ArrayExp", icon);

  // TODO
  // type_->print(cur, n + 1);
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
  // TODO
  //  result_->print(cur, n + 1);

  for (auto &param : params_) {
    param->print(cur, n + 1);
  }
}

llvm::FunctionType *Prototype::traverse(vector<string> &variableTable,
                                        CodeGenContext &context) {
  std::vector<llvm::Type *> args;

  for (auto &field : params_) {
    args.push_back(field->traverse(variableTable, context));
    if (!args.back()) return nullptr;
  }
  if (result_.empty()) {
    resultType_ = llvm::Type::getVoidTy(context.context);
  } else {
    resultType_ = context.typeOf(result_);
  }
  if (!resultType_) return nullptr;
  return llvm::FunctionType::get(resultType_, args, false);
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

llvm::Type *FunctionDec::traverse(vector<string> &, CodeGenContext &context) {
  if (context.functionDecs.lookupOne(name_))
    return context.logErrorT("Function " + name_ +
                             " is already defined in same scope.");
  context.valueDecs.enter();
  auto proto = proto_->traverse(variableTable_, context);
  if (!proto) return nullptr;
  context.functionDecs.push(name_, proto);
  auto body = body_->traverse(variableTable_, context);
  if (!body) return nullptr;
  context.valueDecs.exit();
  context.functionDecs.popOne(name_);
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

llvm::Type *VarDec::traverse(vector<string> &variableTable,
                             CodeGenContext &context) {
  offset = variableTable.size();
  variableTable.push_back(name_);
  auto init = init_->traverse(variableTable, context);
  if (typeName_.empty()) {
    type_ = init;
  } else {
    type_ = context.typeOf(typeName_);
    if (context.isNil(init)) {
      if (!context.isRecord(type_))
        return context.logErrorT("Nil is only for record");
    } else if (init != type_)
      return context.logErrorT("Declare type not match");
  }
  if (!type_) return nullptr;
  context.valueDecs.push(name_, this);
  return context.voidType;
}

void VarDec::print(QTreeWidgetItem *parent, int n) {
  blank(n);
  cout << "VarDec:" << endl;
  QTreeWidgetItem *cur = add_node(parent, "VarDec", icon);

  blank(n + 1);
  cout << "name: " << name_ << endl;
  add_node(cur, "[name:" + name_ + "]", icon);
  // TODO
  // if (type_) type_->print(cur, n + 1);
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
  Q_UNUSED(cur);
  // TODO
  // type_->print(cur, n + 1);
}

llvm::Type *AST::ArrayType::traverse(std::set<std::string> &parentName,
                                     CodeGenContext &context) {
  if (context.types[name_]) return context.types[name_];
  if (parentName.find(name_) != parentName.end())
    return context.logErrorT(name_ + " has an endless loop of type define");
  parentName.insert(name_);
  auto type = context.typeOf(type_, parentName);
  parentName.erase(name_);
  if (!type) return nullptr;
  type = llvm::PointerType::getUnqual(type);
  context.types.push(name_, type);
  return type;
}

llvm::Type *AST::NameType::traverse(std::set<std::string> &parentName,
                                    CodeGenContext &context) {
  if (auto type = context.types[name_]) return type;
  if (auto type = context.types[type_]) {
    context.types.push(name_, type);
    return type;
  }
  if (parentName.find(name_) != parentName.end())
    return context.logErrorT(name_ + " has an endless loop of type define");
  parentName.insert(name_);
  auto type = context.typeOf(type_, parentName);
  parentName.erase(name_);
  return type;
}

llvm::Type *AST::RecordType::traverse(std::set<std::string> &parentName,
                                      CodeGenContext &context) {
  if (context.types[name_]) return context.types[name_];
  std::vector<llvm::Type *> types;
  if (parentName.find(name_) != parentName.end()) {
    auto type = llvm::PointerType::getUnqual(
        llvm::StructType::create(context.context, name_));
    context.types.push(name_, type);
    return type;
  }
  parentName.insert(name_);
  for (auto &field : fields_) {
    auto type = context.typeOf(field->typeName_, parentName);
    if (!type) return nullptr;
    field->type_ = type;
    types.push_back(type);
  }
  parentName.erase(name_);
  if (auto type = context.types[name_]) {
    if (!type->isPointerTy()) return nullptr;
    auto eleType = context.getElementType(type);
    if (!eleType->isStructTy()) return nullptr;
    llvm::cast<llvm::StructType>(eleType)->setBody(types);
    return type;
  } else {
    type = llvm::PointerType::getUnqual(
        llvm::StructType::create(context.context, types, name_));
    if (!type) return nullptr;
    context.types.push(name_, type);
    return type;
  }
}
