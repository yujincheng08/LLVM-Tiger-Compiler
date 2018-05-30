%{
#include <string.h>
#include <unistd.h>
extern "C" {
#include "errormsg.h"
#include "symbol.h"
#include "absyn.h"
#include "util.h"
}
#include "tiger_yacc.h"

#define BUFSIZE 65535
#define ADJ {EM_tokPos=charPos; charPos+=yyleng;}

int charPos = 1;
char strbuf[BUFSIZE+1];
char *strptr = NULL;
unsigned int strlength = 0;
int commentDepth = 0;

extern "C" int yywrap(void)
{
 charPos=1;
 return 1;
}

void setup(void)
{
	*strbuf = '\0';
	strlength = 0;
}

void append(string str)
{
    if (strlength + strlen(str) < BUFSIZE)
    {
        strcat(strbuf, str);
        strlength += strlen(str);
    }
    else
    {
        EM_error(EM_tokPos, "Out of range");
    }
}

%}
%x COMMENT STR
%%
[ \t]	{ADJ; continue;}
(\n|\r\n)  {ADJ; EM_newline(); continue;}
"*"   {ADJ; return TIMES;}
"/"   {ADJ; return DIVIDE;}
"/*"  {ADJ; BEGIN(COMMENT); commentDepth++;}
<COMMENT>{
	"/*" {ADJ; commentDepth++;}
	"*/" {ADJ; if (--commentDepth == 0) BEGIN(INITIAL);}
	[^\n] {ADJ;}
	(\n|\r\n)	{ADJ; EM_newline();}
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
[a-zA-Z][a-zA-Z0-9_]*    {ADJ; tigerlval.sval=yytext; return ID;}
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

\" {ADJ; BEGIN(STR); setup();}
<STR>{
        \" 			     {ADJ; tigerlval.sval=String(strbuf); BEGIN(INITIAL); return STRING;}
	\\n			     {ADJ; append("\n");}
	\\t			     {ADJ; append("\t");}
    \\^[GHIJLM]	     {ADJ; append(yytext);}
	\\[0-9]{3}	     {ADJ; append(yytext);}
	\\\"    		 {ADJ; append(yytext);}
	\\[ \n\t\r\f]+\\ {ADJ;}
	\\(.|\n)	     {ADJ; EM_error(EM_tokPos, "illegal token");}
	(\n|\r\n)	     {ADJ; EM_error(EM_tokPos, "illegal token");}
	[^\"\\\n(\r\n)]+ {ADJ; append(yytext);}
}
.	 {ADJ; EM_error(EM_tokPos,"illegal token");}
%%

