/* from Modern Compiler Implementation in C
 */

#ifndef UTIL_H
#define UTIL_H
#include <assert.h>
#include <stdbool.h>

typedef const char *string;

#define TRUE 1
#define FALSE 0

void *checked_malloc(int);
string String(char *);

typedef struct U_boolList_ *U_boolList;
struct U_boolList_ {
  bool head;
  U_boolList tail;
};
U_boolList UboolList(bool head, U_boolList tail);

#endif  // UTIL_H
