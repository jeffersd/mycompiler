#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "header.h"

extern int yyparse();
extern int linenum;

int islocal = 0;
int yyerror(const char *message);
char *previoustype;

struct list {
  char *name;
  char *value;
  type_t vartype;
  type_t datatype;
  int arraysize;
  int isextern;
  //typepointer tp;
  struct list *next;
  struct paramlist *myparams;
};

struct paramlist {
  char *name;
  type_t datatype;
  type_t vartype;
  struct paramlist *next;
};

struct protolist {
  char *name;
  type_t datatype;
  int isextern;
  struct protolist *next;
  struct paramlist *myparamhead;
};

struct errorlist {
  char *name;
  type_t datatype;
  type_t vartype;
  struct errorlist *next;
};

struct list *globalhead = NULL;
struct list *localhead = NULL;
struct protolist *protohead = NULL;
struct errorlist *errorhead = NULL;
struct paramlist *paramhead = NULL;

struct tree *root = NULL;

void setPreviousType(char *ptype) {
  previoustype = ptype;
}

type_t parseEnum(char *str) {
  type_t newtype = VAR;
  if (!strcmp(str, "array")) {
    newtype = ARRAY;
  } else if (!strcmp(str, "int")) {
    newtype = INT;
  } else if (!strcmp(str, "char")) {
    newtype = CHAR;
  } else if (!strcmp(str, "func")) {
    newtype = FUNC;
  } else if (!strcmp(str, "void")) {
    newtype = VOID;
  } else if (!strcmp(str, "var")) {
    newtype = VAR;
  }
  return newtype;
}

void addGlobal(char *newname, type_t newdatatype, type_t newvartype, int newsize) {
  struct list *temp = NULL;
  temp = malloc( sizeof(struct list) );
  temp -> name = newname;
  temp -> datatype = newdatatype;
  temp -> vartype = newvartype;
  temp -> arraysize = newsize;
  temp -> value = NULL;
  temp -> next = NULL;
  
  if (globalhead == NULL) {
    globalhead = temp;
  } else {
    temp -> next = globalhead;
    globalhead = temp;
  }
}


void addLocal(char *newname, type_t newdatatype, type_t newvartype, int newsize) {
  struct list *temp = NULL;
  temp = malloc( sizeof(struct list) );
  temp -> name = newname;
  temp -> datatype = newdatatype;
  temp -> vartype = newvartype;
  temp -> arraysize = newsize;
  temp -> value = NULL;
  temp -> next = NULL;
  
  if (localhead == NULL) {
    localhead = temp;
  } else {
    temp -> next = localhead;
    localhead = temp;
  }
}

void addError(char *newname, type_t newdatatype, type_t newvartype) {
  struct errorlist *temp = NULL;
  temp = malloc( sizeof(struct errorlist) );
  temp -> name = newname;
  temp -> datatype = newdatatype;
  temp -> vartype = newvartype;
  temp -> next = NULL;
  
  if (errorhead == NULL) {
    errorhead = temp;
  } else {
    temp -> next = errorhead;
    errorhead = temp;
  }
}

int isInGlobal(char *targetname) {
  struct list *current = globalhead;
  while(current != NULL) {
    if (!strcmp(targetname, current -> name)) {
      return 1;
    }
    current = current -> next;
  }
  
  return 0;
}

struct list * getFromGlobal(char *targetname) {
  struct list *current = globalhead;
  while(current != NULL) {
    if (!strcmp(targetname, current -> name)) {
      return current;
    }
    current = current -> next;
  }
  
  return NULL;
}

int isInLocal(char *targetname) {
  struct list *current = localhead;
  while(current != NULL) {
    if (!strcmp(targetname, current -> name)) {
      return 1;
    }
    current = current -> next;
  }
  return 0;
}

struct list * getFromLocal(char *targetname) {
  struct list *current = localhead;
  while(current != NULL) {
    if (!strcmp(targetname, current -> name)) {
      return current;
    }
    current = current -> next;
  }
  return NULL;
}

int countLocal() {
  struct list *current = localhead;
  int count = 0;
  while (current != NULL) {
    count++;
    current = current->next;
  }
  
  return count;
}

struct list * getNearest(char *targetname) {
  struct list *target = NULL;
  target = getFromLocal(targetname);
  if (target == NULL) {
    target = getFromGlobal(targetname);
  }
  return target;
}

int isInProto(char *targetname) {
  struct protolist *current = protohead;
  while(current != NULL) {
    if (!strcmp(targetname, current -> name)) {
      return 1;
    }
    current = current -> next;
  }
  return 0;
}

int isInError(char *targetname) {
  struct errorlist *current = errorhead;
  while(current != NULL) {
    if (!strcmp(targetname, current -> name)) {
      return 1;
    }
    current = current -> next;
  }
  return 0;
}

int isInParam(char *targetname) {
  struct paramlist *current = paramhead;
  while(current != NULL) {
    if (!strcmp(targetname, current -> name)) {
      return 1;
    }
    current = current -> next;
  }
  
  return 0;
}

void addParam(char *newname, char *dtype_asstring, char *vtype_asstring, int newsize) {
  if (isInParam(newname)) {
    yyerror("Id already used in parameter list.");
  }
  type_t newdatatype = parseEnum(dtype_asstring);
  type_t newvartype = parseEnum(vtype_asstring);
  struct paramlist *temp = NULL;
  temp = malloc( sizeof(struct paramlist) );
  temp -> name = newname;
  temp -> datatype = newdatatype;
  temp -> vartype = newvartype;
  temp -> next = NULL;
  
  if (paramhead == NULL) {
    paramhead = temp;
  } else {
    temp -> next = paramhead;
    paramhead = temp;
  }
}

void addProto(char *newname, char *pnewdatatype, int pisextern) {
  if (isInProto(newname)) {
    yyerror("Function already has a prototype.");
  } else {
    type_t newdatatype = parseEnum(pnewdatatype);
    
    struct protolist *temp = NULL;
    temp = malloc( sizeof(struct protolist) );
    temp -> name = newname;
    temp -> datatype = newdatatype;
    temp -> isextern = pisextern;
    temp -> next = NULL;
    temp -> myparamhead = paramhead;
    paramhead = NULL;
    
    if (protohead == NULL) {
      protohead = temp;
    } else {
      temp -> next = protohead;
      protohead = temp;
    }
  }
}

void checkProto(char *funcname) {
  /*
   * If a function has a prototype, then the types of the formals at its definition must match (i.e., be the same), in number and order, the types of the argument in its prototype; and the type of the return value at its definition must match the type of the return value at its prototype. The prototype, if present, must precede the definition of the function.
   */
  if (isInProto(funcname)) {
    
    // params must match number and order of type
    struct protolist *current = protohead;
    while (strcmp(funcname, current -> name)) {
      current = current -> next;
    }
    
    if (current -> isextern) {
      yyerror("External function cannot be defined here.");
    }
    
    // now compare paramlist and current's param list
    struct paramlist *protoparam = current -> myparamhead;
    struct paramlist *funcparam = paramhead;
    while ( protoparam != NULL && funcparam != NULL ) {
      if ( ((protoparam -> datatype) != (funcparam -> datatype)) || 
	((protoparam -> vartype) != (funcparam -> vartype))) {
	//return 0;
	yyerror("Function's parameter types must match its prototype's.");
	break;
      }
      
      protoparam = protoparam -> next;
      funcparam = funcparam -> next;
    }
    
    
    
    while ( protoparam != NULL && funcparam != NULL ) {
      protoparam = protoparam -> next;
      funcparam = funcparam -> next;
    }
    
    if ( (funcparam == NULL && protoparam != NULL) || // lengths were unequal
      (funcparam != NULL && protoparam == NULL) ) {
      yyerror("Number of arguments in function differs from its prototype's.");
      //return 0;
    }
    
    // return value must match
    struct list *func = globalhead;
    while (func != NULL) {
      if (!strcmp(func -> name, funcname)) {
	break;
      }
      func = func -> next;
    }
    if (current -> datatype != func -> datatype) {
      yyerror("Function return type must match its prototype's.");
      //return 0;
    }
    
  }
  
  //return 1; // func satisfies prototype vacuously or conditionally
}

// prototype must come first
void assertNoPreviousFuncDefin(char *funcproto) {
  if (isInGlobal(funcproto)) {
    yyerror("Function prototype must come before function.");
  }
}

int isVarExists(char *name/*, char *type, int size*/) {
  if ( (!isInLocal(name)) && (!isInGlobal(name)) ) {
    yyerror("Id has not been declared.");
    return 0;
  }
  return 1;
}
  

void addVar(char *newname, char *datatype, char *vartype, int newsize) {
  if ( (islocal && isInLocal(newname)) || (!islocal && isInGlobal(newname)) ) {
    if (!isInError(newname)) {
      yyerror("Id is already declared.");
      type_t vtype = parseEnum(vartype);
      type_t dtype = parseEnum(datatype);
      addError(newname, dtype, vtype);
    }
  }
  
  if (newsize < 0) {
    yyerror("Array has a negative size.");
  }

  type_t newdatatype = parseEnum(datatype);
  type_t newvartype = parseEnum(vartype);
  
  if(islocal) {
    addLocal(newname, newdatatype, newvartype, newsize);
  } else {
    addGlobal(newname, newdatatype, newvartype, newsize);
  }
}

void clearGlobal() {
  while (globalhead != NULL) {
    struct list *current = globalhead;
    globalhead = globalhead -> next;
    free(current);
  }
}
  
void clearLocal() {
  islocal = 0;
  while (localhead != NULL) {
    struct list *current = localhead;
    localhead = localhead -> next;
    free(current); 
  }
}

void clearProto() {
  while (protohead != NULL) {
    struct protolist *current = protohead;
    protohead = protohead -> next;
    free(current); 
  }
}

void deleteParam() {
  while (paramhead != NULL) {
    struct paramlist *current = paramhead;
    paramhead = paramhead -> next;
    free(current); 
  }
}

void clearParam(char *id) {
  islocal = 1;
  while (paramhead != NULL) {
    struct list *func = getFromGlobal(id);
    
    func -> myparams = paramhead;
    
    // get node
    struct paramlist *current = paramhead;
    paramhead = paramhead -> next;
    // add it to global or local
    char *paramname = current -> name;
    type_t paramdatatype = current -> datatype;
    type_t paramvartype = current -> vartype;
    
    if (islocal) {
      addLocal(paramname, paramdatatype, paramvartype, 0);
    } /*else {
      struct protohead currentproto = NULL;
      addGlobal(paramname, paramdatatype, paramvartype, paramarraysize);
    }*/
    
    // remove from param
    //free(current);
  }
}

// end of declaration funcs
// start expr checker funcs

void validateIDExpr(char *id) {
  if (isVarExists(id)) {
    struct list *variable = getFromLocal(id);
    if (variable == NULL) {
      variable = getFromGlobal(id);
    }
    
    if (variable -> datatype == VOID) {
      yyerror("Function in an expression cannot have a return type of void.");
    }
    
    
  }
  
}

void validateIDStmt(char *id) {
  if (isVarExists(id)) {
    struct list *variable = getFromLocal(id);
    if (variable == NULL) {
      variable = getFromGlobal(id);
    }
    
    if (variable -> datatype != VOID) {
      yyerror("Function in a statement must have a return type of void.");
    }
    
  }
  
  
}

void validateNonVoidFunc() {
  if (islocal) {
    // get return type of the parent function, if its void, give error
    struct list *func = globalhead; // the parent function is the most recent
    if (func -> datatype == VOID) {
      yyerror("A function cannot return a value if its return type is void.");
    }
    
  }
  
}

void validateVoidFunc() {
  if (islocal) {
    // get return type of the parent function, if its void, give error
    struct list *func = globalhead; // the parent function is the most recent
    if (func -> datatype != VOID) {
      yyerror("A function must return a value if its return type is not void.");
    }
    
  }
  
}

// end validation functions

// start tree functions
/*typepointer newnode(type_t newtype) {
  typepointer newnode;
  newnode = malloc( sizeof(struct treenode) );
  newnode -> type = newtype;
  newnode -> left = NULL;
  newnode -> right = NULL;
  return newnode ;
}*/

int typecheck(type_t first, type_t second) {
  if (first == second) {
    return 1;
  }
  if (first == CHAR && second == INT) {
    return 1;
  }
  if (first == INT && second == CHAR) {
    return 1;
  }
  
  return 0;
}

void addGlobalConst(char *global_const) {
  struct list *temp = NULL;
  temp = malloc( sizeof(struct list) );
  temp -> name = "9const";
  temp -> datatype = 0;
  temp -> vartype = 9;
  temp -> arraysize = 0;
  temp -> next = NULL;
  temp -> value = global_const;
  
  if (globalhead == NULL) {
    globalhead = temp;
  } else {
    temp -> next = globalhead;
    globalhead = temp;
  }
}

struct list * findConstant(char *targetvalue) {
  struct list *current = globalhead;
  while(current != NULL) {
    if ( (current->value != NULL) && (!strcmp(targetvalue, current -> value)) ) {
      return current;
    }
    current = current -> next;
  }
  
  return NULL;
}
