#include "semant.h"
#include "absyn.h"
#include "env.h"
#include "errormsg.h"
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

static struct expty dummy_expTy() { return expTy(Tr_noExp(), Ty_Int()); }

static Ty_ty actual_ty(Ty_ty ty) {
  if (ty->kind == Ty_name)
    return actual_ty(ty->u.name.ty);
  else
    return ty;
}

static struct expty transExp(S_table venv, S_table tenv, A_exp a);

static struct expty transVar(S_table venv, S_table tenv, A_var v) {
  switch (v->kind) {
    case A_simpleVar: {
      E_enventry x = S_look(venv, v->u.simple);
      if (x && x->kind == E_varEntry)
        return expTy(NULL, actual_ty(x->u.var.ty));
      break;
    }
    case A_fieldVar: {
      struct expty var = transVar(venv, tenv, v->u.field.var);
      if (var.ty->kind != Ty_record) {
        EM_error(v->u.field.var->pos, "record required");
        return dummy_expTy();
      }
      Ty_fieldList f = NULL;
      int fieldOffset = 0;
      for (f = var.ty->u.record; f; f = f->tail, ++fieldOffset) {
        if (f->head->name == v->u.field.sym)
          return expTy(Tr_fieldVar(var.exp, fieldOffset),
                       actual_ty(f->head->ty));
      }
      return dummy_expTy();
      break;
    }
    case A_subscriptVar: {
      struct expty var = transVar(venv, tenv, v->u.subscript.var);
      if (var.ty->kind != Ty_array) {
        EM_error(v->u.subscript.var->pos, "array required");
        return dummy_expTy();
      }
      struct expty index = transExp(venv, tenv, v->u.subscript.exp);
      if (index.ty->kind != Ty_int) {
        EM_error(v->u.subscript.exp->pos, "integer required");
        return dummy_expTy();
      }
      return expTy(NULL, actual_ty(var.ty->u.array));
      break;
    }
  }
  assert(0);
}

static struct expty transExp(S_table venv, S_table tenv, A_exp a) {
  switch (a->kind) {
    case A_varExp: {
      return transVar(venv, tenv, a->u.var);
      break;
    }
    case A_nilExp: {
      return expTy(Tr_nilExp(), Ty_Nil());
      break;
    }
    case A_intExp: {
      return expTy(Tr_intExp(a->u.intt), Ty_Int());
      break;
    }
    case A_stringExp: {
      return expTy(Tr_stringExp(a->u.stringg), Ty_String());
      break;
    }
    case A_callExp: {
      break;
    }
    case A_opExp: {
      A_oper oper = a->u.op.oper;
      struct expty left = transExp(venv, tenv, a->u.op.left);
      struct expty right = transExp(venv, tenv, a->u.op.right);
      if (oper == A_plusOp) {
        if (left.ty->kind != Ty_int)
          EM_error(a->u.op.left->pos, "interger required");
        if (right.ty->kind != Ty_int)
          EM_error(a->u.op.right->pos, "integer required");
        return expTy(NULL, Ty_Int());
      }
      break;
    }
    case A_recordExp: {
      Ty_ty type = S_look(tenv, a->u.record.typ);
      if (!type) {
        EM_error(a->pos, "predefined type required");
        return dummy_expTy();
      }
      type = actual_ty(type);
      if (type->kind != Ty_record) {
        EM_error(a->pos, "record required");
        return dummy_expTy();
      }
      Ty_fieldList f = type->u.record;
      int n = 0;
      // TODO
      break;
    }
    case A_seqExp: {
      struct expty exp = expTy(Tr_noExp(), Ty_Void());
      A_expList seq;
      for (seq = a->u.seq; seq; seq = seq->tail) {
        exp = transExp(venv, tenv, seq->head);
        Tr_ExpList_prepend(list, exp.exp);
      }
      if (Tr_ExpList_empty(list)) break;
    }
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
  assert(0);
}
static void transDec(S_table venv, S_table tenv, A_dec d) {
  switch (d->kind) {
    case A_functionDec: {

      break;
    }
    case A_varDec: {
      struct expty e = transExp(venv, tenv, d->u.var.init);
      S_enter(venv, d->u.var.var, E_VarEntry(e.ty));
      break;
    }
    case A_typeDec: {
      S_enter(tenv, d->u.type->head->name, transTy(d->u.type->head->ty));
      break;
    }
  }
  assert(0);
}
static Ty_ty transTy(S_table tenv, A_ty a) {

}

void SEM_transProg(A_exp exp) {
  S_table tenv = E_base_tenv();
  S_table venv = E_base_venv();
  transExp(venv, tenv, exp);
  return Tr_getResult();
}
