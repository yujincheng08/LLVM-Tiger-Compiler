#ifndef SYMBOLTABLE_H
#define SYMBOLTABLE_H

#include <deque>

template <typename T>
class SymbolTable {
  std::deque<T> stack_;

 public:
  SymbolTable();
  T lookup() const;
  void push(T const &val);
};

#endif  // SYMBOLTABLE_H
