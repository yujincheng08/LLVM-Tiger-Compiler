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
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>

#include <iostream>
#include <stack>
#include <tuple>
#include <unordered_map>
#include "AST/ast.h"
#include "utils/symboltable.h"

static llvm::LLVMContext context;
static llvm::IRBuilder<> builder(context);
static std::unique_ptr<llvm::Module> module;
static SymbolTable<llvm::AllocaInst> values;
static SymbolTable<llvm::Function> functions;
static SymbolTable<AST::Type> typeDecs;
static SymbolTable<llvm::Type> types;
static std::stack<
    std::tuple<llvm::BasicBlock * /*next*/, llvm::BasicBlock * /*after*/>>
    loopStack;

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
                                                const std::string &name,
                                                llvm::Value *size = nullptr) {
  llvm::IRBuilder<> TmpB(&function->getEntryBlock(),
                         function->getEntryBlock().begin());
  return TmpB.CreateAlloca(type, size, name.c_str());
}

static llvm::Type *getElementType(llvm::Type *type) {
  return llvm::cast<llvm::PointerType>(type)->getElementType();
}

static bool isNil(llvm::Value *exp) {
  return exp->getType()->isPointerTy() &&
         getElementType(exp->getType())->isVoidTy();
}

static llvm::Value *checkStore(llvm::Value *val, llvm::Value *ptr) {
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

static llvm::Type *typeOf(std::string const &name,
                          std::set<std::string> &parentName) {
  if (auto type = types[name]) return type;
  auto typeDec = typeDecs[name];
  if (!typeDec) logErrorT(name + " is not a type");
  return typeDec->codegen(parentName);
}

static llvm::Type *typeOf(const std::string &name) {
  std::set<std::string> parentName;
  return typeOf(name, parentName);
}

static llvm::Function *createIntrinsicFunction(
    std::string const &name, std::vector<llvm::Type *> const &args,
    llvm::Type *retType) {
  auto functionType = llvm::FunctionType::get(retType, args, false);
  auto function = llvm::Function::Create(
      functionType, llvm::Function::ExternalLinkage, name, module.get());
  return function;
}

static void intrinsic() {
  auto stringType = types["string"];
  auto intType = types["int"];
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

llvm::Value *AST::Root::codegen() {
  module = llvm::make_unique<llvm::Module>("main", context);

  llvm::legacy::PassManager pm;
  pm.add(llvm::createPrintModulePass(llvm::outs()));

  llvm::InitializeAllTargetInfos();
  llvm::InitializeAllTargets();
  llvm::InitializeAllTargetMCs();
  llvm::InitializeAllAsmParsers();
  llvm::InitializeAllAsmPrinters();

  auto targetTriple = llvm::sys::getDefaultTargetTriple();
  module->setTargetTriple(targetTriple);

  std::string error;
  auto target = llvm::TargetRegistry::lookupTarget(targetTriple, error);

  // Print an error and exit if we couldn't find the requested target.
  // This generally occurs if we've forgotten to initialise the
  // TargetRegistry or we have a bogus target triple.
  if (!target) {
    llvm::errs() << error;
    return nullptr;
  }

  auto CPU = "generic";
  auto features = "";

  llvm::TargetOptions opt;
  auto RM = llvm::Optional<llvm::Reloc::Model>();
  auto targetMachine =
      target->createTargetMachine(targetTriple, CPU, features, opt, RM);

  module->setDataLayout(targetMachine->createDataLayout());
  std::vector<llvm::Type *> args;
  auto mainProto = llvm::FunctionType::get(llvm::Type::getInt64Ty(context),
                                           llvm::makeArrayRef(args), false);
  auto mainFunction = llvm::Function::Create(
      mainProto, llvm::GlobalValue::ExternalLinkage, "main", module.get());
  auto block = llvm::BasicBlock::Create(context, "entry", mainFunction);

  types["int"] = llvm::Type::getInt64Ty(context);
  types["string"] =
      llvm::PointerType::getUnqual(llvm::Type::getInt8Ty(context));
  intrinsic();
  builder.SetInsertPoint(block);
  root_->codegen();
  builder.CreateRet(llvm::ConstantInt::get(llvm::Type::getInt64Ty(context),
                                           llvm::APInt(64, 0)));
  // llvm::ReturnInst::Create(context, block);
  std::cout << "Code is generated." << std::endl;

  auto filename = "output.o";
  std::error_code EC;
  llvm::raw_fd_ostream dest(filename, EC, llvm::sys::fs::F_None);

  if (EC) {
    llvm::errs() << "Could not open file: " << EC.message();
    return nullptr;
  }

  // llvm::legacy::PassManager pass;
  auto fileType = llvm::TargetMachine::CGFT_ObjectFile;

  std::cout << "done." << std::endl;
  if (targetMachine->addPassesToEmitFile(pm, dest, fileType)) {
    llvm::errs() << "TheTargetMachine can't emit a file of this type";
    return nullptr;
  }

  pm.run(*module);
  // pass.run(*module);
  dest.flush();

  llvm::outs() << "Wrote " << filename << "\n";
  return nullptr;
}

llvm::Value *AST::SimpleVar::codegen() {
  auto var = values[name_];
  if (!var) return logErrorV("Unknown variable name " + name_);
  return var;
}

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
  if (!low->getType()->isIntegerTy())
    return logErrorV("loop lower bound should be integer");
  auto high = high_->codegen();
  if (!high) return nullptr;
  if (!high->getType()->isIntegerTy())
    return logErrorV("loop higher bound should be integer");
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

  auto EndCond = builder.CreateICmpSLE(builder.CreateLoad(variable, var_), high,
                                       "loopcond");
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

llvm::Value *AST::SequenceExp::codegen() {
  llvm::Value *last = nullptr;
  for (auto &exp : exps_) last = exp->codegen();
  return last;
}

llvm::Value *AST::LetExp::codegen() {
  values.enter();
  types.enter();
  typeDecs.enter();
  functions.enter();
  for (auto &dec : decs_) dec->codegen();
  auto result = body_->codegen();
  functions.exit();
  types.exit();
  typeDecs.exit();
  values.exit();
  return result;
}

llvm::Value *AST::NilExp::codegen() {
  return llvm::ConstantPointerNull::get(
      llvm::PointerType::getUnqual(llvm::Type::getVoidTy(context)));
}

llvm::Value *AST::VarExp::codegen() {
  auto var = var_->codegen();
  if (!var) return nullptr;
  return builder.CreateLoad(var, var->getName());
}

llvm::Value *AST::AssignExp::codegen() {
  auto var = var_->codegen();
  if (!var) return nullptr;
  auto exp = exp_->codegen();
  if (!exp) return nullptr;
  checkStore(exp, var);
  return exp;  // var is a pointer, should not return
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

  function->getBasicBlockList().push_back(elseBB);
  builder.SetInsertPoint(elseBB);

  llvm::Value *elsee;
  if (else_) {
    elsee = else_->codegen();
    if (!elsee) return nullptr;
  }

  builder.CreateBr(mergeBB);
  elseBB = builder.GetInsertBlock();

  function->getBasicBlockList().push_back(mergeBB);
  builder.SetInsertPoint(mergeBB);
  if (thenBB->getType() != elseBB->getType())
    return logErrorV("Require same type in both branch");

  auto PN = builder.CreatePHI(then->getType(), 2, "iftmp");
  PN->addIncoming(then, thenBB);
  PN->addIncoming(elsee, elseBB);

  return PN;
}

llvm::Value *AST::WhileExp::codegen() {
  auto function = builder.GetInsertBlock()->getParent();
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

llvm::Value *AST::ArrayExp::codegen() {
  static auto allocaArrayFunction = createIntrinsicFunction(
      "allocaArray",
      {llvm::Type::getInt64Ty(context), llvm::Type::getInt64Ty(context)},
      llvm::Type::getInt8PtrTy(context));
  auto function = builder.GetInsertBlock()->getParent();
  auto type = typeOf(type_);
  if (!type->isPointerTy()) return logErrorV("Array type required");
  auto eleType = getElementType(type);
  auto size = size_->codegen();
  auto init = init_->codegen();
  auto eleSize = module->getDataLayout().getTypeAllocSize(eleType);
  llvm::Value *arrayPtr = builder.CreateCall(
      allocaArrayFunction,
      std::vector<llvm::Value *>{
          size, llvm::ConstantInt::get(llvm::Type::getInt64Ty(context),
                                       llvm::APInt(64, eleSize))},
      "alloca");
  arrayPtr = builder.CreateBitCast(arrayPtr, type, "array");

  // auto arrayPtr = createEntryBlockAlloca(function, eleType, "arrayPtr",
  // size);
  auto zero = llvm::ConstantInt::get(context, llvm::APInt(64, 0, true));

  std::string indexName = "index";
  auto indexPtr = createEntryBlockAlloca(
      function, llvm::Type::getInt64Ty(context), indexName);
  // before loop:
  builder.CreateStore(zero, indexPtr);

  auto testBB = llvm::BasicBlock::Create(context, "test", function);
  auto loopBB = llvm::BasicBlock::Create(context, "loop", function);
  auto nextBB = llvm::BasicBlock::Create(context, "next", function);
  auto afterBB = llvm::BasicBlock::Create(context, "after", function);

  builder.CreateBr(testBB);

  builder.SetInsertPoint(testBB);

  auto index = builder.CreateLoad(indexPtr, indexName);
  auto EndCond = builder.CreateICmpSLT(index, size, "loopcond");
  // auto loopEndBB = builder.GetInsertBlock();

  // goto after or loop
  builder.CreateCondBr(EndCond, loopBB, afterBB);

  builder.SetInsertPoint(loopBB);

  // loop:
  // variable->addIncoming(low, preheadBB);

  // TODO: check its non-type value
  auto elePtr = builder.CreateGEP(eleType, arrayPtr, index, "elePtr");
  checkStore(init, elePtr);
  // builder.CreateStore(init, elePtr);
  // goto next:
  builder.CreateBr(nextBB);

  // next:
  builder.SetInsertPoint(nextBB);

  auto nextVar = builder.CreateAdd(
      index, llvm::ConstantInt::get(context, llvm::APInt(64, 1)), "nextvar");
  builder.CreateStore(nextVar, indexPtr);

  builder.CreateBr(testBB);

  // after:
  builder.SetInsertPoint(afterBB);

  // variable->addIncoming(next, loopEndBB);

  return arrayPtr;
}

llvm::Value *AST::SubscriptVar::codegen() {
  auto var = var_->codegen();
  if (!var) return nullptr;
  auto type = var->getType();
  if (!type->isPointerTy() || !getElementType(type)->isPointerTy() ||
      getElementType(getElementType(type))->isStructTy())
    return logErrorV("Subscript is only for array type.");
  var = builder.CreateLoad(var, "arrayPtr");
  type = var->getType();
  auto eleType = getElementType(type);
  auto exp = exp_->codegen();
  if (!exp) return nullptr;
  if (exp->getType() != llvm::Type::getInt64Ty(context))
    return logErrorV("Subscript has to be integer value.");
  return builder.CreateGEP(eleType, var, exp, "ptr");
}

llvm::Value *AST::FieldVar::codegen() {
  auto var = var_->codegen();
  if (!var) return nullptr;
  auto type = var->getType();
  if (!type->isPointerTy() || !getElementType(type)->isPointerTy() ||
      !getElementType(getElementType(type))->isStructTy())
    return logErrorV("field reference is only for struct type.");
  var = builder.CreateLoad(var, "structPtr");
  type = var->getType();
  llvm::StructType *eleType =
      llvm::cast<llvm::StructType>(getElementType(type));

  auto typeDec = dynamic_cast<RecordType *>(typeDecs[eleType->getStructName()]);
  assert(typeDec);
  size_t i = 0u;
  for (auto &field : typeDec->fields_)
    if (field->name_ == field_)
      break;
    else
      ++i;
  auto idx = llvm::ConstantInt::get(llvm::Type::getInt64Ty(context),
                                    llvm::APInt(64, i));
  return builder.CreateGEP(typeOf(typeDec->fields_[i]->type_), var, idx, "ptr");
}

llvm::Value *AST::RecordExp::codegen() {
  static auto allocaRecordFunction =
      createIntrinsicFunction("allocaRecord", {llvm::Type::getInt64Ty(context)},
                              llvm::Type::getInt8PtrTy(context));
  builder.GetInsertBlock()->getParent();
  auto type = typeOf(type_);
  if (!type) return nullptr;
  if (!type->isPointerTy()) return logErrorV("Require a struct type");
  auto eleType = getElementType(type);
  if (!eleType->isStructTy()) return logErrorV("Require a struct type");
  auto typeDec = dynamic_cast<RecordType *>(typeDecs[type_]);
  assert(typeDec);
  // auto var = createEntryBlockAlloca(function, type, "record");
  auto size = module->getDataLayout().getTypeAllocSize(eleType);
  llvm::Value *var =
      builder.CreateCall(allocaRecordFunction,
                         llvm::ConstantInt::get(llvm::Type::getInt64Ty(context),
                                                llvm::APInt(64, size)),
                         "alloca");
  var = builder.CreateBitCast(var, type, "record");
  size_t idx = 0u;
  if (typeDec->fields_.size() != fieldExps_.size())
    return logErrorV("Wrong number of fields");
  for (auto &fieldDec : typeDec->fields_) {
    auto &field = fieldExps_[idx];
    if (field->name_ != fieldDec->name_)
      return logErrorV(field->name_ +
                       " is not a field or not on the right position of " +
                       type_);
    auto exp = field->exp_->codegen();
    if (!exp) return nullptr;
    auto elementPtr = builder.CreateGEP(
        typeOf(fieldDec->type_), var,
        llvm::ConstantInt::get(llvm::Type::getInt64Ty(context),
                               llvm::APInt(64, idx)),
        "elementPtr");
    // CHECK NIL
    checkStore(exp, elementPtr);
    // builder.CreateStore(exp, elementPtr);
    ++idx;
  }
  return var;
}

llvm::Type *AST::ArrayType::codegen(std::set<std::string> &parentName) {
  if (types[name_]) return types[name_];
  if (parentName.find(name_) != parentName.end())
    return logErrorT(name_ + " has an endless loop of type define");
  parentName.insert(name_);
  auto type = typeOf(type_, parentName);
  parentName.erase(name_);
  if (!type) return nullptr;
  type = llvm::PointerType::getUnqual(type);
  types[name_] = type;
  return type;
}

llvm::Type *AST::NameType::codegen(std::set<std::string> &parentName) {
  if (auto type = types[name_]) return type;
  if (auto type = types[type_]) {
    types[name_] = type;
    return type;
  }
  if (parentName.find(name_) != parentName.end())
    return logErrorT(name_ + " has an endless loop of type define");
  parentName.insert(name_);
  auto type = typeOf(type_, parentName);
  parentName.erase(name_);
  return type;
}

llvm::Type *AST::RecordType::codegen(std::set<std::string> &parentName) {
  if (types[name_]) return types[name_];
  std::vector<llvm::Type *> types;
  if (parentName.find(name_) != parentName.end()) {
    auto type =
        llvm::PointerType::getUnqual(llvm::StructType::create(context, name_));
    ::types[name_] = type;
    return type;
  }
  parentName.insert(name_);
  for (auto &field : fields_) {
    auto type = typeOf(field->type_, parentName);
    if (!type) return nullptr;
    types.push_back(type);
  }
  parentName.erase(name_);
  if (auto type = ::types[name_]) {
    if (!type->isPointerTy()) return nullptr;
    auto eleType = getElementType(type);
    if (!eleType->isStructTy()) return nullptr;
    llvm::cast<llvm::StructType>(eleType)->setBody(types);
    return type;
  } else {
    type = llvm::PointerType::getUnqual(
        llvm::StructType::create(context, types, name_));
    if (!type) return nullptr;
    ::types[name_] = type;
    return type;
  }
}

llvm::Value *AST::StringExp::codegen() {
  return builder.CreateGlobalStringPtr(val_, "str");
}

llvm::Function *AST::Prototype::codegen() {
  std::vector<llvm::Type *> args;
  for (auto &arg : params_) {
    auto argType = typeOf(arg->type_);
    if (!argType) return nullptr;
    args.push_back(argType);
  }
  llvm::Type *retType;
  if (result_.empty()) {
    retType = llvm::Type::getVoidTy(context);
  } else {
    retType = typeOf(result_);
    if (!retType) return nullptr;
  }
  // auto oldFunc = functions[name_];
  // if (oldFunc) rename(oldFunc->getName().str() + "-");
  auto functionType = llvm::FunctionType::get(retType, args, false);
  auto function = llvm::Function::Create(
      functionType, llvm::Function::InternalLinkage, name_, module.get());

  size_t idx = 0u;
  for (auto &arg : function->args()) arg.setName(params_[idx++]->name_);
  return function;
}

// TODO: Static link
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
    checkStore(&arg, argAlloca);
    // builder.CreateStore(&arg, argAlloca);
    values.push(argName, argAlloca);
  }

  if (auto retVal = body_->codegen()) {
    builder.CreateRet(retVal);
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

llvm::Value *AST::VarDec::codegen() {
  llvm::Function *function = builder.GetInsertBlock()->getParent();
  auto init = init_->codegen();
  if (!init) return nullptr;
  if (values.lookupOne(name_))
    return logErrorV(name_ + " is already defined in this function.");
  llvm::Type *type;
  if (type_.empty())
    type = init->getType();
  else {
    type = typeOf(type_);
    if (!type) return nullptr;
  }
  auto *variable = createEntryBlockAlloca(function, type, name_);
  //  if (isNil(init)) {
  //    if (!type->isStructTy()) {
  //      return logErrorV("Nil can only assign to struct type");
  //    } else {
  //      init =
  //      llvm::ConstantPointerNull::get(llvm::PointerType::getUnqual(type));
  //    }
  //  }
  checkStore(init, variable);
  // builder.CreateStore(init, variable);
  values.push(name_, variable);
  return variable;
}

llvm::Value *AST::TypeDec::codegen() {
  type_->setName(name_);
  typeDecs[name_] = type_.get();
  return llvm::Constant::getNullValue(llvm::Type::getInt64Ty(context));
}

llvm::Value *AST::BinaryExp::codegen() {
  auto L = left_->codegen();
  auto R = right_->codegen();
  if (!L || !R) return nullptr;
  // TODO: check for nil
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
