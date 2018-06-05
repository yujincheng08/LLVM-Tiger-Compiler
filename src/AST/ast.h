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
  setPos(const size_t &pos) { pos_ = pos; }
};

class Var : public Node {};

class Exp : public Node {};

class Dec : public Node {
  string name_;

 public:
  Dec(string name) : name_(move(name)) {}
};

class Type : public Node {
  string name_;

 public:
  Type(string name) : name_(move(name)) {}
};

class SimpleVar : public Var {
  string name_;

 public:
  SimpleVar(string name) : name_(move(name)) {}
  Value *codegen() override;
};

class FieldVar : public Var {
  unique_ptr<Var> var_;
  string field_;

 public:
  FieldVar(unique_ptr<Var> var, string field)
      : var_(move(var)), field_(move(field)) {}
  Value *codegen() override;
};

class SubscriptVar : public Var {
  unique_ptr<Var> var_;
  unique_ptr<Exp> exp_;

 public:
  SubscriptVar(unique_ptr<Var> var, unique_ptr<Exp> exp)
      : var_(move(var)), exp_(move(exp)) {}
  Value *codegen() override;
};

class VarExp : public Exp {
  unique_ptr<Var> var_;

 public:
  VarExp(unique_ptr<Var> var) : var_(move(var)) {}
  Value *codegen() override;
};

class NilExp : public Exp {
  // dummy body
 public:
  NilExp() = default;
  Value *codegen() override;
};

class IntExp : public Exp {
  int val_;

 public:
  IntExp(int const &val) : val_(val) {}
  Value *codegen() override;
};

class StringExp : public Exp {
  string val_;

 public:
  StringExp(string val) : val_(move(val)) {}
  Value *codegen() override;
};

class CallExp : public Exp {
  string func_;
  vector<unique_ptr<Exp>> args_;

 public:
  CallExp(string func, vector<unique_ptr<Exp>> args)
      : func_(move(func)), args_(move(args)) {}
  Value *codegen() override;
};

// TODO: UnaryExp

class BinaryExp : public Exp {
  char op_;  // TODO: use enum
  unique_ptr<Exp> left_;
  unique_ptr<Exp> right_;

 public:
  BinaryExp(char const &op, unique_ptr<Exp> left, unique_ptr<Exp> right)
      : op_(op), left_(move(left)), right_(move(right)) {}
  Value *codegen() override;
};

class Field {
  string name_;
  unique_ptr<Exp> exp_;

 public:
  Field(string name, unique_ptr<Exp> exp)
      : name_(move(name)), exp_(move(exp)) {}
};

class RecordExp : Exp {
  string name_;
  vector<unique_ptr<Field>> fields_;

 public:
  RecordExp(string name, vector<unique_ptr<Field>> fields)
      : name_(move(name)), fields_(move(fields)) {}
  Value *codegen() override;
};

class SequenceExp : Exp {
  vector<unique_ptr<Exp>> exps_;

 public:
  SequenceExp(vector<unique_ptr<Exp>> exps) : exps_(move(exps)) {}
  Value *codegen() override;
};

class AssignExp : Exp {
  unique_ptr<Var> var_;
  unique_ptr<Exp> exp_;

 public:
  AssignExp(unique_ptr<Var> var, unique_ptr<Exp> exp)
      : var_(move(var)), exp_(move(exp)) {}
  Value *codegen() override;
};

class IfExp : Exp {
  unique_ptr<Exp> test_;
  unique_ptr<Exp> then_;
  unique_ptr<Exp> else_;

 public:
  IfExp(unique_ptr<Exp> test, unique_ptr<Exp> then, unique_ptr<Exp> elsee)
      : test_(move(test)), then_(move(then)), else_(move(elsee)) {}
  Value *codegen() override;
};

class WhileExp : Exp {
  unique_ptr<Exp> test_;
  unique_ptr<Exp> body_;

 public:
  WhileExp(unique_ptr<Exp> test, unique_ptr<Exp> body)
      : test_(move(test)), body_(move(body)) {}
  Value *codegen() override;
};

class ForExp : Exp {
  unique_ptr<Var> var_;
  unique_ptr<Exp> low_;
  unique_ptr<Exp> high_;
  unique_ptr<Exp> body_;
  // bool escape;

 public:
  ForExp(unique_ptr<Var> var, unique_ptr<Exp> low, unique_ptr<Exp> high,
         unique_ptr<Exp> body)
      : var_(move(var)),
        low_(move(low)),
        high_(move(high)),
        body_(move(body)) {}
  Value *codegen() override;
};

class BreakExp : Exp {
  // dummpy body
 public:
  BreakExp() = default;
  Value *codegen() override;
};

class LetExp : Exp {
  vector<unique_ptr<Dec>> decs_;
  unique_ptr<Exp> body_;

 public:
  LetExp(vector<unique_ptr<Dec>> decs, unique_ptr<Exp> body)
      : decs_(move(decs)), body_(move(body)) {}
  Value *codegen() override;
};

class ArrayExp : Exp {
  string type_;
  unique_ptr<Exp> size_;
  unique_ptr<Exp> init_;

 public:
  ArrayExp(string type, unique_ptr<Exp> size, unique_ptr<Exp> init)
      : type_(move(type)), size_(move(size)), init_(move(init)) {}
  Value *codegen() override;
};

class Prototype {
  vector<unique_ptr<Field>> params_;
  string result_;

 public:
  Prototype(vector<unique_ptr<Field>> params, string result)
      : params_(move(params)), result_(move(result)) {}
};

class FunctionDec : Dec {
  string name_;
  unique_ptr<Prototype> proto_;
  unique_ptr<Exp> body_;

 public:
  FunctionDec(string name, unique_ptr<Prototype> proto, unique_ptr<Exp> body)
      : Dec(move(name)), proto_(move(proto)), body_(move(body)) {}
  Value *codegen() override;
};

class VarDec : Dec {
  string type_;
  unique_ptr<Exp> init_;
  // bool escape;
 public:
  VarDec(string name, string type, unique_ptr<Exp> init)
      : Dec(move(name)), type_(move(type)), init_(move(init)) {}
  Value *codegen() override;
};

class TypeDec : Dec {
  vector<unique_ptr<Type>> types_;

 public:
  TypeDec(string name, vector<unique_ptr<Type>> types)
      : Dec(move(name)), types_(move(types)) {}
  Value *codegen() override;
};

class NameType : Type {
  unique_ptr<Type> type_;

 public:
  NameType(string name, unique_ptr<Type> type)
      : Type(move(name)), type_(move(type)) {}
  Value *codegen() override;
};

class RecordType : Type {
  vector<unique_ptr<Field>> fields_;

 public:
  RecordType(string name, vector<unique_ptr<Field>> fields)
      : Type(move(name)), fields_(move(fields)) {}
};

class ArrayType : Type {
  unique_ptr<Type> type_;

 public:
  ArrayType(string name, unique_ptr<Type> type)
      : Type(move(name)), type_(move(type)) {}
};

}  // namespace AST

#endif  // AST_H
