%{
#include <stdio.h>
#include "header.h"
#include "y.tab.h"

int linenum = 1;

void scanning() {
  //printf("%s", yytext);
  yylval.token = strdup(yytext);
}

%}

%option nounput
%option noinput

spaces		[ \t\r]+
id		[a-zA-Z]([a-zA-Z]|[0-9]|_)*
intcon		[0-9]([0-9])*
charcon		\'([\0-&\(-\[\]-~])\'|\'\n\'|\'\0\'
stringcon	\"[\0-\t\v-!#-~]*\"
comment		\/\*[^(\*\/)]*\*\/
int		"int"
for		"for"
else		"else"
while		"while"
if		"if"
return		"return"
void		"void"
extern		"extern"
char		"char"
%%
{spaces}	
\n		linenum++;
{comment}
{int}		scanning(); return INT_;
;		scanning(); return ';';
,		scanning(); return ',';
{extern}	scanning(); return EXTERN;
\(		scanning(); return '(';
\)		scanning(); return ')';
{void}		scanning(); return VOID_;
\[		scanning(); return '[';
\]		scanning(); return ']';
{char}		scanning(); return CHAR_;
\{		scanning(); return '{';
\}		scanning(); return '}';
{if}		scanning(); return IF_;
{else}		scanning(); return ELSE_;
{while}		scanning(); return WHILE_;
{for}		scanning(); return FOR_;
{return}	scanning(); return RETURN_;
-		scanning(); return '-';
!		scanning(); return '!';
\+		scanning(); return '+';
\*		scanning(); return '*';
\/		scanning(); return '/';
=		scanning(); return '=';
==		scanning(); return EQUAL_EQUAL;
\!\=		scanning(); return NOT_EQUAL;
\<\=		scanning(); return LT_EQUAL;
\>\=		scanning(); return GT_EQUAL;
\&\&		scanning(); return BOOL_AND;
\|\|		scanning(); return BOOL_OR;
\>		scanning(); return '>';
\<		scanning(); return '<';
{id}		scanning(); return ID;
{intcon}	scanning(); return INTCON;
{charcon}	scanning(); return CHARCON;
{stringcon}	scanning(); return STRINGCON;
%%
