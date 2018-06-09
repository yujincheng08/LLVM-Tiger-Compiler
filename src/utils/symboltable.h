#ifndef SYMBOLTABLE_H
#define SYMBOLTABLE_H

#include <deque>
#include <memory>
#include <string>
#include <unordered_map>

template <typename T>
class SymbolTable {
  std::deque<std::unordered_map<std::string, T *>> stack_;

 public:
  SymbolTable();
  ~SymbolTable();
  T *&operator[](std::string const &name);
  T *&lookup(std::string const &name);
  T *&lookupOne(std::string const &name);
  void push(std::string const &name, T *const val);
  void pop(std::string const &name);
  void popOne(std::string const &name);
  void enter();
  void exit();
  void reset();
};

template <typename T>
SymbolTable<T>::SymbolTable() {
  enter();
}

template <typename T>
SymbolTable<T>::~SymbolTable() {
  exit();
}

template <typename T>
T *&SymbolTable<T>::operator[](const std::string &name) {
  return lookup(name);
}

template <typename T>
T *&SymbolTable<T>::lookup(const std::string &name) {
  for (auto stack : stack_)
    if (stack[name]) return stack[name];
  return stack_.front()[name];
}

template <typename T>
T *&SymbolTable<T>::lookupOne(const std::string &name) {
  if (stack_.front()[name]) return stack_.front()[name];
  return stack_.front()[name];
}

template <typename T>
void SymbolTable<T>::push(const std::string &name, T *const val) {
  stack_.front()[name] = val;
}

template <typename T>
void SymbolTable<T>::popOne(const std::string &name) {
  stack_.front().erase(name);
}

template <typename T>
void SymbolTable<T>::enter() {
  stack_.push_front(std::unordered_map<std::string, T *>());
}

template <typename T>
void SymbolTable<T>::exit() {
  stack_.pop_front();
}

template <typename T>
void SymbolTable<T>::reset() {
  stack_.clear();
  enter();
}

#endif  // SYMBOLTABLE_H
