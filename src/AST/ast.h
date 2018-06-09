#ifndef AST_H
#define AST_H

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

class Node {
  size_t pos_;

 public:
  virtual ~Node() = default;
  virtual Value *codegen(CodeGenContext &context) = 0;
  void setPos(const size_t &pos) { pos_ = pos; }

  virtual void print(int n) = 0;

  virtual llvm::Type *traverse(vector<string> &, CodeGenContext &) = 0;
};

class Var : public Node {};

class Exp : public Node {};

class Root : public Node {
  unique_ptr<Exp> root_;
  vector<string> mainVariableTable_;

 public:
  Root(unique_ptr<Exp> root) : root_(move(root)) {}
  Value *codegen(CodeGenContext &context) override;
  void print(int n) override;
  llvm::Type *traverse(vector<string> &variableTable,
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
  virtual llvm::Type *codegen(std::set<string> &parentName,
                              CodeGenContext &context) = 0;
  virtual void print(int n) = 0;
};

class SimpleVar : public Var {
  string name_;

 public:
  SimpleVar(string name) : name_(move(name)) {}
  Value *codegen(CodeGenContext &context) override;
  llvm::Type *traverse(vector<string> &variableTable,
                       CodeGenContext &context) override;
  void print(int n) override;
};

class FieldVar : public Var {
  unique_ptr<Var> var_;
  string field_;
  llvm::Type *type_{nullptr};

 public:
  FieldVar(unique_ptr<Var> var, string field)
      : var_(move(var)), field_(move(field)) {}
  Value *codegen(CodeGenContext &context) override;
  void print(int n) override;

  llvm::Type *traverse(vector<string> &variableTable,
                       CodeGenContext &context) override;
};

class SubscriptVar : public Var {
  unique_ptr<Var> var_;
  unique_ptr<Exp> exp_;

 public:
  SubscriptVar(unique_ptr<Var> var, unique_ptr<Exp> exp)
      : var_(move(var)), exp_(move(exp)) {}
  Value *codegen(CodeGenContext &context) override;

  llvm::Type *traverse(vector<string> &variableTable,
                       CodeGenContext &context) override;
  void print(int n) override;
};

class VarExp : public Exp {
  unique_ptr<Var> var_;

 public:
  VarExp(unique_ptr<Var> var) : var_(move(var)) {}
  Value *codegen(CodeGenContext &context) override;

  llvm::Type *traverse(vector<string> &variableTable,
                       CodeGenContext &context) override;
  void print(int n) override;
};

class NilExp : public Exp {
  // dummy body
 public:
  NilExp() = default;
  Value *codegen(CodeGenContext &context) override;

  llvm::Type *traverse(vector<string> &variableTable,
                       CodeGenContext &context) override;
  void print(int n) override;
};

class IntExp : public Exp {
  int val_;

 public:
  IntExp(int const &val) : val_(val) {}
  Value *codegen(CodeGenContext &context) override;

  llvm::Type *traverse(vector<string> &variableTable,
                       CodeGenContext &context) override;
  void print(int n) override;
};

class StringExp : public Exp {
  string val_;

 public:
  StringExp(string val) : val_(move(val)) {}
  Value *codegen(CodeGenContext &context) override;

  llvm::Type *traverse(vector<string> &variableTable,
                       CodeGenContext &context) override;
  void print(int n) override;
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

  void print(int n) override;

  llvm::Type *traverse(vector<string> &variableTable,
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
  void print(int n) override;

  llvm::Type *traverse(vector<string> &variableTable,
                       CodeGenContext &context) override;
};

class Field {
  friend class Prototype;
  friend class RecordType;
  friend class RecordExp;
  friend class FieldVar;

  string name_;
  string typeName_;
  llvm::Type *type_{nullptr};

 public:
  Field(string name, string type) : name_(move(name)), typeName_(move(type)) {}

  void print(int n);

  llvm::Type *traverse(vector<string> &variableTable, CodeGenContext &context);
};

class FieldExp : Exp {
  string name_;
  unique_ptr<Exp> exp_;

 public:
  FieldExp(string name, unique_ptr<Exp> exp)
      : name_(move(name)), exp_(move(exp)) {}

  const string &getName() const { return name_; }

  void print(int n) override;

  Value *codegen(CodeGenContext &context) override;

  llvm::Type *traverse(vector<std::string> &variableTable,
                       CodeGenContext &context) override;
};

class RecordExp : public Exp {
  string typeName_;
  vector<unique_ptr<FieldExp>> fieldExps_;
  llvm::Type *type_{nullptr};

 public:
  RecordExp(string type, vector<unique_ptr<FieldExp>> fieldExps)
      : typeName_(move(type)), fieldExps_(move(fieldExps)) {
    reverse(fieldExps_.begin(), fieldExps_.end());
  }
  Value *codegen(CodeGenContext &context) override;

  void print(int n) override;
  llvm::Type *traverse(vector<std::string> &variableTable,
                       CodeGenContext &context) override;
};

class SequenceExp : public Exp {
  vector<unique_ptr<Exp>> exps_;

 public:
  SequenceExp(vector<unique_ptr<Exp>> exps) : exps_(move(exps)) {
    reverse(exps_.begin(), exps_.end());
  }
  Value *codegen(CodeGenContext &context) override;

  void print(int n) override;

  llvm::Type *traverse(vector<string> &variableTable,
                       CodeGenContext &context) override;
};

class AssignExp : public Exp {
  unique_ptr<Var> var_;
  unique_ptr<Exp> exp_;

 public:
  AssignExp(unique_ptr<Var> var, unique_ptr<Exp> exp)
      : var_(move(var)), exp_(move(exp)) {}
  Value *codegen(CodeGenContext &context) override;

  void print(int n) override;
  llvm::Type *traverse(vector<std::string> &variableTable,
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

  void print(int n) override;
  llvm::Type *traverse(vector<std::string> &variableTable,
                       CodeGenContext &context) override;
};

class WhileExp : public Exp {
  unique_ptr<Exp> test_;
  unique_ptr<Exp> body_;

 public:
  WhileExp(unique_ptr<Exp> test, unique_ptr<Exp> body)
      : test_(move(test)), body_(move(body)) {}
  Value *codegen(CodeGenContext &context) override;

  void print(int n) override;
  llvm::Type *traverse(vector<std::string> &variableTable,
                       CodeGenContext &context) override;
};

class ForExp : public Exp {
  string var_;
  unique_ptr<Exp> low_;
  unique_ptr<Exp> high_;
  unique_ptr<Exp> body_;
  // bool escape;

 public:
  ForExp(string var, unique_ptr<Exp> low, unique_ptr<Exp> high,
         unique_ptr<Exp> body)
      : var_(move(var)),
        low_(move(low)),
        high_(move(high)),
        body_(move(body)) {}
  Value *codegen(CodeGenContext &context) override;

  void print(int n) override;
  llvm::Type *traverse(vector<string> &variableTable,
                       CodeGenContext &context) override;
};

class BreakExp : public Exp {
  // dummpy body
 public:
  BreakExp() = default;
  Value *codegen(CodeGenContext &context) override;
  llvm::Type *traverse(vector<string> &variableTable,
                       CodeGenContext &context) override;

  void print(int n) override;
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

  void print(int n) override;
  llvm::Type *traverse(vector<string> &variableTable,
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

  void print(int n) override;
  llvm::Type *traverse(vector<std::string> &variableTable,
                       CodeGenContext &context) override;
};

class Prototype {
  string name_;
  vector<unique_ptr<Field>> params_;
  string result_;
  llvm::Type *resultType_{nullptr};

 public:
  Prototype(string name, vector<unique_ptr<Field>> params, string result)
      : name_(move(name)), params_(move(params)), result_(move(result)) {
    reverse(params_.begin(), params_.end());
  }
  llvm::Function *codegen(CodeGenContext &context);

  const string &getName() const { return name_; }

  void rename(string name) { name_ = move(name); }

  void print(int n);

  llvm::Type *traverse(vector<string> &variableTable, CodeGenContext &context);
};

class FunctionDec : public Dec {
  unique_ptr<Prototype> proto_;
  unique_ptr<Exp> body_;
  vector<string> variableTable_;

 public:
  FunctionDec(string name, unique_ptr<Prototype> proto, unique_ptr<Exp> body)
      : Dec(move(name)), proto_(move(proto)), body_(move(body)) {}
  Value *codegen(CodeGenContext &context) override;

  void print(int n) override;
  llvm::Type *traverse(vector<string> &variableTable,
                       CodeGenContext &context) override;
};

class VarDec : public Dec {
  string typeName_;
  unique_ptr<Exp> init_;
  // bool escape;
  size_t offset;
  llvm::Type *type_{nullptr};

 public:
  VarDec(string name, string type, unique_ptr<Exp> init)
      : Dec(move(name)), typeName_(move(type)), init_(move(init)) {}
  Value *codegen(CodeGenContext &context) override;

  llvm::Type *getType() const { return type_; }

  void print(int n) override;
  llvm::Type *traverse(vector<string> &variableTable,
                       CodeGenContext &context) override;
};

class TypeDec : public Dec {
  unique_ptr<Type> type_;

 public:
  TypeDec(string name, unique_ptr<Type> type)
      : Dec(move(name)), type_(move(type)) {}
  Value *codegen(CodeGenContext &context) override;

  void print(int n) override;
  llvm::Type *traverse(vector<string> &variableTable,
                       CodeGenContext &context) override;
};

class NameType : public Type {
  string type_;

 public:
  llvm::Type *codegen(std::set<string> &parentName,
                      CodeGenContext &context) override;
  NameType(string type) : type_(move(type)) {}
  void print(int n) override;
};

class RecordType : public Type {
  friend class RecordExp;
  friend class FieldVar;
  vector<unique_ptr<Field>> fields_;

 public:
  RecordType(vector<unique_ptr<Field>> fields) : fields_(move(fields)) {
    reverse(fields_.begin(), fields_.end());
  }
  llvm::Type *codegen(std::set<string> &parentName,
                      CodeGenContext &context) override;
  void print(int n) override;
};

class ArrayType : public Type {
  string type_;

 protected:
 public:
  ArrayType(string type) : type_(move(type)) {}
  llvm::Type *codegen(std::set<string> &parentName,
                      CodeGenContext &context) override;
  void print(int n) override;
};

}  // namespace AST

#endif  // AST_H
