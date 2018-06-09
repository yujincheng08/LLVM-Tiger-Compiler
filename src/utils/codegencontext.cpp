#include "codegencontext.h"
#include <iostream>

CodeGenContext::CodeGenContext() {}

void CodeGenContext::intrinsic() {
  auto stringType =
      llvm::PointerType::getUnqual(llvm::Type::getInt8Ty(context));
  auto intType = llvm::Type::getInt64Ty(context);
  auto voidType = llvm::Type::getVoidTy(context);
  functions["print"] = createIntrinsicFunction("print", {stringType}, voidType);
  functions["printd"] = createIntrinsicFunction("printd", {intType}, voidType);
  functions["flush"] = createIntrinsicFunction("flush", {}, voidType);
  functions["getchar"] = createIntrinsicFunction("getchar_", {}, stringType);
  functions["ord"] = createIntrinsicFunction("ord", {stringType}, intType);
  functions["chr"] = createIntrinsicFunction("chr", {intType}, stringType);
  functions["size"] = createIntrinsicFunction("size", {stringType}, intType);
  functions["substring"] = createIntrinsicFunction(
      "substring", {stringType, intType, intType}, stringType);
  functions["concat"] =
      createIntrinsicFunction("concat", {stringType, stringType}, stringType);
  functions["not"] = createIntrinsicFunction("not_", {intType}, intType);
  functions["exit"] = createIntrinsicFunction("exit_", {intType}, voidType);
}

llvm::Function *CodeGenContext::createIntrinsicFunction(
    std::string const &name, std::vector<llvm::Type *> const &args,
    llvm::Type *retType) {
  auto functionType = llvm::FunctionType::get(retType, args, false);
  auto function = llvm::Function::Create(
      functionType, llvm::Function::ExternalLinkage, name, module.get());
  return function;
}

llvm::Value *CodeGenContext::checkStore(llvm::Value *val, llvm::Value *ptr) {
  if (isNil(val)) {
    auto type = ptr->getType();
    if ((type->isPointerTy() && getElementType(type)->isPointerTy() &&
         getElementType(getElementType(type))->isStructTy())) {
      val = llvm::ConstantPointerNull::get(
          llvm::cast<llvm::PointerType>(getElementType(type)));
    } else {
      return logErrorV("Nil can only assign to struct type");
    }
  }
  return builder.CreateStore(val, ptr);
}

llvm::Value *CodeGenContext::logErrorV(std::string const &msg) {
  std::cerr << msg << std::endl;
  return nullptr;
}

llvm::AllocaInst *CodeGenContext::createEntryBlockAlloca(
    llvm::Function *function, llvm::Type *type, const std::string &name,
    llvm::Value *size) {
  llvm::IRBuilder<> TmpB(&function->getEntryBlock(),
                         function->getEntryBlock().begin());
  return TmpB.CreateAlloca(type, size, name.c_str());
}

llvm::Type *CodeGenContext::getElementType(llvm::Type *type) {
  return llvm::cast<llvm::PointerType>(type)->getElementType();
}

bool CodeGenContext::isNil(llvm::Value *exp) {
  return exp->getType()->isPointerTy() &&
         getElementType(exp->getType())->isVoidTy();
}

llvm::Type *CodeGenContext::logErrorT(std::string const &msg) {
  std::cerr << msg << std::endl;
  return nullptr;
}

llvm::Type *CodeGenContext::typeOf(std::string const &name,
                                   std::set<std::string> &parentName) {
  if (auto type = types[name]) return type;
  auto typeDec = typeDecs[name];
  if (!typeDec) logErrorT(name + " is not a type");
  return typeDec->codegen(parentName, *this);
}

llvm::Type *CodeGenContext::typeOf(const std::string &name) {
  std::set<std::string> parentName;
  return typeOf(name, parentName);
}
