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
  FunctionDec *functionDec;
  TypeDec *typeDec;
  std::vector<std::unique_ptr<Exp>> *expList;
  std::vector<std::unique_ptr<Dec>> *decList;
  std::vector<std::unique_ptr<Type>> *typeList;
  std::vector<std::unique_ptr<Field>> *fieldList;
  std::vector<std::unique_ptr<FieldExp>> *fieldExpList;
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
%type <dec> dec vardec
%type <decList> decs
%type <fieldList> tyfields
%type <field> tyfield
%type <functionDec> fundec
%type <typeDec> tydec
%type <fieldExpList> reclist
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

prog:             root                              {root=std::unique_ptr<Node>($1);}
                ;

root:           /* empty */                         {$$=nullptr;}
                | exp								{$$=std::move($1);}

exp:              INT                       		{$$=new IntExp($1);}
                | STRING							{$$=new StringExp(*$1); delete $1;}
                | NIL								{$$=new NilExp();}
                | lvalue							{$$=new VarExp(std::unique_ptr<Var>($1));}
                | lvalue ASSIGN exp					{$$=new AssignExp(std::unique_ptr<Var>($1), std::unique_ptr<Exp>($3));}
                | LPAREN explist RPAREN				{$$=new SequenceExp(std::move(*$2));}
                | cond						    	{$$=$1;}
                | let						    	{$$=$1;}
                | exp OR exp						{$$=new IfExp(std::unique_ptr<Exp>($1), std::unique_ptr<Exp>(new IntExp(1)), std::unique_ptr<Exp>($3));}
                | exp AND exp						{$$=new IfExp(std::unique_ptr<Exp>($1), std::unique_ptr<Exp>($3), std::unique_ptr<Exp>(new IntExp(0)));}
                | exp LT exp						{$$=new BinaryExp(BinaryExp::LTH, std::unique_ptr<Exp>($1), std::unique_ptr<Exp>($3));}
                | exp GT exp						{$$=new BinaryExp(BinaryExp::GTH, std::unique_ptr<Exp>($1), std::unique_ptr<Exp>($3));}
                | exp LE exp						{$$=new BinaryExp(BinaryExp::LEQ, std::unique_ptr<Exp>($1), std::unique_ptr<Exp>($3));}
                | exp GE exp						{$$=new BinaryExp(BinaryExp::GEQ, std::unique_ptr<Exp>($1), std::unique_ptr<Exp>($3));}
                | exp PLUS exp						{$$=new BinaryExp(BinaryExp::ADD, std::unique_ptr<Exp>($1), std::unique_ptr<Exp>($3));}
                | exp MINUS exp						{$$=new BinaryExp(BinaryExp::SUB, std::unique_ptr<Exp>($1), std::unique_ptr<Exp>($3));}
                | exp TIMES exp						{$$=new BinaryExp(BinaryExp::MUL, std::unique_ptr<Exp>($1), std::unique_ptr<Exp>($3));}
                | exp DIVIDE exp					{$$=new BinaryExp(BinaryExp::DIV, std::unique_ptr<Exp>($1), std::unique_ptr<Exp>($3));}
                | MINUS exp %prec UMINUS			{$$=new BinaryExp(BinaryExp::SUB, std::unique_ptr<Exp>(new IntExp(0)), std::unique_ptr<Exp>($2));}
                | exp EQ exp						{$$=new BinaryExp(BinaryExp::EQU, std::unique_ptr<Exp>($1), std::unique_ptr<Exp>($3));}
                | exp NEQ exp						{$$=new BinaryExp(BinaryExp::NEQU, std::unique_ptr<Exp>($1), std::unique_ptr<Exp>($3));}
                | id LPAREN arglist RPAREN			{$$=new CallExp(*$1, std::move(*$3)); delete $1;}
                | id LBRACK exp RBRACK OF exp		{$$=new ArrayExp(*$1, std::unique_ptr<Exp>($3), std::unique_ptr<Exp>($6)); delete $1;}
                | id LBRACE reclist RBRACE			{$$=new RecordExp(*$1, std::move(*$3)); delete $1;}
                | BREAK								{$$=new BreakExp();}
                ;

reclist:        /* empty */                         {$$=new std::vector<std::unique_ptr<FieldExp>>();}
                | id EQ exp							{$$=new std::vector<std::unique_ptr<FieldExp>>();
                                                                                 $$->push_back(llvm::make_unique<FieldExp>(*$1, std::unique_ptr<Exp>($3)));
                                                                                 delete $1;}
                | id EQ exp	COMMA reclist		{$$=$5; $5->push_back(llvm::make_unique<FieldExp>(*$1, std::unique_ptr<Exp>($3))); delete $1;}

let:              LET decs IN explist END			{$$=new LetExp(std::move(*$2), llvm::make_unique<SequenceExp>(std::move(*$4)));}
                ;

arglist:        /* empty */							{$$=new std::vector<std::unique_ptr<Exp>>();}
                | nonarglist						{$$=$1;}
                ;

nonarglist:       exp								{$$=new std::vector<std::unique_ptr<Exp>>();$$->push_back(std::unique_ptr<Exp>($1));}
                | exp COMMA nonarglist				{$$=$3; $3->push_back(std::unique_ptr<Exp>($1));}
                ;

decs:           /* empty */							{$$=new std::vector<std::unique_ptr<Dec>>();}
                | dec decs							{$$=$2; $2->push_back(std::unique_ptr<Dec>($1));}
                ;

dec:              tydec 							{$$=$1;}
                | vardec							{$$=$1;}
                | fundec							{$$=$1;}
                ;

// tydecs:           tydec	%prec LOW                   {$$=new TypeDec(A_NametyList($1, NULL));}
                //| tydec tydecs						{$$=new TypeDec(A_NametyList($1, $2->u.type));}
                //;

lvalue:           id %prec LOW                      {$$=new SimpleVar(*$1); delete $1;}
                | id LBRACK exp RBRACK 				{$$=new SubscriptVar(llvm::make_unique<SimpleVar>(*$1), std::unique_ptr<Exp>($3)); delete $1;}
                | lvalue LBRACK exp RBRACK			{$$=new SubscriptVar(std::unique_ptr<Var>($1), std::unique_ptr<Exp>($3));}
                | lvalue DOT id						{$$=new FieldVar(std::unique_ptr<Var>($1), *$3); delete $3;}
                ;

explist:		/* empty */							{$$=new std::vector<std::unique_ptr<Exp>>();}
                | exp								{$$=new std::vector<std::unique_ptr<Exp>>(); $$->push_back(std::unique_ptr<Exp>($1));}
                | exp SEMICOLON explist				{$$=$3; $3->push_back(std::unique_ptr<Exp>($1));}
                ;

cond:             IF exp THEN exp ELSE exp			{$$=new IfExp(std::unique_ptr<Exp>($2), std::unique_ptr<Exp>($4), std::unique_ptr<Exp>($6));}
                | IF exp THEN exp					{$$=new IfExp(std::unique_ptr<Exp>($2), std::unique_ptr<Exp>($4), nullptr);}
                | WHILE exp DO exp					{$$=new WhileExp(std::unique_ptr<Exp>($2), std::unique_ptr<Exp>($4));}
                | FOR id ASSIGN exp TO exp DO exp	{$$=new ForExp(*$2, std::unique_ptr<Exp>($4), std::unique_ptr<Exp>($6), std::unique_ptr<Exp>($8)); delete $2;}
                ;

tydec:            TYPE id EQ ty						{$$=new TypeDec(*$2, std::unique_ptr<Type>($4)); delete $2;}
                ;

ty:               id								{$$=new NameType(*$1); delete $1;}
                | LBRACE tyfields RBRACE			{$$=new RecordType(std::move(*$2));}
                | ARRAY OF id						{$$=new ArrayType(*$3); delete $3;}
                ;

tyfields:       /* empty */							{$$=new std::vector<std::unique_Ptr<Field>>();}
                | tyfield							{$$=new std::vector<std::unique_ptr<Field>>(); $$->push_back(std::unique_ptr<Field>($1));}
                | tyfield COMMA tyfields			{$$=$3; $3->push_back(std::unique_ptr<Field>($1));}
                ;

tyfield:          id COLON id						{$$=new Field(*$1, llvm::make_unique<NameType>(*$3)); delete $1; delete $3;}
                ;

vardec:           VAR id ASSIGN exp					{$$=new VarDec(*$2, "", std::unique_ptr<Exp>($4)); delete $2;}
                | VAR id COLON id ASSIGN exp		{$$=new VarDec(*$2, *$4, std::unique_ptr<Exp>($6)); delete $2; delete $4;}
                ;

id:               ID								{$$=$1;}
                ;

//fundecs:          fundec %prec LOW                  {$$=A_FunctionDec(EM_tokPos, A_FundecList($1, NULL));}
                //| fundec fundecs					{$$=A_FunctionDec(EM_tokPos, A_FundecList($1, $2->u.function));}
                //;

fundec:           FUNCTION id LPAREN tyfields RPAREN EQ exp				{$$=new FunctionDec(*$2, llvm::make_unique<Prototype>(*$2, std::move(*$4), "nil"), std::unique_ptr<Exp>($7)); delete $2;}
                | FUNCTION id LPAREN tyfields RPAREN COLON id EQ exp	{$$=new FunctionDec(*$2, llvm::make_unique<Prototype>(*$2, std::move(*$4), *$7), std::unique_ptr<Exp>($9)); delete $2;}
                ;




