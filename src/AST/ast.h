#ifndef AST_H
#define AST_H

#include <llvm/IR/Value.h>
#include <algorithm>
#include <memory>
#include <string>
#include <vector>

namespace AST {
using llvm::Value;
using std::move;
using std::reverse;
using std::string;
using std::unique_ptr;
using std::vector;

class Node {
  size_t pos_;

 public:
  virtual ~Node() = default;
  virtual Value *codegen() = 0;
  void setPos(const size_t &pos) { pos_ = pos; }

  virtual void print(int n) = 0;
};

class Var : public Node {};

class Exp : public Node {};

class Root : public Node {
  unique_ptr<Exp> root_;

 public:
  Root(unique_ptr<Exp> root) : root_(move(root)) {}
  Value *codegen() override;
  void print(int n) override;
};

class Dec : public Node {
 protected:
  string name_;

 public:
  Dec(string name) : name_(move(name)) {}
};

class Type {
 public:
  virtual ~Type() = default;
  virtual llvm::Type *codegen(string const &parentName = "") = 0;
  virtual void print(int n) = 0;
};

class SimpleVar : public Var {
  string name_;

 public:
  SimpleVar(string name) : name_(move(name)) {}
  Value *codegen() override;
  void print(int n) override;
};

class FieldVar : public Var {
  unique_ptr<Var> var_;
  string field_;

 public:
  FieldVar(unique_ptr<Var> var, string field)
      : var_(move(var)), field_(move(field)) {}
  Value *codegen() override;
  void print(int n) override;
};

class SubscriptVar : public Var {
  unique_ptr<Var> var_;
  unique_ptr<Exp> exp_;

 public:
  SubscriptVar(unique_ptr<Var> var, unique_ptr<Exp> exp)
      : var_(move(var)), exp_(move(exp)) {}
  Value *codegen() override;

  void print(int n) override;
};

class VarExp : public Exp {
  unique_ptr<Var> var_;

 public:
  VarExp(unique_ptr<Var> var) : var_(move(var)) {}
  Value *codegen() override;

  void print(int n) override;
};

class NilExp : public Exp {
  // dummy body
 public:
  NilExp() = default;
  Value *codegen() override;

  void print(int n) override;
};

class IntExp : public Exp {
  int val_;

 public:
  IntExp(int const &val) : val_(val) {}
  Value *codegen() override;

  void print(int n) override;
};

class StringExp : public Exp {
  string val_;

 public:
  StringExp(string val) : val_(move(val)) {}
  Value *codegen() override;

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
  Value *codegen() override;

  void print(int n) override;
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
  Value *codegen() override;
  void print(int n) override;
};

class Field {
  friend class Prototype;
  friend class RecordType;

  string name_;
  unique_ptr<Type> type_;

 public:
  Field(string name, unique_ptr<Type> type)
      : name_(move(name)), type_(move(type)) {}

  void print(int n);
};

class FieldExp {
  friend class RecordExp;
  string name_;
  unique_ptr<Exp> exp_;

 public:
  FieldExp(string name, unique_ptr<Exp> exp)
      : name_(move(name)), exp_(move(exp)) {}

  void print(int n);
};

class RecordExp : public Exp {
  unique_ptr<Type> type_;
  vector<unique_ptr<FieldExp>> fieldExps_;

 public:
  RecordExp(unique_ptr<Type> type, vector<unique_ptr<FieldExp>> fieldExps)
      : type_(move(type)), fieldExps_(move(fieldExps)) {
    reverse(fieldExps_.begin(), fieldExps_.end());
  }
  Value *codegen() override;

  void print(int n) override;
};

class SequenceExp : public Exp {
  vector<unique_ptr<Exp>> exps_;

 public:
  SequenceExp(vector<unique_ptr<Exp>> exps) : exps_(move(exps)) {
    reverse(exps_.begin(), exps_.end());
  }
  Value *codegen() override;

  void print(int n) override;
};

class AssignExp : public Exp {
  unique_ptr<Var> var_;
  unique_ptr<Exp> exp_;

 public:
  AssignExp(unique_ptr<Var> var, unique_ptr<Exp> exp)
      : var_(move(var)), exp_(move(exp)) {}
  Value *codegen() override;

  void print(int n) override;
};

class IfExp : public Exp {
  unique_ptr<Exp> test_;
  unique_ptr<Exp> then_;
  unique_ptr<Exp> else_;

 public:
  IfExp(unique_ptr<Exp> test, unique_ptr<Exp> then, unique_ptr<Exp> elsee)
      : test_(move(test)), then_(move(then)), else_(move(elsee)) {}
  Value *codegen() override;

  void print(int n) override;
};

class WhileExp : public Exp {
  unique_ptr<Exp> test_;
  unique_ptr<Exp> body_;

 public:
  WhileExp(unique_ptr<Exp> test, unique_ptr<Exp> body)
      : test_(move(test)), body_(move(body)) {}
  Value *codegen() override;

  void print(int n) override;
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
  Value *codegen() override;

  void print(int n) override;
};

class BreakExp : public Exp {
  // dummpy body
 public:
  BreakExp() = default;
  Value *codegen() override;

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
  Value *codegen() override;

  void print(int n) override;
};

class ArrayExp : public Exp {
  unique_ptr<Type> type_;
  unique_ptr<Exp> size_;
  unique_ptr<Exp> init_;

 public:
  ArrayExp(unique_ptr<Type> type, unique_ptr<Exp> size, unique_ptr<Exp> init)
      : type_(move(type)), size_(move(size)), init_(move(init)) {}
  Value *codegen() override;

  void print(int n) override;
};

class Prototype {
  string name_;
  vector<unique_ptr<Field>> params_;
  unique_ptr<Type> result_;

 public:
  Prototype(string name, vector<unique_ptr<Field>> params,
            unique_ptr<Type> result)
      : name_(move(name)), params_(move(params)), result_(move(result)) {
    reverse(params_.begin(), params_.end());
  }
  llvm::Function *codegen();

  const std::string &getName() const { return name_; }

  void rename(string name) { name_ = move(name); }

  void print(int n);
};

class FunctionDec : public Dec {
  unique_ptr<Prototype> proto_;
  unique_ptr<Exp> body_;

 public:
  FunctionDec(string name, unique_ptr<Prototype> proto, unique_ptr<Exp> body)
      : Dec(move(name)), proto_(move(proto)), body_(move(body)) {}
  Value *codegen() override;

  void print(int n) override;
};

class VarDec : public Dec {
  unique_ptr<Type> type_;
  unique_ptr<Exp> init_;
  // bool escape;
 public:
  VarDec(string name, unique_ptr<Type> type, unique_ptr<Exp> init)
      : Dec(move(name)), type_(move(type)), init_(move(init)) {}
  Value *codegen() override;

  void print(int n) override;
};

class TypeDec : public Dec {
  unique_ptr<Type> type_;

 public:
  TypeDec(string name, unique_ptr<Type> type)
      : Dec(move(name)), type_(move(type)) {}
  Value *codegen() override;

  void print(int n) override;
};

class NameType : public Type {
  string name_;

 public:
  llvm::Type *codegen(string const &parentName) override;
  NameType(string name) : name_(move(name)) {}
  void print(int n) override;
};

class RecordType : public Type {
  vector<unique_ptr<Field>> fields_;

 public:
  RecordType(vector<unique_ptr<Field>> fields) : fields_(move(fields)) {
    reverse(fields_.begin(), fields_.end());
  }
  llvm::Type *codegen(string const &parentName) override;
  void print(int n) override;
};

class ArrayType : public Type {
  unique_ptr<Type> type_;

 protected:
 public:
  ArrayType(unique_ptr<Type> name) : type_(move(name)) {}
  llvm::Type *codegen(string const &parentName) override;
  void print(int n) override;
};

}  // namespace AST

#endif  // AST_H
