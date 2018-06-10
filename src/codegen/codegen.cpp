#include <utils/codegencontext.h>
#include <iostream>
#include <stack>
#include <tuple>
#include <unordered_map>
#include "AST/ast.h"

llvm::Value *AST::Root::codegen(CodeGenContext &context) {
  // clear context.builder and context;
  llvm::legacy::PassManager pm;
  pm.add(llvm::createPrintModulePass(llvm::outs()));

  llvm::InitializeAllTargetInfos();
  llvm::InitializeAllTargets();
  llvm::InitializeAllTargetMCs();
  llvm::InitializeAllAsmParsers();
  llvm::InitializeAllAsmPrinters();

  auto targetTriple = llvm::sys::getDefaultTargetTriple();
  context.module->setTargetTriple(targetTriple);

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

  context.module->setDataLayout(targetMachine->createDataLayout());
  std::vector<llvm::Type *> args;
  auto mainProto = llvm::FunctionType::get(
      llvm::Type::getInt64Ty(context.context), llvm::makeArrayRef(args), false);
  auto mainFunction =
      llvm::Function::Create(mainProto, llvm::GlobalValue::ExternalLinkage,
                             "main", context.module.get());
  context.staticLink.push_front(
      llvm::StructType::create(context.context, "main"));
  auto block = llvm::BasicBlock::Create(context.context, "entry", mainFunction);
  context.types["int"] = context.intType;
  context.types["string"] = context.stringType;
  context.intrinsic();
  traverse(mainVariableTable_, context);
  if (context.hasError) return nullptr;
  context.valueDecs.reset();
  context.functionDecs.reset();
  context.builder.SetInsertPoint(block);
  std::vector<llvm::Type *> localVar;
  for (auto &var : mainVariableTable_) {
    localVar.push_back(var->getType());
    context.valueDecs.push(var->getName(), var);
  }
  context.staticLink.front()->setBody(localVar);
  context.currentFrame = context.createEntryBlockAlloca(
      mainFunction, context.staticLink.front(), "mainframe");
  root_->codegen(context);
  context.builder.CreateRet(llvm::ConstantInt::get(
      llvm::Type::getInt64Ty(context.context), llvm::APInt(64, 0)));
  if (llvm::verifyFunction(*mainFunction, &llvm::errs())) {
    return context.logErrorV("Generate fail");
  }
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

  pm.run(*context.module);
  // pass.run(*module);
  dest.flush();

  llvm::outs() << "Wrote " << filename << "\n";
  return nullptr;
}

llvm::Value *AST::SimpleVar::codegen(CodeGenContext &context) {
  auto var = context.valueDecs[name_];
  if (!var) return context.logErrorV("Unknown variable name " + name_);
  return var->read(context);
}

llvm::Value *AST::IntExp::codegen(CodeGenContext &context) {
  return llvm::ConstantInt::get(context.context, llvm::APInt(64, val_));
}

/* // TODO: continue
llvm::Value *AST::ContinueExp::codegen(CodeGenContext &context) {
  context.builder.CreateBr(std::get<0>(loopStacks.top()));
  return llvm::Constant::getNullValue(
      llvm::Type::getInt64Ty(context));  // return nothing
}
*/

llvm::Value *AST::BreakExp::codegen(CodeGenContext &context) {
  context.builder.CreateBr(std::get<1>(context.loopStack.top()));
  return llvm::Constant::getNullValue(
      llvm::Type::getInt64Ty(context.context));  // return nothing
}

llvm::Value *AST::ForExp::codegen(CodeGenContext &context) {
  auto low = low_->codegen(context);
  if (!low) return nullptr;
  if (!low->getType()->isIntegerTy())
    return context.logErrorV("loop lower bound should be integer");
  auto high = high_->codegen(context);
  if (!high) return nullptr;
  if (!high->getType()->isIntegerTy())
    return context.logErrorV("loop higher bound should be integer");
  auto function = context.builder.GetInsertBlock()->getParent();
  // TODO: it should read only in the body
  // auto variable = context.createEntryBlockAlloca(
  // function, llvm::Type::getInt64Ty(context.context), var_);
  // before loop:
  context.builder.CreateStore(low, varDec_->read(context));

  auto testBB = llvm::BasicBlock::Create(context.context, "test", function);
  auto loopBB = llvm::BasicBlock::Create(context.context, "loop", function);
  auto nextBB = llvm::BasicBlock::Create(context.context, "next", function);
  auto afterBB = llvm::BasicBlock::Create(context.context, "after", function);
  context.loopStack.push({nextBB, afterBB});

  context.builder.CreateBr(testBB);

  context.builder.SetInsertPoint(testBB);

  auto EndCond = context.builder.CreateICmpSLE(
      context.builder.CreateLoad(varDec_->read(context), var_), high,
      "loopcond");
  // auto loopEndBB = context.builder.GetInsertBlock();

  // goto after or loop
  context.builder.CreateCondBr(EndCond, loopBB, afterBB);

  context.builder.SetInsertPoint(loopBB);

  // loop:
  // variable->addIncoming(low, preheadBB);

  auto oldVal = context.valueDecs[var_];
  if (oldVal) context.valueDecs.popOne(var_);
  context.valueDecs.push(var_, varDec_);
  // TODO: check its non-type value
  if (!body_->codegen(context)) return nullptr;

  // goto next:
  context.builder.CreateBr(nextBB);

  // next:
  context.builder.SetInsertPoint(nextBB);

  auto nextVar = context.builder.CreateAdd(
      context.builder.CreateLoad(varDec_->read(context), var_),
      llvm::ConstantInt::get(context.context, llvm::APInt(64, 1)), "nextvar");
  context.builder.CreateStore(nextVar, varDec_->read(context));

  context.builder.CreateBr(testBB);

  // after:
  context.builder.SetInsertPoint(afterBB);

  // variable->addIncoming(next, loopEndBB);

  if (oldVal)
    context.valueDecs[var_] = oldVal;
  else
    context.valueDecs.popOne(var_);

  context.loopStack.pop();

  return llvm::Constant::getNullValue(llvm::Type::getInt64Ty(context.context));
}

llvm::Value *AST::SequenceExp::codegen(CodeGenContext &context) {
  llvm::Value *last = nullptr;
  for (auto &exp : exps_) last = exp->codegen(context);
  return last;
}

llvm::Value *AST::LetExp::codegen(CodeGenContext &context) {
  context.valueDecs.enter();
  context.functionDecs.enter();
  for (auto &dec : decs_) dec->codegen(context);
  auto result = body_->codegen(context);
  context.functionDecs.exit();
  context.valueDecs.exit();
  return result;
}

llvm::Value *AST::NilExp::codegen(CodeGenContext &context) {
  return llvm::ConstantPointerNull::get(
      llvm::PointerType::getUnqual(llvm::Type::getVoidTy(context.context)));
}

llvm::Value *AST::VarExp::codegen(CodeGenContext &context) {
  auto var = var_->codegen(context);
  if (!var) return nullptr;
  return context.builder.CreateLoad(var, var->getName());
}
llvm::Value *AST::AssignExp::codegen(CodeGenContext &context) {
  auto var = var_->codegen(context);
  if (!var) return nullptr;
  auto exp = exp_->codegen(context);
  if (!exp) return nullptr;
  context.checkStore(exp, var);
  return exp;  // var is a pointer, should not return
}

llvm::Value *AST::IfExp::codegen(CodeGenContext &context) {
  auto test = test_->codegen(context);
  if (!test) return nullptr;

  test = context.builder.CreateICmpNE(test, context.zero, "iftest");
  auto function = context.builder.GetInsertBlock()->getParent();

  auto thenBB = llvm::BasicBlock::Create(context.context, "then", function);
  auto elseBB = llvm::BasicBlock::Create(context.context, "else");
  auto mergeBB = llvm::BasicBlock::Create(context.context, "ifcont");

  context.builder.CreateCondBr(test, thenBB, elseBB);

  context.builder.SetInsertPoint(thenBB);

  auto then = then_->codegen(context);
  if (!then) return nullptr;
  context.builder.CreateBr(mergeBB);

  thenBB = context.builder.GetInsertBlock();

  function->getBasicBlockList().push_back(elseBB);
  context.builder.SetInsertPoint(elseBB);

  llvm::Value *elsee;
  if (else_) {
    elsee = else_->codegen(context);
    if (!elsee) return nullptr;
  }

  context.builder.CreateBr(mergeBB);
  elseBB = context.builder.GetInsertBlock();

  function->getBasicBlockList().push_back(mergeBB);
  context.builder.SetInsertPoint(mergeBB);

  if (else_ && !then->getType()->isVoidTy() && !elsee->getType()->isVoidTy()) {
    auto PN = context.builder.CreatePHI(then->getType(), 2, "iftmp");
    PN->addIncoming(then, thenBB);
    PN->addIncoming(elsee, elseBB);

    return PN;
  } else {
    return llvm::Constant::getNullValue(
        llvm::Type::getInt64Ty(context.context));
  }
}

llvm::Value *AST::WhileExp::codegen(CodeGenContext &context) {
  auto function = context.builder.GetInsertBlock()->getParent();
  auto testBB = llvm::BasicBlock::Create(context.context, "test", function);
  auto loopBB = llvm::BasicBlock::Create(context.context, "loop", function);
  auto nextBB = llvm::BasicBlock::Create(context.context, "next", function);
  auto afterBB = llvm::BasicBlock::Create(context.context, "after", function);
  context.loopStack.push({nextBB, afterBB});

  context.builder.CreateBr(testBB);

  context.builder.SetInsertPoint(testBB);

  auto test = test_->codegen(context);
  if (!test) return nullptr;

  auto EndCond = context.builder.CreateICmpNE(
      test,
      llvm::ConstantInt::get(llvm::Type::getInt64Ty(context.context),
                             llvm::APInt(1, 0)),
      "loopcond");
  // auto loopEndBB = context.builder.GetInsertBlock();

  // goto after or loop
  context.builder.CreateCondBr(EndCond, loopBB, afterBB);

  context.builder.SetInsertPoint(loopBB);

  // loop:

  if (!body_->codegen(context)) return nullptr;

  // goto next:
  context.builder.CreateBr(nextBB);

  // next:
  context.builder.SetInsertPoint(nextBB);

  context.builder.CreateBr(testBB);

  // after:
  context.builder.SetInsertPoint(afterBB);

  // variable->addIncoming(next, loopEndBB);

  return llvm::Constant::getNullValue(llvm::Type::getInt64Ty(context.context));
}

llvm::Value *AST::CallExp::codegen(CodeGenContext &context) {
  auto callee = context.functionDecs[func_];
  llvm::Function *function;
  llvm::Function::LinkageTypes type;
  size_t level = 0u;
  if (!callee) {
    function = context.functions[func_];
    if (!function) return context.logErrorV("Function " + func_ + " not found");
    type = llvm::Function::ExternalLinkage;
  } else {
    function = callee->getProto().getFunction();
    type = function->getLinkage();
    level = callee->getLevel();
  }

  // If argument mismatch error.
  std::vector<llvm::Value *> args;
  if (type == llvm::Function::InternalLinkage) {
    size_t currentLevel = context.currentLevel;
    auto staticLink = context.staticLink.begin();
    llvm::Value *value = context.currentFrame;
    while (currentLevel-- >= level) {
      value =
          context.builder.CreateGEP(llvm::PointerType::getUnqual(*++staticLink),
                                    value, context.zero, "staticLink");
      value = context.builder.CreateLoad(value, "frame");
    }
    args.push_back(value);
  }
  for (size_t i = 0u; i != args_.size(); ++i) {
    args.push_back(args_[i]->codegen(context));
    if (!args.back()) return nullptr;
  }

  if (function->getFunctionType()->getReturnType()->isVoidTy()) {
    return context.builder.CreateCall(function, args);
  } else {
    return context.builder.CreateCall(function, args, "calltmp");
  }
}

llvm::Value *AST::ArrayExp::codegen(CodeGenContext &context) {
  auto function = context.builder.GetInsertBlock()->getParent();
  // if (!type_->isPointerTy()) return context.logErrorV("Array type required");
  auto eleType = context.getElementType(type_);
  auto size = size_->codegen(context);
  auto init = init_->codegen(context);
  auto eleSize = context.module->getDataLayout().getTypeAllocSize(eleType);
  llvm::Value *arrayPtr = context.builder.CreateCall(
      context.allocaArrayFunction,
      std::vector<llvm::Value *>{
          size, llvm::ConstantInt::get(llvm::Type::getInt64Ty(context.context),
                                       llvm::APInt(64, eleSize))},
      "alloca");
  arrayPtr = context.builder.CreateBitCast(arrayPtr, type_, "array");

  // auto arrayPtr = createEntryBlockAlloca(function, eleType, "arrayPtr",
  // size);
  auto zero = llvm::ConstantInt::get(context.context, llvm::APInt(64, 0, true));

  std::string indexName = "index";
  auto indexPtr = context.createEntryBlockAlloca(
      function, llvm::Type::getInt64Ty(context.context), indexName);
  // before loop:
  context.builder.CreateStore(zero, indexPtr);

  auto testBB = llvm::BasicBlock::Create(context.context, "test", function);
  auto loopBB = llvm::BasicBlock::Create(context.context, "loop", function);
  auto nextBB = llvm::BasicBlock::Create(context.context, "next", function);
  auto afterBB = llvm::BasicBlock::Create(context.context, "after", function);

  context.builder.CreateBr(testBB);

  context.builder.SetInsertPoint(testBB);

  auto index = context.builder.CreateLoad(indexPtr, indexName);
  auto EndCond = context.builder.CreateICmpSLT(index, size, "loopcond");
  // auto loopEndBB = context.builder.GetInsertBlock();

  // goto after or loop
  context.builder.CreateCondBr(EndCond, loopBB, afterBB);

  context.builder.SetInsertPoint(loopBB);

  // loop:
  // variable->addIncoming(low, preheadBB);

  // TODO: check its non-type value
  auto elePtr = context.builder.CreateGEP(eleType, arrayPtr, index, "elePtr");
  context.checkStore(init, elePtr);
  // context.builder.CreateStore(init, elePtr);
  // goto next:
  context.builder.CreateBr(nextBB);

  // next:
  context.builder.SetInsertPoint(nextBB);

  auto nextVar = context.builder.CreateAdd(
      index, llvm::ConstantInt::get(context.context, llvm::APInt(64, 1)),
      "nextvar");
  context.builder.CreateStore(nextVar, indexPtr);

  context.builder.CreateBr(testBB);

  // after:
  context.builder.SetInsertPoint(afterBB);

  // variable->addIncoming(next, loopEndBB);

  return arrayPtr;
}

llvm::Value *AST::SubscriptVar::codegen(CodeGenContext &context) {
  auto var = var_->codegen(context);
  auto exp = exp_->codegen(context);
  if (!var) return nullptr;
  var = context.builder.CreateLoad(var, "arrayPtr");
  return context.builder.CreateGEP(type_, var, exp, "ptr");
}
llvm::Value *AST::FieldVar::codegen(CodeGenContext &context) {
  auto var = var_->codegen(context);
  if (!var) return nullptr;
  var = context.builder.CreateLoad(var, "structPtr");
  auto idx = llvm::ConstantInt::get(llvm::Type::getInt64Ty(context.context),
                                    llvm::APInt(64, idx_));
  return context.builder.CreateGEP(type_, var, idx, "ptr");
}

llvm::Value *AST::FieldExp::codegen(CodeGenContext &context) {
  return exp_->codegen(context);
}

llvm::Value *AST::RecordExp::codegen(CodeGenContext &context) {
  context.builder.GetInsertBlock()->getParent();
  if (!type_) return nullptr;
  // auto var = createEntryBlockAlloca(function, type, "record");
  auto eleType = context.getElementType(type_);
  auto size = context.module->getDataLayout().getTypeAllocSize(eleType);
  llvm::Value *var = context.builder.CreateCall(
      context.allocaRecordFunction,
      llvm::ConstantInt::get(context.intType, llvm::APInt(64, size)), "alloca");
  var = context.builder.CreateBitCast(var, type_, "record");
  size_t idx = 0u;
  for (auto &field : fieldExps_) {
    auto exp = field->codegen(context);
    if (!exp) return nullptr;
    if (!field->type_) return nullptr;
    auto elementPtr = context.builder.CreateGEP(
        field->type_, var,
        llvm::ConstantInt::get(llvm::Type::getInt64Ty(context.context),
                               llvm::APInt(64, idx)),
        "elementPtr");
    context.checkStore(exp, elementPtr);
    // context.builder.CreateStore(exp, elementPtr);
    ++idx;
  }
  return var;
}

llvm::Value *AST::StringExp::codegen(CodeGenContext &context) {
  return context.builder.CreateGlobalStringPtr(val_, "str");
}

llvm::Function *AST::Prototype::codegen(CodeGenContext &) {
  auto *retType = resultType_;

  if (!retType) return nullptr;

  // auto oldFunc = functions[name_];
  // if (oldFunc) rename(oldFunc->getName().str() + "-");

  size_t idx = 0u;
  size_t offset = 0u;
  if (function_->getLinkage() == llvm::Function::ExternalLinkage)
    offset = 0u;
  else
    offset = 1u;

  for (auto &arg : function_->args())
    if (idx >= offset)
      arg.setName(params_[idx++ - offset]->getName());
    else {
      arg.setName("staticLink");
      ++idx;
    }
  return function_;
}

// TODO: Static link
llvm::Value *AST::FunctionDec::codegen(CodeGenContext &context) {
  auto function = proto_->codegen(context);
  if (context.functionDecs.lookupOne(name_))
    return context.logErrorV("Function " + name_ +
                             " is already defined in same scope.");
  // auto function = module->getFunction(proto.getName());
  if (!function) return nullptr;
  context.functionDecs.push(name_, this);

  auto oldBB = context.builder.GetInsertBlock();
  auto BB = llvm::BasicBlock::Create(context.context, "entry", function);
  context.builder.SetInsertPoint(BB);
  // llvm::StructType::create(context.context, );
  context.valueDecs.enter();
  ++context.currentLevel;
  context.staticLink.push_front(proto_->getFrame());
  std::vector<llvm::Type *> localVar;
  for (auto &var : variableTable_) {
    localVar.push_back(var->getType());
  }
  proto_->getFrame()->setBody(localVar);
  auto oldFrame = context.currentFrame;
  context.currentFrame = context.createEntryBlockAlloca(
      function, proto_->getFrame(), name_ + "frame");
  size_t idx = 0u;
  auto &params = proto_->getParams();
  for (auto &arg : function->args()) {
    if (idx == 0) {
      context.builder.CreateStore(&arg, proto_->getStaticLink()->read(context));
      idx++;
    } else {
      auto var = params[idx - 1]->getVar();
      context.builder.CreateStore(&arg, var->read(context));
      context.valueDecs.push(arg.getName(), var);
    }
  }
  if (auto retVal = body_->codegen(context)) {
    if (proto_->getResultType()->isVoidTy()) {
      context.builder.CreateRetVoid();
    } else {
      context.builder.CreateRet(retVal);
    }
    if (!llvm::verifyFunction(*function, &llvm::errs())) {
      context.valueDecs.exit();
      context.builder.SetInsertPoint(oldBB);
      context.currentFrame = oldFrame;
      context.staticLink.pop_front();
      --context.currentLevel;
      return function;
    }
  }
  context.valueDecs.exit();
  function->eraseFromParent();
  context.functionDecs.popOne(name_);
  context.builder.SetInsertPoint(oldBB);
  context.staticLink.pop_front();
  context.currentFrame = oldFrame;
  --context.currentLevel;
  return context.logErrorV("Function " + name_ + " genteration failed");
}

llvm::Value *AST::VarDec::codegen(CodeGenContext &context) {
  // llvm::Function *function = context.builder.GetInsertBlock()->getParent();
  auto init = init_->codegen(context);
  if (!init) return nullptr;
  // auto *variable = context.createEntryBlockAlloca(function, type_, name_);
  //  if (isNil(init)) {
  //    if (!type->isStructTy()) {
  //      return context.logErrorVV("Nil can only assign to struct type");
  //    } else {
  //      init =
  //      llvm::ConstantPointerNull::get(llvm::PointerType::getUnqual(type));
  //    }
  //  }
  auto var = context.checkStore(init, read(context));
  // context.builder.CreateStore(init, variable);
  context.valueDecs.push(name_, this);
  return var;
}

llvm::Value *AST::VarDec::read(CodeGenContext &context) const {
  size_t currentLevel = context.currentLevel;
  auto staticLink = context.staticLink.begin();
  llvm::Value *value = context.currentFrame;
  while (currentLevel-- > level_) {
    value =
        context.builder.CreateGEP(llvm::PointerType::getUnqual(*++staticLink),
                                  value, context.zero, "staticLink");
    value = context.builder.CreateLoad(value, "frame");
  }
  return context.builder.CreateGEP(
      type_, value,
      llvm::ConstantInt::get(llvm::Type::getInt64Ty(context.context),
                             llvm::APInt(64, offset_)),
      name_);
}

llvm::Value *AST::TypeDec::codegen(CodeGenContext &context) {
  return llvm::Constant::getNullValue(llvm::Type::getInt64Ty(context.context));
}

llvm::Value *AST::BinaryExp::codegen(CodeGenContext &context) {
  auto L = left_->codegen(context);
  auto R = right_->codegen(context);
  if (!L || !R) return nullptr;
  // TODO: check for nil
  switch (op_) {
    case ADD:
      return context.builder.CreateAdd(L, R, "addtmp");
    case SUB:
      return context.builder.CreateSub(L, R, "subtmp");
    case MUL:
      return context.builder.CreateMul(L, R, "multmp");
    case DIV:
      return context.builder.CreateFPToSI(
          context.builder.CreateFDiv(
              context.builder.CreateSIToFP(
                  L, llvm::Type::getDoubleTy(context.context)),
              context.builder.CreateSIToFP(
                  R, llvm::Type::getDoubleTy(context.context)),
              "divftmp"),
          llvm::Type::getInt64Ty(context.context), "divtmp");
    case LTH:
      return context.builder.CreateZExt(
          context.builder.CreateICmpSLT(L, R, "cmptmp"), context.intType,
          "cmptmp");
    case GTH:
      return context.builder.CreateZExt(
          context.builder.CreateICmpSGT(L, R, "cmptmp"), context.intType,
          "cmptmp");
    case EQU:
      return context.builder.CreateZExt(
          context.builder.CreateICmpEQ(L, R, "cmptmp"), context.intType,
          "cmptmp");
    case NEQU:
      return context.builder.CreateZExt(
          context.builder.CreateICmpNE(L, R, "cmptmp"), context.intType,
          "cmptmp");
    case LEQ:
      return context.builder.CreateZExt(
          context.builder.CreateICmpSLE(L, R, "cmptmp"), context.intType,
          "cmptmp");
    case GEQ:
      return context.builder.CreateZExt(
          context.builder.CreateICmpSGE(L, R, "cmptmp"), context.intType,
          "cmptmp");
    case AND_:
      return context.builder.CreateAnd(L, R, "andtmp");
    case OR_:
      return context.builder.CreateOr(L, R, "ortmp");
    case XOR:
      return context.builder.CreateXor(L, R, "xortmp");
  }
  return nullptr;
}
