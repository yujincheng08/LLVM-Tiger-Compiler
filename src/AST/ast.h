#ifndef AST_H
#define AST_H

#include <memory>
#include <string>
#include <vector>

#include <llvm/IR/Value.h>

namespace AST {
using llvm::Value;
using std::move;
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

class Var : public Node {
 public:
  void print(int n) override;
};

class Exp : public Node {
 public:
  void print(int n) override;
};

class Dec : public Node {
  string name_;

 public:
  Dec(string name) : name_(move(name)) {}
  void print(int n) override;
};

class Type {
 public:
  virtual ~Type() = default;
  virtual llvm::Type *codegen() = 0;
  virtual void print(int n);
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
      : func_(move(func)), args_(move(args)) {}
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
  string name_;
  unique_ptr<Type> type_;

 public:
  Field(string name, unique_ptr<Type> type)
      : name_(move(name)), type_(move(type)) {}

  virtual void print(int n);
};

class FieldExp {
  string name_;
  unique_ptr<Exp> exp_;

 public:
  FieldExp(string name, unique_ptr<Exp> exp)
      : name_(move(name)), exp_(move(exp)) {}

  virtual void print(int n);
};

class RecordExp : public Exp {
  string name_;
  vector<unique_ptr<FieldExp>> fieldExps_;

 public:
  RecordExp(string name, vector<unique_ptr<FieldExp>> fieldExps)
      : name_(move(name)), fieldExps_(move(fieldExps)) {}
  Value *codegen() override;

  void print(int n) override;
};

class SequenceExp : public Exp {
  vector<unique_ptr<Exp>> exps_;

 public:
  SequenceExp(vector<unique_ptr<Exp>> exps) : exps_(move(exps)) {}
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
      : decs_(move(decs)), body_(move(body)) {}
  Value *codegen() override;

  void print(int n) override;
};

class ArrayExp : public Exp {
  string type_;
  unique_ptr<Exp> size_;
  unique_ptr<Exp> init_;

 public:
  ArrayExp(string type, unique_ptr<Exp> size, unique_ptr<Exp> init)
      : type_(move(type)), size_(move(size)), init_(move(init)) {}
  Value *codegen() override;

  void print(int n) override;
};

class Prototype {
  string name_;
  vector<unique_ptr<Field>> params_;
  string result_;

 public:
  Prototype(string name, vector<unique_ptr<Field>> params, string result)
      : name_(move(name)), params_(move(params)), result_(move(result)) {}
  llvm::Function *codegen();

  void print(int n);
};

class FunctionDec : public Dec {
  string name_;
  unique_ptr<Prototype> proto_;
  unique_ptr<Exp> body_;

 public:
  FunctionDec(string name, unique_ptr<Prototype> proto, unique_ptr<Exp> body)
      : Dec(move(name)), proto_(move(proto)), body_(move(body)) {}
  Value *codegen() override;

  void print(int n) override;
};

class VarDec : public Dec {
  string type_;
  unique_ptr<Exp> init_;
  // bool escape;
 public:
  VarDec(string name, string type, unique_ptr<Exp> init)
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
  NameType(string name) : name_(move(name)) {}
  llvm::Type *codegen() override;
  void print(int n);
};

class RecordType : public Type {
  vector<unique_ptr<Field>> fields_;

 public:
  RecordType(vector<unique_ptr<Field>> fields) : fields_(move(fields)) {}
  llvm::Type *codegen() override;

  void print(int n);
};

class ArrayType : public Type {
  string name_;

 public:
  ArrayType(string name) : name_(move(name)) {}
  llvm::Type *codegen() override;

  void print(int n);
};

}  // namespace AST

#endif  // AST_H