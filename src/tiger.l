%{
#include <string>
#include <sstream>
#include <AST/ast.h>
#include <iostream>
using namespace AST;
#include <cstring>
#include <unistd.h>
#include "tiger_yacc.h"

#define BUFSIZE 65535
#define ADJ {charPos+=yyleng;}

int charPos = 1;
std::stringstream strbuf;
char *strptr = NULL;
int commentDepth = 0;

extern "C" int yywrap(void)
{
 charPos=1;
 return 1;
}

%}
%x COMMENT STR
%%
[ \t]	{ADJ; continue;}
(\n|\r\n)  {ADJ; /*EM_newline();*/ continue;}
"*"   {ADJ; return TIMES;}
"/"   {ADJ; return DIVIDE;}
"/*"  {ADJ; BEGIN(COMMENT); commentDepth++;}
<COMMENT>{
	"/*" {ADJ; commentDepth++;}
	"*/" {ADJ; if (--commentDepth == 0) BEGIN(INITIAL);}
	[^\n] {ADJ;}
        (\n|\r\n)	{ADJ; /*EM_newline();*/}
}
"array"    {ADJ; return ARRAY;}
"break"    {ADJ; return BREAK;}
"do"	   {ADJ; return DO;}
"end"      {ADJ; return END;}
"else"     {ADJ; return ELSE;}
"for"  	   {ADJ; return FOR;}
"function" {ADJ; return FUNCTION;}
"if"	   {ADJ; return IF;}
"in"       {ADJ; return IN;}
"let"	   {ADJ; return LET;}
"nil"	   {ADJ; return NIL;}
"of"	   {ADJ; return OF;}
"then"     {ADJ; return THEN;}
"to"	   {ADJ; return TO;}
"type"     {ADJ; return TYPE;}
"while"    {ADJ; return WHILE;}
"var"      {ADJ; return VAR;}
[a-zA-Z][a-zA-Z0-9_]*    {ADJ; std::cout<<yytext<<std::endl; tigerlval.sval=new string(yytext); return ID;}
[0-9]+	   {ADJ; tigerlval.ival=atoi(yytext); return INT;}
"+"        {ADJ; return PLUS;}
"-"        {ADJ; return MINUS;}
"&"	       {ADJ; return AND;}
"|"	       {ADJ; return OR;}
","	       {ADJ; return COMMA;}
"."        {ADJ; return DOT;}
":"	       {ADJ; return COLON;}
";"	       {ADJ; return SEMICOLON;}
"("	       {ADJ; return LPAREN;}
")"        {ADJ; return RPAREN;}
"["        {ADJ; return LBRACK;}
"]"        {ADJ; return RBRACK;}
"{"        {ADJ; return LBRACE;}
"}"        {ADJ; return RBRACE;}
"="        {ADJ; return EQ;}
"<>"       {ADJ; return NEQ;}
"<"        {ADJ; return LT;}
"<="       {ADJ; return LE;}
">"        {ADJ; return GT;}
">="       {ADJ; return GE;}
":="       {ADJ; return ASSIGN;}

\" {ADJ; BEGIN(STR); }
<STR>{
        \" 			     {ADJ; tigerlval.sval=new std::string(strbuf.str()); BEGIN(INITIAL); return STRING;}
        \\n			     {ADJ; strbuf << "\n";}
        \\t			     {ADJ; strbuf << "\t";}
    \\^[GHIJLM]	     {ADJ; strbuf<<yytext;}
        \\[0-9]{3}	     {ADJ; strbuf<<yytext;}
        \\\"    		 {ADJ; strbuf<<yytext;}
	\\[ \n\t\r\f]+\\ {ADJ;}
        \\(.|\n)	     {ADJ; std::cerr << "illegal token" << std::endl;}
        (\n|\r\n)	     {ADJ; std::cerr << "illegal token" << std::endl;}
        [^\"\\\n(\r\n)]+ {ADJ; strbuf<<yytext;}
}
.	 {ADJ; std::cerr << "illegal token" << std::endl;}
%%

