#include <llvm/IR/Constant.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/IRPrintingPasses.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/raw_ostream.h>
#include <iostream>
#include <stack>
#include <tuple>
#include <unordered_map>
#include "AST/ast.h"
#include "utils/symboltable.h"

static llvm::LLVMContext context;
static llvm::IRBuilder<> builder(context);
static std::unique_ptr<llvm::Module> module;
// static std::unordered_map<std::string, llvm::AllocaInst *> values;
static SymbolTable<llvm::AllocaInst> values;
static SymbolTable<llvm::Function> functions;
// static std::stack<llvm::Function *> functionStack;
// static std::unordered_map<std::string, std::unique_ptr<AST::Type>> types;
static SymbolTable<AST::Type> types;
static std::stack<
    std::tuple<llvm::BasicBlock * /*next*/, llvm::BasicBlock * /*after*/>>
    loopStack;
// static std::map<std::string, std::unique_ptr<AST::Prototype>> FunctionProtos;

// TODO: FUNCTION STATIC LINK USING FUNCTION::SIZE()??

static llvm::Value *logErrorV(std::string const &msg) {
  std::cerr << msg << std::endl;
  return nullptr;
}

static llvm::Type *logErrorT(std::string const &msg) {
  std::cerr << msg << std::endl;
  return nullptr;
}

static llvm::AllocaInst *createEntryBlockAlloca(llvm::Function *function,
                                                llvm::Type *type,
                                                const std::string &name) {
  llvm::IRBuilder<> TmpB(&function->getEntryBlock(),
                         function->getEntryBlock().begin());
  return TmpB.CreateAlloca(type, 0, name.c_str());
}

// static llvm::Type *typeOf(std::string const &name) {
//  auto &type = types[name];
//  if (!type) return logErrorT(name + " is not a type");
//  return type->codegen(name);
//}

llvm::Value *AST::Root::codegen() {
  module = llvm::make_unique<llvm::Module>("main", context);
  std::vector<llvm::Type *> args;
  auto mainProto = llvm::FunctionType::get(llvm::Type::getVoidTy(context),
                                           llvm::makeArrayRef(args), false);
  auto mainFunction = llvm::Function::Create(
      mainProto, llvm::GlobalValue::ExternalLinkage, "main", module.get());
  // types.push("int", llvm::Type::getInt64Ty(context));
  // functionStack.push(mainFunction);
  auto block = llvm::BasicBlock::Create(context, "entry", mainFunction);

  builder.SetInsertPoint(block);
  root_->codegen();
  // llvm::ReturnInst::Create(context, block);
  std::cout << "Code is generated." << std::endl;
  llvm::legacy::PassManager pm;
  pm.add(llvm::createPrintModulePass(llvm::outs()));
  pm.run(*module);
  std::cout << "done." << std::endl;
  return nullptr;
}

llvm::Value *AST::SimpleVar::codegen() {
  auto var = values[name_];
  if (!var) return logErrorV("Unknown variable name " + name_);
  return builder.CreateLoad(var, name_.c_str());
}

llvm::Value *AST::FieldVar::codegen() {
  auto var = var_->codegen();
  if (!var) return nullptr;  // TODO: Should I log something?
  auto type = var->getType();
  if (!llvm::isa<llvm::StructType>(type))
    return logErrorV(var->getName().str() + " is not a struct type");
  auto *structType = llvm::cast<llvm::StructType>(type);
  structType->elements();
  // TODO
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
  auto variable =
      createEntryBlockAlloca(function, llvm::Type::getInt64Ty(context), var_);
  // before loop:
  builder.CreateStore(low, variable);

  auto testBB = llvm::BasicBlock::Create(context, "test", function);
  auto loopBB = llvm::BasicBlock::Create(context, "loop", function);
  auto nextBB = llvm::BasicBlock::Create(context, "next", function);
  auto afterBB = llvm::BasicBlock::Create(context, "after", function);
  loopStack.push({nextBB, afterBB});

  builder.CreateBr(testBB);

  builder.SetInsertPoint(testBB);

  auto EndCond = builder.CreateICmpSLE(variable, high, "loopcond");
  // auto loopEndBB = builder.GetInsertBlock();

  // goto after or loop
  builder.CreateCondBr(EndCond, loopBB, afterBB);

  builder.SetInsertPoint(loopBB);

  // loop:
  // variable->addIncoming(low, preheadBB);

  auto oldVal = values[var_];
  if (oldVal) values.popOne(var_);
  values.push(var_, variable);
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

  builder.CreateBr(testBB);

  // after:
  builder.SetInsertPoint(afterBB);

  // variable->addIncoming(next, loopEndBB);

  if (oldVal)
    values[var_] = oldVal;
  else
    values.popOne(var_);

  loopStack.pop();

  return llvm::Constant::getNullValue(llvm::Type::getInt64Ty(context));
}

llvm::Value *AST::RecordExp::codegen() {}
llvm::Value *AST::SequenceExp::codegen() {
  llvm::Value *last = nullptr;
  for (auto &exp : exps_) last = exp->codegen();
  return last;
}

llvm::Value *AST::LetExp::codegen() {
  values.enter();
  functions.enter();
  for (auto &dec : decs_) dec->codegen();
  auto result = body_->codegen();
  functions.exit();
  values.exit();
  return result;
}

llvm::Value *AST::NilExp::codegen() {
  return llvm::Constant::getNullValue(llvm::Type::getInt64Ty(context));
}

llvm::Value *AST::VarExp::codegen() { return var_->codegen(); }
llvm::Value *AST::AssignExp::codegen() {
  auto var = var_->codegen();
  if (!var) return nullptr;
  auto exp = exp_->codegen();
  if (!exp) return nullptr;
  builder.CreateStore(exp, var);
  return var;
}

llvm::Value *AST::IfExp::codegen() {
  auto test = test_->codegen();
  if (!test) return nullptr;

  test = builder.CreateICmpNE(
      test, llvm::ConstantInt::get(context, llvm::APInt(1, 0)), "iftest");
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

  llvm::Value *elsee;
  if (else_) {
    elsee = else_->codegen();
    if (!elsee) return nullptr;
  }

  builder.CreateBr(mergeBB);
  elseBB = builder.GetInsertBlock();

  // TODO
  function->getBasicBlockList().push_back(mergeBB);
  builder.SetInsertPoint(mergeBB);
  if (thenBB->getType() != elseBB->getType())
    return logErrorV("Require same type in both branch");

  auto PN = builder.CreatePHI(thenBB->getType(), 2, "iftmp");
  PN->addIncoming(then, thenBB);
  PN->addIncoming(elsee, elseBB);

  return PN;
}

llvm::Value *AST::WhileExp::codegen() {
  auto function = builder.GetInsertBlock()->getParent();
  // TODO: it should read only in the body
  auto testBB = llvm::BasicBlock::Create(context, "test", function);
  auto loopBB = llvm::BasicBlock::Create(context, "loop", function);
  auto nextBB = llvm::BasicBlock::Create(context, "next", function);
  auto afterBB = llvm::BasicBlock::Create(context, "after", function);
  loopStack.push({nextBB, afterBB});

  builder.CreateBr(testBB);

  builder.SetInsertPoint(testBB);

  auto test = test_->codegen();
  if (!test) return nullptr;

  auto EndCond = builder.CreateICmpNE(
      test,
      llvm::ConstantInt::get(llvm::Type::getInt64Ty(context),
                             llvm::APInt(1, 0)),
      "loopcond");
  // auto loopEndBB = builder.GetInsertBlock();

  // goto after or loop
  builder.CreateCondBr(EndCond, loopBB, afterBB);

  builder.SetInsertPoint(loopBB);

  // loop:

  if (!body_->codegen()) return nullptr;

  // goto next:
  builder.CreateBr(nextBB);

  // next:
  builder.SetInsertPoint(nextBB);

  builder.CreateBr(testBB);

  // after:
  builder.SetInsertPoint(afterBB);

  // variable->addIncoming(next, loopEndBB);

  return llvm::Constant::getNullValue(llvm::Type::getInt64Ty(context));
}

llvm::Value *AST::CallExp::codegen() {
  auto callee = functions[func_];
  if (!callee) return logErrorV("Unknown function referenced");

  // If argument mismatch error.
  if (callee->arg_size() != args_.size())
    return logErrorV("Incorrect # arguments passed");

  std::vector<llvm::Value *> args;
  for (size_t i = 0u; i != args_.size(); ++i) {
    args.push_back(args_[i]->codegen());
    if (!args.back()) return nullptr;
  }

  return builder.CreateCall(callee, args, "calltmp");
}

llvm::Value *AST::ArrayExp::codegen() {}

llvm::Function *AST::Prototype::codegen() {
  std::vector<llvm::Type *> args;
  for (auto &arg : params_) {
    auto argType = arg->type_->codegen();
    if (!argType) return nullptr;
    args.push_back(argType);
  }
  auto retType = result_->codegen();
  auto oldFunc = functions[name_];
  if (oldFunc) rename(oldFunc->getName().str() + "-");
  auto functionType = llvm::FunctionType::get(retType, args, false);
  auto function = llvm::Function::Create(
      functionType, llvm::Function::InternalLinkage, name_, module.get());

  size_t idx = 0u;
  for (auto &arg : function->args()) arg.setName(params_[idx++]->name_);
  return function;
}

llvm::Value *AST::FunctionDec::codegen() {
  auto function = proto_->codegen();
  if (functions.lookupOne(name_))
    return logErrorV("Function " + name_ +
                     " is already defined in same scope.");
  // auto function = module->getFunction(proto.getName());
  if (!function) return nullptr;
  functions.push(name_, function);

  auto oldBB = builder.GetInsertBlock();
  auto BB = llvm::BasicBlock::Create(context, "entry", function);
  builder.SetInsertPoint(BB);
  values.enter();
  for (auto &arg : function->args()) {
    auto argName = arg.getName();
    auto argAlloca = createEntryBlockAlloca(function, arg.getType(), argName);
    builder.CreateStore(&arg, argAlloca);
    values.push(argName, argAlloca);
  }

  if (auto retVal = body_->codegen()) {
    builder.CreateRet(retVal);
    // TODO: check return value
    llvm::verifyFunction(*function);
    values.exit();
    builder.SetInsertPoint(oldBB);
    return function;
  }
  values.exit();
  function->eraseFromParent();
  functions.popOne(name_);
  builder.SetInsertPoint(oldBB);
  return logErrorV("Function " + name_ + " genteration failed");
}

llvm::Type *AST::NameType::codegen() {
  if (name_ == "int") return llvm::Type::getInt64Ty(context);
  if (types[name_])
    return types[name_]->codegen(name_);
  else
    return logErrorT(name_ + " is not a type");
}

llvm::Type *AST::RecordType::codegen() {}

llvm::Type *AST::ArrayType::codegen() {}

llvm::Type *AST::NameType::codegen(std::string const &parentName) {
  if (name_ == parentName)
    return logErrorT(name_ + " has an endless loop of type define");
  if (name_ == "int") return llvm::Type::getInt64Ty(context);
  if (types[name_])
    return types[name_]->codegen(parentName);
  else
    return logErrorT(name_ + " is not a type");
}

llvm::Type *AST::ArrayType::codegen(string const &) { return codegen(); }
llvm::Type *AST::RecordType::codegen(string const &) { return codegen(); }
llvm::Value *AST::StringExp::codegen() {}
llvm::Value *AST::VarDec::codegen() {
  llvm::Function *functioin = builder.GetInsertBlock()->getParent();
  auto init = init_->codegen();
  if (!init) return nullptr;
  if (values.lookupOne(name_))
    return logErrorV(name_ + " is already defined in this function.");
  llvm::Type *type;
  if (type_ == nullptr)
    type = init->getType();
  else {
    type = type_->codegen();
    if (!type) return nullptr;
  }
  auto *variable = createEntryBlockAlloca(functioin, type, name_);
  builder.CreateStore(init, variable);
  values.push(name_, variable);
  return variable;
}

llvm::Value *AST::TypeDec::codegen() {
  types[name_] = type_.get();
  return llvm::Constant::getNullValue(llvm::Type::getInt64Ty(context));
}

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
    case AND_:
      return builder.CreateAnd(L, R, "andtmp");
    case OR_:
      return builder.CreateOr(L, R, "ortmp");
    case XOR:
      return builder.CreateXor(L, R, "xortmp");
  }
  assert(false);
}
