#include "env.h"

E_enventry E_VarEntry(Tr_access access, Ty_ty ty) {}
E_enventry E_FunEntry(Tr_level level, Temp_label label, Ty_tyList formals,
                      Ty_ty result) {}

S_table E_base_tenv(void) {}
S_table E_base_venv(void) {}
