%{
#include <iostream>
#include <AST/ast.h>
#include <llvm/ADT/STLExtras.h>
using namespace AST;

int yylex(void); /* function prototype */

std::unique_ptr<Node> root;

void yyerror(char *s)
{
  std::cerr<<s<<std::endl;
}
%}


%union {
  int pos;
  int ival;
  std::string *sval;
  Var *var;
  Exp *exp;
  Dec *dec;
  Type *type;
  Field *field;
  //Efield *efield;
  FunctionDec *functiondec;
  NameType *nametype;
  std::vector<std::unique_ptr<Exp>> *expList;
  std::vector<std::unique_ptr<Dec>> *decList;
  std::vector<std::unique_ptr<Type>> *typeList;
  std::vector<std::unique_ptr<Field>> *fieldList;
  //std::vector<std::unique_ptr<Efield>> *efieldList;
  std::vector<std::unique_ptr<NameType>> *nametypeList;
}

%token <sval> ID STRING
%token <ival> INT

%token
  COMMA COLON SEMICOLON LPAREN RPAREN LBRACK RBRACK LBRACE RBRACE DOT
  PLUS MINUS TIMES DIVIDE EQ NEQ LT LE GT GE AND OR ASSIGN
  ARRAY IF THEN ELSE WHILE FOR TO DO LET IN END OF BREAK NIL FUNCTION VAR TYPE

%type <var> lvalue
%type <exp> root exp let cond
%type <expList> arglist nonarglist explist
%type <type> ty
%type <dec> dec vardec tydecs fundecs
%type <decList> decs
%type <fieldList> tyfields tyfields1
%type <field> tyfield
%type <functiondec> fundec
%type <nametype> tydec
%type <fieldList> reclist nonreclist
%type <sval> id

%nonassoc LOW
%nonassoc THEN DO TYPE FUNCTION ID
%nonassoc ASSIGN LBRACK ELSE OF COMMA
%left OR
%left AND
%nonassoc EQ NEQ LE LT GT GE
%left PLUS MINUS
%left TIMES DIVIDE
%left UMINUS

%start prog

%%

prog:             root                              {root=llvm::make_unique<Node>($1);}
                ;

root:           /* empty */                         {$$=nullptr;}
                | exp								{$$=std::move($1);}

exp:              INT                       		{$$=new IntExp($1);}
                | STRING							{$$=new StringExp(*$1);}
                | NIL								{$$=new NilExp();}
                | lvalue							{$$=new VarExp(llvm::make_unique<Var>($1));}
                | lvalue ASSIGN exp					{$$=new AssignExp(llvm::make_unique<Var>($1), llvm::make_unique<Exp>($3));}
                | LPAREN explist RPAREN				{$$=new SequenceExp(*$2);}
                | cond						    	{$$=$1;}
                | let						    	{$$=$1;}
                | exp OR exp						{$$=new IfExp(llvm::make_unique<Exp>($1), llvm::make_unique<Exp>(new IntExp(1)), llvm::make_unique<Exp>($3));}
                | exp AND exp						{$$=new IfExp(llvm::make_unique<Exp>($1), llvm::make_unique<Exp>($3), llvm::make_unique<Exp>(new IntExp(0)));}
                | exp LT exp						{$$=new BinaryExp(BinaryExp::LTH, llvm::make_unique<Exp>($1), llvm::make_unique<Exp>($3));}
                | exp GT exp						{$$=new BinaryExp(BinaryExp::GTH, llvm::make_unique<Exp>($1), llvm::make_unique<Exp>($3));}
                | exp LE exp						{$$=new BinaryExp(BinaryExp::LEQ, llvm::make_unique<Exp>($1), llvm::make_unique<Exp>($3));}
                | exp GE exp						{$$=new BinaryExp(BinaryExp::GEQ, llvm::make_unique<Exp>($1), llvm::make_unique<Exp>($3));}
                | exp PLUS exp						{$$=new BinaryExp(BinaryExp::ADD, llvm::make_unique<Exp>($1), llvm::make_unique<Exp>($3));}
                | exp MINUS exp						{$$=new BinaryExp(BinaryExp::SUB, llvm::make_unique<Exp>($1), llvm::make_unique<Exp>($3));}
                | exp TIMES exp						{$$=new BinaryExp(BinaryExp::MUL, llvm::make_unique<Exp>($1), llvm::make_unique<Exp>($3));}
                | exp DIVIDE exp					{$$=new BinaryExp(BinaryExp::DIV, llvm::make_unique<Exp>($1), llvm::make_unique<Exp>($3));}
                | MINUS exp %prec UMINUS			{$$=new BinaryExp(BinaryExp::SUB, llvm::make_unique<Exp>(new IntExp(0)), llvm::make_unique<Exp>($2));}
                | exp EQ exp						{$$=new BinaryExp(BinaryExp::EQU, llvm::make_unique<Exp>($1), llvm::make_unique<Exp>($3));}
                | exp NEQ exp						{$$=new BinaryExp(BinaryExp::NEQU, llvm::make_unique<Exp>($1), llvm::make_unique<Exp>($3));}
                | id LPAREN arglist RPAREN			{$$=new CallExp(*$1, *$3);}
                | id LBRACK exp RBRACK OF exp		{$$=new ArrayExp(*$1, llvm::make_unique<Exp>($3), llvm::make_unique<Exp>($6));}
                | id LBRACE reclist RBRACE			{$$=new RecordExp(*$1, *$3);}
                | BREAK								{$$=new BreakExp();}
                ;

reclist:        /* empty */                         {$$=nullptr;}
                | nonreclist						{$$=$1;}
                ;

nonreclist:       id EQ exp							{$$=new std::vector<std::unique_ptr<Field>>();$$->push_back(llvm::make_unique<Field>(new Field(*$1, llvm::make_unique<Exp>($3))));}
                | id EQ exp	COMMA nonreclist		{$$=$5; $5->push_back(llvm::make_unique<Field>(new Field(*$1, llvm::make_unique<Exp>($3))));}

let:              LET decs IN explist END			{$$=new LetExp(*$2, llvm::make_unique<Exp>(new SequenceExp(*$4)));}
                ;

arglist:        /* empty */							{$$=nullptr;}
                | nonarglist						{$$=$1;}
                ;

nonarglist:       exp								{$$=new std::vector<std::unique_ptr<Exp>>();$$->push_back(llvm::make_unique<Exp>($1));}
                | exp COMMA nonarglist				{$$=$3; $3->push_back(llvm::make_unique<Exp>($1));}
                ;

decs:           /* empty */							{$$=nullptr;}
                | dec decs							{$$=$2; $2->push_back(llvm::make_unique<Dec>($2));}
                ;

dec:              tydecs 							{$$=$1;}
                | vardec							{$$=$1;}
                | fundecs							{$$=$1;}
                ;

tydecs:           tydec	%prec LOW                   {$$=new TypeDec(A_NametyList($1, NULL));}
                | tydec tydecs						{$$=new TypeDec(A_NametyList($1, $2->u.type));}
                ;

lvalue:           id %prec LOW                      {$$=new SimpleVar(*$1);}
                | id LBRACK exp RBRACK 				{$$=new SubscriptVar(llvm::make_unique<Var>(new SimpleVar(*$1)), llvm::make_unique<Exp>($3));}
                | lvalue LBRACK exp RBRACK			{$$=new SubscriptVar(llvm::make_unique<Var>($1), llvm::make_unique<Exp>($3));}
                | lvalue DOT id						{$$=new FieldVar(llvm::make_unique<Var>($1), *$3);}
                ;

explist:		/* empty */							{$$=NULL;}
                | exp								{$$=new std::vector<std::unique_ptr<Exp>>(); $$->push_back(llvm::make_unique<Exp>($1));}
                | exp SEMICOLON explist				{$$=$3; $3->push_back(llvm::make_unique<Exp>($1));}
                ;

cond:             IF exp THEN exp ELSE exp			{$$=new IfExp(llvm::make_unique<Exp>($2), llvm::make_unique<Exp>($4), llvm::make_unique<Exp>($6));}
                | IF exp THEN exp					{$$=new IfExp(llvm::make_unique<Exp>($2), llvm::make_unique<Exp>($4), nullptr);}
                | WHILE exp DO exp					{$$=new WhileExp(llvm::make_unique<Exp>($2), llvm::make_unique<Exp>($4));}
                | FOR id ASSIGN exp TO exp DO exp	{$$=new ForExp(llvm::make_unique<Var>($2), llvm::make_unique<Exp>($4), llvm::make_unique<Exp>($6), llvm::make_unique<Exp>($8));}
                ;

tydec:            TYPE id EQ ty						{$$=new NameType(*$2, llvm::make_unique<Type>($4));}
                ;

ty:               id								{$$=NameTy(EM_tokPos, $1);}
                | LBRACE tyfields RBRACE			{$$=RecordTy(EM_tokPos, $2);}
                | ARRAY OF id						{$$=A_ArrayTy(EM_tokPos, $3);}
                ;

tyfields:       /* empty */							{$$=nullptr;}
                | tyfields1                 		{$$=$1;}
                ;

tyfields1:        tyfield							{$$=new std::vector<std::unique_ptr<Field>>(); $$->push_back(llvm::make_unique<Field>($1));}
                | tyfield COMMA tyfields1			{$$=$3; $3->push_back(llvm::make_unique<Field>($1));}
                ;

tyfield:          id COLON id						{$$=new Field(*$1, llvm::make_unique<Exp>($3));}
                ;

vardec:           VAR id ASSIGN exp					{$$=new VarDec(*$2, nullptr, llvm::make_unique<Exp>($4));}
                | VAR id COLON id ASSIGN exp		{$$=new VarDec(*$2, *$4, llvm::make_unique<Exp>($6));}
                ;

id:               ID								{$$=std::string($1);}
                ;

fundecs:          fundec %prec LOW                  {$$=A_FunctionDec(EM_tokPos, A_FundecList($1, NULL));}
                | fundec fundecs					{$$=A_FunctionDec(EM_tokPos, A_FundecList($1, $2->u.function));}
                ;

fundec:           FUNCTION id LPAREN tyfields RPAREN EQ exp				{$$=A_Fundec(EM_tokPos, $2, $4, NULL, $7);}
                | FUNCTION id LPAREN tyfields RPAREN COLON id EQ exp	{$$=A_Fundec(EM_tokPos, $2, $4, $7, $9);}
                ;




