# include <ctype.h>
# include <stdio.h>
# include <string.h>

int labelCounter = 0;
int hasNoReturnValue = 0;
struct quad *currentQuad = NULL;

char * mkNewLabel() {
  char *prefix = "assembly_label_";
  char *number = malloc( sizeof(char) * 22 );;
  sprintf(number, "%d", labelCounter);
  char *newLabel = concat(prefix, number);
  labelCounter++;
  return newLabel;
}

char * getArrayId(char *src) {
  char *temp = malloc(strlen(src)+1);//+1 for the zero-terminator
  strcpy(temp, src);
  char *result = strtok(temp, " ");
  return result;
}

char * getArrayReference(char *src) {
  char *temp = malloc(strlen(src)+1);//+1 for the zero-terminator
  strcpy(temp, src);
  char *result = strtok(temp, " ");
  result = strtok(NULL, " ");
  return result;
}

char * getVariableName(char *userName) {
  char *assemblyName = concat("assembly_variable_", userName);
  return assemblyName;
}

int isExprSign(char *str) {
  if (strcmp(str, "+") == 0) { return 1; }
  if (strcmp(str, "-") == 0) { return 1; }
  if (strcmp(str, "*") == 0) { return 1; }
  if (strcmp(str, "/") == 0) { return 1; }
  if (strcmp(str, "!") == 0) { return 1; }
  if (strcmp(str, "<") == 0) { return 1; }
  
  // func return value?
  //if (strcmp(str, "") == 0) { return 1; }
  
  return 0;
}

int isNumber(char *str) {
  if (atoi(str)) {
    return 1;
  }
  if (strcmp(str, "0") == 0) {
    return 1;
  }
  
  return 0;
}

int writeVariable(char *name, int registerNum) {
  if (strstr(name, " ") != NULL) { // we have an array
    char *arrayId = getArrayId(name);
    printf("la\t$t%d, %s\t# load the array address\n", registerNum+1, getVariableName(arrayId));
    int index = atoi(getArrayReference(name));
    struct list *temp = getNearest(arrayId);
    if (temp->datatype == INT) {
      index = index * 4;
    }
    printf("addi\t$t%d, $t%d, ", registerNum+1, registerNum+1);
    printf("%d\t# increment the address\n", index);
    printf("lw\t$t%d, 0($t%d)\t# load the value\n", registerNum, registerNum+1);
    return 1;
  } else if (isNumber(name)) { // we have a number
    printf("li\t$t%d, %s\t# load the intermediate value\n", registerNum, name);
  } else if (getNearest(name) != NULL) { // we have a variable
    printf("lw\t$t%d, %s\t# load the ram destination\n", registerNum, getVariableName(name));
  } else { // we have a char literal
    printf("lb\t$t%d, %s\t# load the char literal\n", registerNum, name);
  }
  return -1;
}

// writes the assembly code to save the return value to the stack
void writePushReturnValue() {
  printf("sw\t$s0, 0($sp)\t# save the return value\n");
}

void writePopReturnValue() {
  printf("lw\t$s0, 0($sp)\t# load the return value\n");
}

struct quad * getNextQuad(struct quad *currquad) {
  if (currquad != NULL) {
    if (currquad == currentQuad) {
      currentQuad = currentQuad -> next;
      return currquad -> next;
    } else {
      currentQuad = currquad;
      return currentQuad;
    }
  }
  return NULL;
}

void writeFunctionProlog() {
  int num_variables = countLocal(); // get the number of local variables
  num_variables = num_variables * 4; // multiply by 4 for number of bytes
  num_variables = num_variables + 12; // the basis after saving our fp and sp
  printf("# prologue, set up stack pointer\n");
  printf("sw\t$fp, -4($sp)\n");
  printf("sw\t$ra, -8($sp)\n");
  printf("la\t$fp, 0($sp)\n");
  printf("la\t$sp, -%d($sp)\n\n", num_variables);
}

void writeSum(struct quad *currquad) {
  printf("add\t$t0, $t0, $t1\t# load sum\n");
}
void writeMinus(struct quad *currquad) {
  printf("sub\t$t0, $t0, $t1\t# load difference\n");
}
void writeMult(struct quad *currquad) {
  printf("mult\t$t0, $t1\t# calculate product\n");
  printf("mflo\t$t0\t# load lo product into register (hi is truncated)\n");
}
void writeDiv(struct quad *currquad) {
  printf("div\t$t0, $t1\t# calc quotient\n");
  printf("mflo\t$t0\t# load quotient (hi is remainder)\n");
}
void writeUnaryMinus(struct quad *currquad) {
  printf("li\t$t0, 0\t# load zero\n");
  printf("sub\t$t0, $t0, $t1\t# load difference\n");
}
void writeNot(struct quad *currquad) {
  printf("li\t$t1, -1\t# load -1\n");
  printf("xor\t$t0, $t0, $t1\t# xor with -1 to bitwise negate\n");
}

void parseExpression(struct quad *currquad) {
  writeVariable(currquad->src1, 0);
  if (strcmp(currquad->src2, "null") != 0) {
    writeVariable(currquad->src2, 1);
  }
  if (strcmp(currquad->op_type, "+") == 0) {
    writeSum(currquad);
  } else if (strcmp(currquad->op_type, "-") == 0) {
    writeMinus(currquad);
  } else if (strcmp(currquad->op_type, "*") == 0) {
    writeMult(currquad);
  } else if (strcmp(currquad->op_type, "/") == 0) {
    writeDiv(currquad);
  } else if (strcmp(currquad->op_type, "!") == 0) {
    writeNot(currquad);
  } else {
    writeUnaryMinus(currquad);
  }
}

void writeProcedureCall(struct quad *currquad) {
  // need to save parameter for procedure to use
  // first four go to a0-a3, rest go to stack
  printf("# procedure call\n");
  if (strcmp(currquad->dest, "local") == 0) {
    
  } else if (findConstant(currquad->src2) != NULL) {
    struct list *constant = findConstant(currquad->src2);
    printf("la\t$a0, %s\t# save parameter to $a0\n", getVariableName(constant->name));
  } else if (strstr(currquad->src2, " ") != NULL) { // we have an array
    char *arrayId = getArrayId(currquad->src2);
    if (strcmp(currquad->src1, "print_string") == 0) {
      printf("la\t$a0, %s\t# load address of array\n", getVariableName(arrayId));
    } else {
      char *arrayIndex = getArrayReference(currquad->src2);
      printf("la\t$t4, %s\t# load address of array\n", getVariableName(arrayId));
      int index = atoi(arrayIndex);
      struct list *var = getNearest(arrayId);
      if (var->datatype == INT) {
	index = index * 4;
      }
      printf("addi\t$t3, $t4, %d\t# compute indexed array to $a0\n", index);
      printf("lw\t$a0, 0($t3)\t# saved indexed array to $a0\n");
    }
  } else if (isNumber(currquad->src2)) { // for int constants
    printf("li\t$a0, %s\t# save parameter to $a0\n", currquad->src2);
  } else if (strcmp(currquad->src2, "none") == 0) {
    // do nothing
  } else {
    printf("lw\t$a0, %s\t# save parameter to $a0\n", getVariableName(currquad->src2));
  }
  printf("jal\t%s\t# jump and link\n\n", getVariableName(currquad->src1));
}


void writeAssignment(struct quad *currquad) {
  printf("# assignment\n");
  int index = -1;
  char *dest = currquad -> dest;
  if (strstr(dest, " ") != NULL) {
    dest = getArrayId(dest);
  }
  
  // first is the assignee
  if (strstr(currquad->dest, " ") != NULL) { // if we have an array
    dest = getArrayId(currquad -> dest);
    printf("la\t$t3, %s\t# load the array address\n", getVariableName(dest));
    index = atoi(getArrayReference(currquad->dest));
    struct list *temp = getNearest(dest);
    if (temp->datatype == INT) {
      index = index * 4;
    }
    printf("addi\t$t3, $t3, %d\t# increment the address\n", index); 
  } else { // it is a scalar variable
    printf("lw\t$t0, %s\t# load the ram destination\n", getVariableName(dest));
  }  
  // next is the value
  int index1 = -1;
  
  if (strstr(currquad->src1, " ") != NULL) { // we have an array
    char *src = getArrayId(currquad->src1);
    char *srcIndex = getArrayReference(currquad->src1);
    printf("la\t$t1, %s\t# load the array address\n", getVariableName(src));
    index1 = atoi(srcIndex);
    struct list *temp = getNearest(src);
    if (temp->datatype == INT) {
      index1 = index1 * 4;
    }
    //printf("addi\t$t2, $t2, %d\t# increment the address\n", index1);
    printf("lw\t$t0, %d($t1)\t# load the actual value\n", index1);
  } else if (isNumber(currquad->src1)) {
    printf("li\t$t0, %s\t# load the value to be assigned\n", currquad->src1);
  } else if (isInGlobal(currquad->src1)) { // it is a func return value or a var
    struct list *temp = getFromGlobal(currquad->src1);
    switch(temp->vartype) {
      case VAR: printf("lw\t$t0, %s\t# load the value to be assigned\n", getVariableName(currquad->src1));
	break;
      case FUNC: writeProcedureCall(mkNewQuadNode(NULL, currquad->src1, "params", "global")); writePopReturnValue();
	break;
      default:
	break;
    }
  } else if (findConstant(currquad->src1) != NULL) { // str const
    struct list *constant = findConstant(currquad->src1);
    printf("la\t$t0, %s\t# load the value\n", getVariableName(constant->name));
  } else if (isExprSign(currquad->src1)) { // it is an expression
    currquad = currquad->next;
    parseExpression(currquad);
  } else {
    printf("lb\t$t0, %s\t# load character constant\n", currquad->src1);
  }
  if (index == -1 && index1 == -1) {
    printf("sw\t$t0, %s\t# return the value to the ram dest\n\n", getVariableName(dest));
  } else if (index == -1) {
    printf("lw\t$t0, 0($t0)\t# load the value of the array\n");
    printf("sw\t$t0, %s\t# return the value to the ram dest\n\n", getVariableName(dest));
  } else if (index1 == -1) {
    printf("sw\t$t0, 0($t3)\t# return the value to the ram dest\n\n");
  } else {
    printf("lw\t$t0, 0($t0)\t# load the value of the array\n");
    printf("sw\t$t0, 0($t3)\t# return the value to the ram dest\n\n");
  }
} // end write assignment



void writeLoadParameter(struct quad *currquad) {
  // load the parameter for the procedure to use
  printf("# loading parameter\n");
  
  //printf("addi\t$sp, $sp, -8\t# decrement stack pointer by 4\n");
  //printf("sw\t$ra, 0($sp)\t# save variable to the stack\n");
  
  /*NOT NEEDED? CALLER SAVES PARAMS TO A0-A3 + STACK*/
  //printf("sw\t$a0, 4($sp)\t# save variable to the stack\n");
}

void writeProcedureReturn(int isreturn) {
  if (isreturn) {
    writePushReturnValue();
  }
  printf("jr\t$ra\t# jump to the return address\n\n");
}

void writeFunctionEpilogue(int isreturn) {
  //int num_variables = countLocal();
  printf("# epilogue, return stack pointer\n");
  printf("la\t$sp, ($fp)\n");
  printf("lw\t$ra, -8($sp)\n");
  printf("lw\t$fp, -4($sp)\n");
  writeProcedureReturn(isreturn);
}

void writeMainEnd() {
  printf("j\tend\t# jump to the end\n\n");
}

struct quad * writeIfBlock(); // declaration
struct quad * writeWhileBlock();
struct quad * writeForBlock();

void updateQuad(struct quad *currquad) {
  currentQuad = currquad;
}

int parseQuad(struct quad *currquad) {
  if (strcmp(currquad -> op_type, "if") == 0) {
    currquad = writeIfBlock(currquad);
    updateQuad(currquad);
    return 0;
  }
  
  if (strcmp(currquad -> op_type, "while") == 0) {
    currquad = writeWhileBlock(currquad);
    updateQuad(currquad);
    return 0;
  }
  if (strcmp(currquad -> op_type, "for") == 0) {
    currquad = writeForBlock(currquad);
    updateQuad(currquad);
    return 0;
  }
  
  if (strcmp(currquad -> op_type, "=") == 0) {
    writeAssignment(currquad);
    return 0;
  }
  
  if (strcmp(currquad -> op_type, "procedure call") == 0) {
    writeProcedureCall(currquad);
    return 0;
  }
  
  if (strcmp(currquad -> op_type, "load local") == 0) {
    writeLoadParameter(currquad);
    return 0;
  }
  
  if (strcmp(currquad -> op_type, "return") == 0) {
    //writePushReturnValue(currquad);
    return 1;
  }
  
  return 0;
  //return line;
}

char * getBranchType(char *op) {
  if (strcmp(op, "<") == 0) {
    return "bge";
  }
  if (strcmp(op, ">") == 0) {
    return "ble";
  }
  if (strcmp(op, "<=") == 0) {
    return "bgt";
  }
  if (strcmp(op, ">=") == 0) {
    return "blt";
  }
  if (strcmp(op, "==") == 0) {
    return "bne";
  }
  if (strcmp(op, "!=") == 0) {
    return "beq";
  }
  return NULL;
}

void writeEvalExpr(struct quad *currquad, char *falseDest) {
  char *branch = getBranchType(currquad->op_type);
  writeVariable(currquad->src1, 6);
  writeVariable(currquad->src2, 7);
    /*if (isNumber(currquad->src1)) {
      printf("li\t$t6, %s\n", currquad->src1);
    } else {
      printf("lw\t$t6, %s\n", getVariableName(currquad->src1));
    }
    if (isNumber(currquad->src2)) {
      printf("li\t$t7, %s\n", currquad->src2);
    } else {
      printf("lw\t$t7, %s\n", getVariableName(currquad->src2));
    }*/
    printf("%s\t$t6, $t7, ", branch);
    printf("%s\t# jump if boolean fails\n", falseDest);
}

struct quad * writeTrueCode(struct quad *currquad, char *endDest) {
  while (strcmp(currquad->op_type, "end stmt") != 0) {
    parseQuad(currquad);
    currquad = currquad->next;
  }
  printf("j\t%s\t# jump to end-if label\n", endDest);
  return currquad -> next;
}

struct quad * writeTrueLoop(struct quad *currquad, char *start, struct quad *forQuad) {
  while (strcmp(currquad->op_type, "end stmt") != 0) {
    parseQuad(currquad);
    currquad = currquad->next;
  }
  if (forQuad != NULL) {
    parseQuad(forQuad);
  }
  printf("j\t%s\t# jump to top label\n", start);
  return currquad -> next;
}

struct quad * writeFalseCode(struct quad *currquad, int hasElse, char *falseDest, char *endDest) {
  printf("%s:\t# false label\n", falseDest);
  if (hasElse) {
    while (strcmp(currquad->op_type, "end stmt") != 0) {
      parseQuad(currquad);
      currquad = currquad->next;
    }
  }
  printf("%s:\t# end-if label\n", endDest);
  return currquad->next;
}

struct quad * writeIfBlock(struct quad *currquad) {
  int hasElse = 0;
  char *falseDest = mkNewLabel();
  char *endDest = mkNewLabel();
  if (strcmp(currquad->dest, "null") != 0) {
    hasElse = 1;
  }
  currquad = currquad->next;
  printf("# conditional statement\n");
  writeEvalExpr(currquad, falseDest);
  currquad = writeTrueCode(currquad, endDest);
  currquad = writeFalseCode(currquad, hasElse, falseDest, endDest);
  return currquad;
}

struct quad * writeWhileBlock(struct quad *currquad) {
  currquad = currquad->next;
  char *loopLabel = mkNewLabel();
  char *falseDest = mkNewLabel();
  printf("# while loop\n");
  printf("%s:\t# start loop here\n", loopLabel);
  writeEvalExpr(currquad, falseDest);
  currquad = writeTrueLoop(currquad, loopLabel, NULL);
  printf("%s:\t# false label\n", falseDest);
  return currquad;
}

struct quad * writeForBlock(struct quad *currquad) {
  currquad = currquad->next;
  char *loopLabel = mkNewLabel();
  char *falseDest = mkNewLabel();
  printf("# for loop\n");
  while (strcmp(currquad->op_type, "end for init")!= 0) {
    if (getBranchType(currquad->op_type) != NULL) {
      break;
    }
    parseQuad(currquad);
    currquad = currquad->next;
  }
  struct quad *controlQuad = currquad->next;
  printf("%s:\t# start loop here\n", loopLabel);
  writeEvalExpr(currquad, falseDest);
  while (strcmp(currquad->op_type, "end for init")!= 0) {
    currquad = currquad->next;
  }
  currquad = writeTrueLoop(currquad, loopLabel, controlQuad);
  printf("%s:\t# false label\n", falseDest);
  return currquad;
}

void writePrintIntBody() {
  printf("li\t$v0, 1\t# 1 is the command for printing an int\n");
  printf("syscall\t# system interrupt\n");
  writeProcedureReturn(0);
}

void writePrintStringBody() {
  printf("li\t$v0, 4\t# 4 is the command for printing a string\n");
  printf("syscall\t# system interrupt\n");
  writeProcedureReturn(0);
}

struct quad * writeFuncBody(struct quad *currquad) {
  writeFunctionProlog();
  int isreturn = 0;
  while ( (currquad != NULL) && (strcmp(currquad->op_type, "func end") != 0) ) {
    isreturn = parseQuad(currquad);
    if ( (currentQuad != NULL) && (currquad != currentQuad) ) {
      currquad = currentQuad;
    } else {
      currquad = currquad -> next;
    }
  }
  writeFunctionEpilogue(isreturn);
  return currquad;
}
  
 /* walks the quad list and writes the appropriate assembly code to a file
  * This is the compiled program.
  */
void mapQuadListToAssembly() {
  struct quad *currquad = quad_head;
  while (currquad !=NULL) {
    // start by checking for global declarations
    if ( (strcmp(currquad -> op_type, "decl") == 0) || 
      (strstr(currquad -> op_type, "const") != NULL) ) {   
      printf("\n.data\t# variable declarations here\n\n");
      while (currquad != NULL) {
	if (strcmp(currquad -> op_type, "decl") == 0) { // make a variable
	  printf("%s:\t.space\t", getVariableName(currquad->src1));
	  printf("%s\n", currquad->src2);
	  if (atoi(currquad->src2) % 2 == 1) {
	  }
	} else if (strcmp(currquad -> op_type, "const") == 0) {
	  printf("%s:\t.asciiz\t", getVariableName(currquad->src1));
	  printf("%s\n", currquad->dest);
	} else { // make a constant
	  printf("%s:\t.asciiz\t", getVariableName(currquad->op_type));
	  printf("%s\n", currquad->src1);
	}
	printf("\t.align 2\n");
	currquad = currquad->next;
      }
      printf("\n");
      return;
    }
    if (strcmp(currquad -> op_type, "func decl") == 0) {
      while (currquad != NULL) {
	if ( strcmp(currquad->src1, "main") == 0 ) {
	  printf(".globl\t%s\n", currquad->src1);
	  printf("%s:\n", currquad->src1);
	} else {
	  printf(".globl\t%s\n", getVariableName(currquad->src1));
	  printf("%s:\n", getVariableName(currquad->src1));
	}
	if (strcmp(currquad -> src1, "print_int") == 0) {
	  writePrintIntBody();
	} else if (strcmp(currquad -> src1, "print_string") == 0) {
	  writePrintStringBody();
	} else {
	  currquad = writeFuncBody(currquad);
	}
	if (currquad != NULL) {
	  currquad = currquad -> next;
	}
      }
      printf("\n");
      return;
    }
    if (currquad != NULL) {
      currquad = currquad -> next;
    }
  }
}

void compileFunction(struct parseTree *p) {
  createQuadList(p);
  //printQuadList();
  mapQuadListToAssembly();
  clearQuadList();
  clearLocal();
}

void compileGlobals() {
  if (quad_head != NULL) {
    clearQuadList();
  }
  mapGlobalToQuad();
  mapQuadListToAssembly();
  clearQuadList();
}

void startCompile() {
  printf("# c-- program compiled by Dillon Jeffers for CSc 453\n");
  printf(".text\t# code here\n\n");
}

void closeFile() {
  compileGlobals();
  printf(".text\t# code here\n\n");
  printf("end:\t# end of program\n");
  printf("li\t$v0, 10\t# system call code for exit = 10\n");
  printf("syscall\t\t# call operating sys\n\n");
}
