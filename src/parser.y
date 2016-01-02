/*
No syntax checking, and incomplete semantic checking.
need to make a pointer from syntax tree to symbol table for access to data

*/
%{
#include "typecheckfuncs.h"
#include "parsetreefuncs.h"
#include "codegenfuncs.h"

int yylex();
int count = 0;
void type_error(const char *message);
%}

%union {
  char *token;
  typepointer p;
}

%type <token> type op binop relop logical_op
%type <p> prog dcl func stmt expr assg parm_types func_body more_body more_stmts many_opt_expr more_expr more_vars for_loop_expr var_decl opt_assg opt_expr many_opt_paren_expr more_types more_funcs

%token <token> ID
%token <token> EXTERN RETURN_
%token <token> VOID_ CHAR_ INT_
%token <token> IF_ ELSE_ WHILE_ FOR_
%token <token> CHARCON STRINGCON INTCON
%token <token> BOOL_AND BOOL_OR UNARY_MINUS
%token <token> EQUAL_EQUAL NOT_EQUAL LT_EQUAL GT_EQUAL
%token <token> ';' ',' '(' ')' '[' ']' '{' '}' '-' '!' '+' '*' '/' '<' '>'

%left BOOL_OR
%left BOOL_AND
%left EQUAL_EQUAL NOT_EQUAL
%left '<' LT_EQUAL GT_EQUAL '>'
%left '+'  '-'
%left '*'  '/'
%right '!' UNARY_MINUS

%nonassoc EXPR
%nonassoc THEN_
%nonassoc ELSE_

%%
prog	: dcl ';' prog { $$ = NULL; }
	| func prog { $$ = NULL; }
	| /* empty */ { $$ = NULL; }
;
dcl	: type var_decl more_vars { $$ = NULL; }
	
 	| type ID '(' parm_types ')' { addProto($2, $1, 0); addVar($2, $1, "func", 0);  assertNoPreviousFuncDefin($2); previoustype = $1; } more_funcs { $$ = NULL; }
 	
 	| EXTERN type ID '(' parm_types ')' { addProto($3, $2, 1);
 	addVar($3, $2, "func", 0); assertNoPreviousFuncDefin($2); previoustype = $1; } more_funcs { $$ = NULL; }
 	
 	| EXTERN VOID_ ID '(' parm_types ')' { addProto($3, $2, 1); 
 	addVar($3, $2, "func", 0);   assertNoPreviousFuncDefin($2); previoustype = $1; } more_funcs { struct parseTree *p = mkNewParseNode("func decl", mkNewParseNode($3, $5, NULL), $8); $$ = p; compileFunction(p); }
 	
 	| VOID_ ID '(' parm_types ')' { addProto($2, $1, 0); addVar($2, $1, "func", 0); assertNoPreviousFuncDefin($2); previoustype = $1; } more_funcs { $$ = NULL; }
;
type	: CHAR_ { $$ = $1; previoustype = $1; }
 	| INT_ { $$ = $1; previoustype = $1; }
;
parm_types: VOID_ { $$ = mkNewParseNode("param", mkNewParseNode($1, NULL, NULL), NULL); }

	| type ID '[' ']' { addParam($2, $1, "array", 0); } more_types { $$ = mkNewParseNode("param", mkNewParseNode($2, NULL, NULL), $6); }
	
	| type ID { addParam($2, $1, "var", 0); } more_types { $$ = mkNewParseNode("param", mkNewParseNode($2, NULL, NULL), $4); }
;
func	: type ID '(' parm_types ')' { addVar($2, $1, "func", 0); checkProto($2); }
	'{' { clearParam($2); }
	func_body '}' { islocal=0; struct parseTree *p = mkNewParseNode("func decl", mkNewParseNode($2, $4, $9), mkNewParseNode("func end", NULL, NULL)); $$ = p; compileFunction(p); }

 	| VOID_ ID '(' parm_types ')' { addVar($2, $1, "func", 0); checkProto($2); }
 	'{' { islocal = 1; clearParam($2); }
 	func_body '}' { islocal=0; struct parseTree *p = mkNewParseNode("func decl", mkNewParseNode($2, $4, $9), mkNewParseNode("func end", NULL, NULL)); $$ = p; compileFunction(p); }
;
stmt	: IF_ '(' expr ')' stmt %prec THEN_ { $$ = mkNewParseNode("temp", mkNewParseIfNode($1, $3, $5, NULL), mkNewParseNode("end stmt", NULL, NULL)); }

	| IF_ '(' expr ')' stmt ELSE_ stmt { $$ = mkNewParseNode("temp", mkNewParseIfNode($1, $3, $5, mkNewParseNode("end stmt", NULL, NULL)), mkNewParseNode("else", $7, mkNewParseNode("end stmt", NULL, NULL))); }
	
	| WHILE_ '(' expr ')' stmt { $$ = mkNewParseNode("temp", mkNewParseNode($1, $3, $5), mkNewParseNode("end stmt", NULL, NULL)); }
	
	| FOR_ '(' for_loop_expr ')' stmt { $$ = mkNewParseNode("temp", mkNewParseNode($1, $3, $5), mkNewParseNode("end stmt", NULL, NULL)); }
	
	| RETURN_ expr ';' { validateNonVoidFunc(); $$ = mkNewParseNode($1, $2, NULL); }
	
	| RETURN_ ';' { validateVoidFunc(); $$ = mkNewParseNode($1, NULL, NULL); }
	
	| assg ';' { $$ = mkNewParseNode("assg", $1, NULL); }
	
	| ID '(' many_opt_expr ')' ';' { validateIDStmt($1); $$ = mkNewParseNode("procedure call", mkNewParseNode($1, $3, NULL), NULL); }
	
	| '{' more_stmts '}' { $$ = mkNewParseNode($2->name, $2, mkNewParseNode("end stmt", NULL, NULL)); }
	
	| ';' { $$ = NULL; }
;
assg	: ID '[' expr ']' '=' expr { struct list *id = getNearest($1); if (id->vartype != ARRAY) { type_error("Id with brackets must be an array."); } if (!typecheck($3->type, INT)) { type_error("Expression in brackets must evaluate to an int."); } if (!typecheck(id->datatype, $6->type)) { type_error("Types of both sides of an assignment must match."); } $$ = mkNewParseNode("=", mkNewParseNode($1, $3, NULL), $6); }
	
	| ID '=' expr { struct list *id = getNearest($1);
	if (!typecheck(id->datatype, INT)) {
	  type_error("Types of both sides of an assignment must be compatible."); }
	if (id->vartype != VAR) {
	  type_error("Left operand must be a variable."); }
	$$ = mkNewParseNode("=", mkNewParseNode($1, NULL, NULL), $3); }
;
expr	: '-' %prec UNARY_MINUS expr { 
	if (typecheck($2->type, INT)) {$$ = mkNewParseNode("Uminus", $2, NULL);} 
	else {type_error("Minus expects an int compatible operand.");} 
	}

	| '!' expr { 
	if (typecheck($2->type, INT)) {$$ = mkNewParseNode($1, $2, NULL);} 
	else {type_error("\'!\' expects an int compatible operand.");} 
	}
	
	| %prec EXPR expr op expr { 
	if (typecheck($1->type, $3->type)) { $$ = mkNewParseNode($2, $1, $3); $$->type = $1->type; } 
	else { type_error("Expression types do not match."); } 
	}
	
	| ID { validateIDExpr($1); } many_opt_paren_expr {
	struct list *id = getNearest($1);
	if (id->datatype != VOID) {
	  $$ = mkNewParseNode($1, $3, NULL); $$->type = id->datatype;
	} else {
	  type_error("Function in an expression cannot have a return type of void");
	}
	
	// check func params
	}
	
	| '(' expr ')' { $$ = $2; }
	| INTCON { $$ = mkNewParseNode($1, NULL, NULL); $$->type = INT; }
	
	| CHARCON { $$ = mkNewParseNode($1, NULL, NULL); $$->type = CHAR; }
	
	| STRINGCON { $$ = mkNewParseNode($1, NULL, NULL); $$->type = STRING; addGlobalConst($1); struct list *tmp = findConstant($1); tmp->name = generateNewName(count); count++; tmp->vartype = CONST; }
;
op	: binop { $$ = $1; }
	| relop { $$ = $1; }
	| logical_op { $$ = $1; }
;
binop	: '+' { $$ = $1; }
 	| '-' { $$ = $1; }
 	| '*' { $$ = $1; }
 	| '/' { $$ = $1; }
;
relop	: EQUAL_EQUAL { $$ = $1; }
 	| NOT_EQUAL { $$ = $1; }
 	| LT_EQUAL { $$ = $1; }
 	| '<' { $$ = $1; }
 	| GT_EQUAL { $$ = $1; }
 	| '>' { $$ = $1; }
;
logical_op : BOOL_AND { $$ = $1; }
 	| BOOL_OR { $$ = $1; }
;
more_vars : ',' var_decl more_vars { $$ = mkNewParseNode(previoustype, $2, $3); }
	  | /* empty */ { $$ = NULL; }
;
func_body : more_body more_stmts { $$ = mkNewParseNode("body", $1, $2); }
;
more_body : type { previoustype = $1; } var_decl more_vars ';' more_body { $$ = mkNewParseNode("local decl", mkNewParseNode($1, $3, $4), mkNewParseNode("body", NULL, $6)); }

	  | /* empty */ { $$ = NULL; }
;
more_stmts: stmt more_stmts { $$ = mkNewParseNode("statement", $1, $2); }
	  | /* empty */ { $$ = NULL; }
;
more_funcs  : ',' ID '(' parm_types ')' { addVar($2, previoustype, "func", 0); } more_funcs { $$ = mkNewParseNode($2, $4, $7); }

	  | /* empty */ { $$ = NULL; }
;
more_types: ',' type ID '[' ']' { addParam($3, $2, "array", 0); } more_types { $$ = mkNewParseNode($2, mkNewParseNode($3, NULL, NULL), $7); }

	  | ',' type ID { addParam($3, $2, "var", 0); } more_types { $$ = mkNewParseNode($2, mkNewParseNode($3, NULL, NULL), $5); }
	  
	  | /* empty */ { $$ = NULL; }
;
more_expr : ',' expr more_expr { $$ = mkNewParseNode("another expr", $2, $3); }
	  | /* empty */ { $$ = NULL; }
;
opt_assg  : assg { $$ = $1; }
	  | /* empty */ { $$ = NULL; }
;
opt_expr  : expr { 
	  if (!typecheck($1->type, INT)) {
	  type_error("Expression must evaluate to an int.");
	  } else {
	  $$ = $1;
	  }
	  
	  }
	  
	  | /* empty */ { $$ = NULL; }
;
many_opt_expr: expr more_expr { $$ = mkNewParseNode("opt expressions", $1, $2); }
	  | /* empty */ { /*$$ = NULL;*/$$ = mkNewParseNode("opt expressions", mkNewParseNode("none", NULL, NULL), NULL); }
;
many_opt_paren_expr: '(' many_opt_expr ')' { $$ = $2;/*mkNewParseNode("params", $2, NULL);*/ }

	  | '[' expr ']' {
	    if (typecheck($2->type, INT)) {
	      $$ = $2;
	    } else {
	      type_error("Array expression must evaluate to an int.");
	    }
	  }
	  | /* empty */ { $$ = NULL; }
;
for_loop_expr : opt_assg ';' opt_expr ';' opt_assg { $$ = mkNewParseIfNode("for loop expr", $1, $3, mkNewParseNode("temp", $5, mkNewParseNode("end for init", NULL, NULL))); }
;
var_decl  : ID { addVar($1, previoustype, "var", 0); $$ = mkNewParseNode($1, NULL, NULL); }
	  | ID '[' INTCON ']' { addVar($1, previoustype, "array", atoi($3)); $$ = mkNewParseNode($1, mkNewParseNode($3, NULL, NULL), NULL); }
;
%%

int yyerror(const char *str) {
  fprintf(stderr, "Line %d: %s\n", linenum, str);
  return 1;
}

void type_error(const char *str) {
  fprintf(stderr, "Line %d: %s\n", linenum, str);
}

int yywrap() {
  return 1;
}

int main() {
  startCompile();
  yyparse();
  closeFile();
  return 1;
}
