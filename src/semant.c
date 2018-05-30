#include "semant.h"
#include "absyn.h"
#include "translate.h"
#include "types.h"

struct expty {
  Tr_exp exp;
  Ty_ty ty;
};

// Constructor
static struct expty expTy(Tr_exp exp, Ty_ty ty) {
  struct expty e;
  e.exp = exp;
  e.ty = ty;
  return e;
}

static struct expty transVar(S_table venv, S_table tenv, A_var v) {}
static struct expty transExp(S_table venv, S_table tenv, A_exp a) {
  switch (a->kind) {
    case A_varExp:
      break;
    case A_nilExp:
      break;
    case A_intExp:
      break;
    case A_stringExp:
      break;
    case A_callExp:
      break;
    case A_opExp:
      break;
    case A_recordExp:
      break;
    case A_seqExp:
      break;
    case A_assignExp:
      break;
    case A_ifExp:
      break;
    case A_whileExp:
      break;
    case A_forExp:
      break;
    case A_breakExp:
      break;
    case A_letExp:
      break;
    case A_arrayExp:
      break;
  }
}
static void transDec(S_table venv, S_table tenv, A_dec d);
static Ty_ty transTy(S_table tenv, A_ty a);

void SEM_transProg(A_exp exp) {}
