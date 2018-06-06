#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>
#include <iostream>
#include <unordered_map>
#include "AST/ast.h"

static llvm::LLVMContext context;
static llvm::IRBuilder<> builder(context);
static std::unique_ptr<llvm::Module> module;
static std::unordered_map<std::string, llvm::AllocaInst *> values;
//static std::unordered_map<std::string,

// TODO: FUNCTION STATIC LINK

static llvm::Value *logError(std::string const &msg) {
  std::cerr << msg << std::endl;
  return nullptr;
}

llvm::Value *AST::SimpleVar::codegen() {
  auto var = values[name_];
  if (!var) return logError("Unknown variable name " + name_);
  return builder.CreateLoad(var, name_.c_str());
}

llvm::Value *AST::FieldVar::codegen() {
  auto var = var_->codegen();
  if (!var) return nullptr;  // TODO: Should I log something?
  auto type = var->getType();
  if (!llvm::isa<llvm::StructType>(type))
    return logError(var->getName().str() + " is not a struct type");
  auto *structType = llvm::cast<llvm::StructType>(type);
  structType->elements();
}

llvm::Value *AST::TypeDec::codegen() {}

llvm::Value *AST::BinaryExp::codegen() {
  auto L = left_->codegen();
  auto R = right_->codegen();
  if (!L || !R) return nullptr;

  switch (op_)  // TODO: use enum instead
  {
    case ADD:
      return builder.CreateAdd(L, R, "addtmp");
    case SUB:
      return builder.CreateSub(L, R, "subtmp");
    case MUL:
      return builder.CreateMul(L, R, "multmp");
    case DIV:
      return builder.CreateFPToSI(
          builder.CreateFDiv(
              builder.CreateSIToFP(L, llvm::Type::getDoubleTy(context)),
              builder.CreateSIToFP(R, llvm::Type::getDoubleTy(context)),
              "divftmp"),
          llvm::Type::getInt64Ty(context), "divtmp");
    case LTH:
      return builder.CreateICmpSLT(L, R, "cmptmp");
    case GTH:
      return builder.CreateICmpSGT(L, R, "cmptmp");
    case EQU:
      return builder.CreateICmpEQ(L, R, "cmptmp");
    case NEQU:
      return builder.CreateICmpNE(L, R, "cmptmp");
    case LEQ:
      return builder.CreateICmpSLE(L, R, "cmptmp");
    case GEQ:
      return builder.CreateICmpSGE(L, R, "cmptmp");
    case AND:
      return builder.CreateAnd(L, R, "andtmp");
    case OR:
      return builder.CreateOr(L, R, "ortmp");
    case XOR:
      return builder.CreateXor(L, R, "xortmp");
  }
  assert(false);
}
