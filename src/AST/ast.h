#ifndef AST_H
#define AST_H

#include <llvm/IR/Function.h>
#include <llvm/IR/Value.h>
#include <utils/codegencontext.h>
#include <algorithm>
#include <memory>
#include <set>
#include <string>
#include <vector>

class CodeGenContext;
namespace AST {
using llvm::Value;
using std::move;
using std::reverse;
using std::set;
using std::string;
using std::unique_ptr;
using std::vector;

class VarDec;

class Node {
  size_t pos_;

 public:
  virtual ~Node() = default;
  virtual Value *codegen(CodeGenContext &context) = 0;
  void setPos(const size_t &pos) { pos_ = pos; }

  virtual llvm::Type *traverse(vector<VarDec *> &, CodeGenContext &) = 0;
};

class Var : public Node {
};

class Exp : public Node {
};

class Root : public Node {
  unique_ptr<Exp> root_;
  vector<VarDec *> mainVariableTable_;


 public:
  Root(unique_ptr<Exp> root) : root_(move(root)) {}
  Value *codegen(CodeGenContext &context) override;
  llvm::Type *traverse(vector<VarDec *> &variableTable,
                       CodeGenContext &context) override;
};

class Dec : public Node {
 protected:
  string name_;

 public:
  Dec(string name) : name_(move(name)) {}
};

class Type {
 protected:
  string name_;

 public:
  Type() = default;
  void setName(string name) { name_ = move(name); }
  const string &getName() const { return name_; }
  virtual ~Type() = default;
  virtual llvm::Type *traverse(std::set<string> &parentName,
                               CodeGenContext &context) = 0;
};

class SimpleVar : public Var {
  string name_;

 public:
  SimpleVar(string name) : name_(move(name)) {}
  Value *codegen(CodeGenContext &context) override;
  llvm::Type *traverse(vector<VarDec *> &variableTable,
                       CodeGenContext &context) override;
};

class FieldVar : public Var {
  unique_ptr<Var> var_;
  string field_;
  llvm::Type *type_{nullptr};
  size_t idx_{0u};

 public:
  FieldVar(unique_ptr<Var> var, string field)
      : var_(move(var)), field_(move(field)) {}
  Value *codegen(CodeGenContext &context) override;

  llvm::Type *traverse(vector<VarDec *> &variableTable,
                       CodeGenContext &context) override;
};

class SubscriptVar : public Var {
  unique_ptr<Var> var_;
  unique_ptr<Exp> exp_;
  llvm::Type *type_{nullptr};

 public:
  SubscriptVar(unique_ptr<Var> var, unique_ptr<Exp> exp)
      : var_(move(var)), exp_(move(exp)) {}
  Value *codegen(CodeGenContext &context) override;

  llvm::Type *traverse(vector<VarDec *> &variableTable,
                       CodeGenContext &context) override;
};

class VarExp : public Exp {
  unique_ptr<Var> var_;

 public:
  VarExp(unique_ptr<Var> var) : var_(move(var)) {}
  Value *codegen(CodeGenContext &context) override;

  llvm::Type *traverse(vector<VarDec *> &variableTable,
                       CodeGenContext &context) override;
};

class NilExp : public Exp {
  // dummy body
  llvm::Type *type_{nullptr};
 public:
  NilExp() = default;
  Value *codegen(CodeGenContext &context) override;

  llvm::Type *traverse(vector<VarDec *> &variableTable,
                       CodeGenContext &context) override;

  void setType(llvm::Type* type){type_ = type;}
};

class IntExp : public Exp {
  int val_;

 public:
  IntExp(int const &val) : val_(val) {}
  Value *codegen(CodeGenContext &context) override;

  llvm::Type *traverse(vector<VarDec *> &variableTable,
                       CodeGenContext &context) override;
};

class StringExp : public Exp {
  string val_;

 public:
  StringExp(string val) : val_(move(val)) {}
  Value *codegen(CodeGenContext &context) override;

  llvm::Type *traverse(vector<VarDec *> &variableTable,
                       CodeGenContext &context) override;
};

class CallExp : public Exp {
  string func_;
  vector<unique_ptr<Exp>> args_;

 public:
  CallExp(string func, vector<unique_ptr<Exp>> args)
      : func_(move(func)), args_(move(args)) {
    reverse(args_.begin(), args_.end());
  }
  Value *codegen(CodeGenContext &context) override;

  llvm::Type *traverse(vector<VarDec *> &variableTable,
                       CodeGenContext &context) override;
};

// TODO: UnaryExp

class BinaryExp : public Exp {
 public:
  enum Operator : char {
    ADD = '+',
    SUB = '-',
    MUL = '*',
    DIV = '/',
    LTH = '<',
    GTH = '>',
    EQU = '=',
    NEQU = '!',
    LEQ = '[',
    GEQ = ']',

    AND_ = '&',
    OR_ = '|',
    //    AND = '&',
    //    OR = '|',
    XOR = '^',
  };

 private:
  Operator op_;  // TODO: use enum
  unique_ptr<Exp> left_;
  unique_ptr<Exp> right_;

 public:
  BinaryExp(Operator const &op, unique_ptr<Exp> left, unique_ptr<Exp> right)
      : op_(op), left_(move(left)), right_(move(right)) {}
  Value *codegen(CodeGenContext &context) override;

  llvm::Type *traverse(vector<VarDec *> &variableTable,
                       CodeGenContext &context) override;
};

class Field {
  friend class RecordType;

  string name_;
  string typeName_;
  llvm::Type *type_{nullptr};
  VarDec *varDec_{nullptr};

 public:
  Field(string name, string type) : name_(move(name)), typeName_(move(type)) {}

  llvm::Type *traverse(vector<VarDec *> &variableTable,
                       CodeGenContext &context);
  llvm::Type *getType() const { return type_; }
  const string &getName() const { return name_; }
  VarDec *getVar() const { return varDec_; }
};

class FieldExp : public Exp {
  friend class RecordExp;
  string name_;
  unique_ptr<Exp> exp_;
  llvm::Type *type_;

 public:
  FieldExp(string name, unique_ptr<Exp> exp)
      : name_(move(name)), exp_(move(exp)) {}

  const string &getName() const { return name_; }

  Value *codegen(CodeGenContext &context) override;

  llvm::Type *traverse(vector<VarDec *> &variableTable,
                       CodeGenContext &context) override;
};

class RecordExp : public Exp {
  friend class RecordType;
  string typeName_;
  vector<unique_ptr<FieldExp>> fieldExps_;
  llvm::Type *type_{nullptr};

 public:
  RecordExp(string type, vector<unique_ptr<FieldExp>> fieldExps)
      : typeName_(move(type)), fieldExps_(move(fieldExps)) {
    reverse(fieldExps_.begin(), fieldExps_.end());
  }
  Value *codegen(CodeGenContext &context) override;

  llvm::Type *traverse(vector<VarDec *> &variableTable,
                       CodeGenContext &context) override;
};

class SequenceExp : public Exp {
  vector<unique_ptr<Exp>> exps_;

 public:
  SequenceExp(vector<unique_ptr<Exp>> exps) : exps_(move(exps)) {
    reverse(exps_.begin(), exps_.end());
  }
  Value *codegen(CodeGenContext &context) override;

  llvm::Type *traverse(vector<VarDec *> &variableTable,
                       CodeGenContext &context) override;
};

class AssignExp : public Exp {
  unique_ptr<Var> var_;
  unique_ptr<Exp> exp_;

 public:
  AssignExp(unique_ptr<Var> var, unique_ptr<Exp> exp)
      : var_(move(var)), exp_(move(exp)) {}
  Value *codegen(CodeGenContext &context) override;

  llvm::Type *traverse(vector<VarDec *> &variableTable,
                       CodeGenContext &context) override;
};

class IfExp : public Exp {
  unique_ptr<Exp> test_;
  unique_ptr<Exp> then_;
  unique_ptr<Exp> else_;

 public:
  IfExp(unique_ptr<Exp> test, unique_ptr<Exp> then, unique_ptr<Exp> elsee)
      : test_(move(test)), then_(move(then)), else_(move(elsee)) {}
  Value *codegen(CodeGenContext &context) override;

  llvm::Type *traverse(vector<VarDec *> &variableTable,
                       CodeGenContext &context) override;
};

class WhileExp : public Exp {
  unique_ptr<Exp> test_;
  unique_ptr<Exp> body_;

 public:
  WhileExp(unique_ptr<Exp> test, unique_ptr<Exp> body)
      : test_(move(test)), body_(move(body)) {}
  Value *codegen(CodeGenContext &context) override;

  llvm::Type *traverse(vector<VarDec *> &variableTable,
                       CodeGenContext &context) override;
};

class ForExp : public Exp {
  string var_;
  unique_ptr<Exp> low_;
  unique_ptr<Exp> high_;
  unique_ptr<Exp> body_;
  // bool escape;
  VarDec *varDec_{nullptr};

 public:
  ForExp(string var, unique_ptr<Exp> low, unique_ptr<Exp> high,
         unique_ptr<Exp> body)
      : var_(move(var)),
        low_(move(low)),
        high_(move(high)),
        body_(move(body)) {}
  Value *codegen(CodeGenContext &context) override;

  llvm::Type *traverse(vector<VarDec *> &variableTable,
                       CodeGenContext &context) override;
};

class BreakExp : public Exp {
  // dummpy body
 public:
  BreakExp() = default;
  Value *codegen(CodeGenContext &context) override;
  llvm::Type *traverse(vector<VarDec *> &variableTable,
                       CodeGenContext &context) override;

};

class LetExp : public Exp {
  vector<unique_ptr<Dec>> decs_;
  unique_ptr<Exp> body_;

 public:
  LetExp(vector<unique_ptr<Dec>> decs, unique_ptr<Exp> body)
      : decs_(move(decs)), body_(move(body)) {
    reverse(decs_.begin(), decs_.end());
  }
  Value *codegen(CodeGenContext &context) override;

  llvm::Type *traverse(vector<VarDec *> &variableTable,
                       CodeGenContext &context) override;
};

class ArrayExp : public Exp {
  string typeName_;
  unique_ptr<Exp> size_;
  unique_ptr<Exp> init_;
  llvm::Type *type_{nullptr};

 public:
  ArrayExp(string type, unique_ptr<Exp> size, unique_ptr<Exp> init)
      : typeName_(move(type)), size_(move(size)), init_(move(init)) {}
  Value *codegen(CodeGenContext &context) override;

  llvm::Type *traverse(vector<VarDec *> &variableTable,
                       CodeGenContext &context) override;
};

class Prototype {
  string name_;
  vector<unique_ptr<Field>> params_;
  string result_;
  llvm::Type *resultType_{nullptr};
  llvm::Function *function_{nullptr};
  VarDec *staticLink_{nullptr};
  llvm::StructType *frame{nullptr};

 public:
  Prototype(string name, vector<unique_ptr<Field>> params, string result)
      : name_(move(name)), params_(move(params)), result_(move(result)) {
    reverse(params_.begin(), params_.end());
  }
  llvm::Function *codegen(CodeGenContext &context);

  const string &getName() const { return name_; }

  void rename(string name) { name_ = move(name); }

  const vector<unique_ptr<Field>> &getParams() const { return params_; }

  llvm::Type *getResultType() const { return resultType_; }

  llvm::FunctionType *traverse(vector<VarDec *> &variableTable,
                               CodeGenContext &context);
  llvm::StructType *getFrame() const { return frame; }

  llvm::Function *getFunction() const { return function_; }
  VarDec *getStaticLink() const { return staticLink_; }
};

class FunctionDec : public Dec {
  unique_ptr<Prototype> proto_;
  unique_ptr<Exp> body_;
  vector<VarDec *> variableTable_;
  size_t level_{0u};

 public:
  FunctionDec(string name, unique_ptr<Prototype> proto, unique_ptr<Exp> body)
      : Dec(move(name)), proto_(move(proto)), body_(move(body)) {}
  Value *codegen(CodeGenContext &context) override;

  llvm::Type *traverse(vector<VarDec *> &variableTable,
                       CodeGenContext &context) override;
  Prototype &getProto() const { return *proto_; }
  size_t getLevel() const { return level_; }
};

class VarDec : public Dec {
  string typeName_;
  unique_ptr<Exp> init_{nullptr};
  // bool escape;
  size_t offset_;
  size_t level_;
  llvm::Type *type_{nullptr};

 public:
  VarDec(string name, string type, unique_ptr<Exp> init)
      : Dec(move(name)), typeName_(move(type)), init_(move(init)) {}
  VarDec(string name, llvm::Type *type, size_t const &offset,
         size_t const &level)
      : Dec(move(name)), offset_(offset), level_(level), type_(type) {}
  Value *codegen(CodeGenContext &context) override;

  llvm::Type *getType() const { return type_; }
  const string &getName() const { return name_; }

  llvm::Value *read(CodeGenContext &context) const;
  llvm::Type *traverse(vector<VarDec *> &variableTable,
                       CodeGenContext &context) override;
};

class TypeDec : public Dec {
  unique_ptr<Type> type_;

 public:
  TypeDec(string name, unique_ptr<Type> type)
      : Dec(move(name)), type_(move(type)) {}
  Value *codegen(CodeGenContext &context) override;

  llvm::Type *traverse(vector<VarDec *> &variableTable,
                       CodeGenContext &context) override;
};

class NameType : public Type {
  string type_;

 public:
  llvm::Type *traverse(std::set<string> &parentName,
                       CodeGenContext &context) override;
  NameType(string type) : type_(move(type)) {}
};

class RecordType : public Type {
  friend class RecordExp;
  friend class FieldVar;
  vector<unique_ptr<Field>> fields_;

 public:
  RecordType(vector<unique_ptr<Field>> fields) : fields_(move(fields)) {
    reverse(fields_.begin(), fields_.end());
  }
  llvm::Type *traverse(std::set<string> &parentName,
                       CodeGenContext &context) override;
};

class ArrayType : public Type {
  string type_;

 protected:
 public:
  ArrayType(string type) : type_(move(type)) {}
  llvm::Type *traverse(std::set<string> &parentName,
                       CodeGenContext &context) override;
};

}  // namespace AST

#endif  // AST_H
