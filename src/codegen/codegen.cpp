#include <llvm/IR/Constant.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>
#include <iostream>
#include <stack>
#include <tuple>
#include <unordered_map>
#include "AST/ast.h"

static llvm::LLVMContext context;
static llvm::IRBuilder<> builder(context);
static std::unique_ptr<llvm::Module> module;
static std::unordered_map<std::string, llvm::AllocaInst *> values;
static std::unordered_map<std::string, llvm::Type *> types;
static std::stack<
    std::tuple<llvm::BasicBlock * /*next*/, llvm::BasicBlock * /*after*/>>
    loopStack;

// TODO: FUNCTION STATIC LINK USING FUNCTION::SIZE()??

static llvm::Value *logError(std::string const &msg) {
  std::cerr << msg << std::endl;
  return nullptr;
}

static llvm::AllocaInst *createEntryBlockAlloca(llvm::Function *function,
                                                const std::string &name) {
  llvm::IRBuilder<> TmpB(&function->getEntryBlock(),
                         function->getEntryBlock().begin());
  return TmpB.CreateAlloca(llvm::Type::getDoubleTy(context), 0, name.c_str());
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
  //TODO
}

llvm::Value *AST::SubscriptVar::codegen() {}

llvm::Value *AST::IntExp::codegen() {
  return llvm::ConstantInt::get(context, llvm::APInt(64, val_));
}

/* // TODO: continue
llvm::Value *AST::ContinueExp::codegen() {
  builder.CreateBr(std::get<0>(loopStacks.top()));
  return llvm::Constant::getNullValue(
      llvm::Type::getInt64Ty(context));  // return nothing
}
*/

llvm::Value *AST::BreakExp::codegen() {
  builder.CreateBr(std::get<1>(loopStack.top()));
  return llvm::Constant::getNullValue(
      llvm::Type::getInt64Ty(context));  // return nothing
}

llvm::Value *AST::ForExp::codegen() {
  auto low = low_->codegen();
  if (!low) return nullptr;
  auto high = high_->codegen();
  if (!high) return nullptr;
  auto function = builder.GetInsertBlock()->getParent();
  // TODO: it should read only in the body
  auto variable = createEntryBlockAlloca(function, var_);
  // before loop:
  builder.CreateStore(low, variable);

  auto loopBB = llvm::BasicBlock::Create(context, "loop", function);
  auto nextBB = llvm::BasicBlock::Create(context, "next", function);
  auto afterBB = llvm::BasicBlock::Create(context, "after", function);
  loopStack.push({nextBB, afterBB});

  // goto loop
  builder.CreateBr(loopBB);

  builder.SetInsertPoint(loopBB);

  // loop:
  // variable->addIncoming(low, preheadBB);

  auto oldVal = values[var_];
  values[var_] = variable;
  // TODO: check its non-type value
  if (!body_->codegen()) return nullptr;

  // goto next:
  builder.CreateBr(nextBB);

  // next:
  builder.SetInsertPoint(nextBB);

  auto nextVar = builder.CreateAdd(
      builder.CreateLoad(variable, var_),
      llvm::ConstantInt::get(context, llvm::APInt(64, 1)), "nextvar");
  builder.CreateStore(nextVar, variable);

  auto EndCond = builder.CreateICmpSLE(nextVar, high, "loopcond");
  // auto loopEndBB = builder.GetInsertBlock();

  // goto after or loop
  builder.CreateCondBr(EndCond, loopBB, afterBB);

  // after:
  builder.SetInsertPoint(afterBB);

  // variable->addIncoming(next, loopEndBB);

  if (oldVal)
    values[var_] = oldVal;
  else
    values.erase(var_);

  loopStack.pop();

  return llvm::Constant::getNullValue(llvm::Type::getInt64Ty(context));
}

llvm::Value *AST::RecordExp::codegen() {}
llvm::Value *AST::SequenceExp::codegen() {}
llvm::Value *AST::LetExp::codegen() {}
llvm::Value *AST::NilExp::codegen() {}
llvm::Value *AST::VarExp::codegen() {}
llvm::Value *AST::AssignExp::codegen() {}

llvm::Value *AST::IfExp::codegen() {
  auto test = test_->codegen();
  if (!test) return nullptr;

  test = builder.CreateICmpNE(
      test, llvm::ConstantInt::get(context, llvm::APInt(64, 0)), "iftest");
  auto function = builder.GetInsertBlock()->getParent();

  auto thenBB = llvm::BasicBlock::Create(context, "then", function);
  auto elseBB = llvm::BasicBlock::Create(context, "else");
  auto mergeBB = llvm::BasicBlock::Create(context, "ifcont");

  builder.CreateCondBr(test, thenBB, elseBB);

  builder.SetInsertPoint(thenBB);

  auto then = then_->codegen();
  if (!then) return nullptr;
  builder.CreateBr(mergeBB);

  thenBB = builder.GetInsertBlock();

  // TODO: how about branch without a function
  function->getBasicBlockList().push_back(elseBB);
  builder.SetInsertPoint(elseBB);

  auto elsee = else_->codegen();
  if (!elsee) return nullptr;

  builder.CreateBr(mergeBB);
  elseBB = builder.GetInsertBlock();

  // TODO
  function->getBasicBlockList().push_back(mergeBB);
  builder.SetInsertPoint(mergeBB);
  if (thenBB->getType() != elseBB->getType())
    return logError("Require same type in both branch");

  auto PN = builder.CreatePHI(thenBB->getType(), 2, "iftmp");
  PN->addIncoming(then, thenBB);
  PN->addIncoming(elsee, elseBB);

  return PN;
}

llvm::Value *AST::WhileExp::codegen() {}
llvm::Value *AST::CallExp::codegen() {}
llvm::Value *AST::ArrayExp::codegen() {}
llvm::Value *AST::FunctionDec::codegen() {}
llvm::Type *AST::NameType::codegen() {}
llvm::Type *AST::ArrayType::codegen() {}
llvm::Type *AST::RecordType::codegen() {
}
llvm::Value *AST::StringExp::codegen() {
}
llvm::Value *AST::VarDec::codegen() {}

llvm::Value *AST::TypeDec::codegen() {}

llvm::Value *AST::BinaryExp::codegen() {
  auto L = left_->codegen();
  auto R = right_->codegen();
  if (!L || !R) return nullptr;

  switch (op_) {
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
