typedef enum {FUNCTION, BODY} op_type_t;

char* concat(char *s1, char *s2) {
  char *result = malloc(strlen(s1)+strlen(s2)+1);//+1 for the zero-terminator
  strcpy(result, s1);
  strcat(result, s2);
  return result;
}

struct quad {
  char *op_type;
  char *src1;
  char *src2;
  char *dest;
  struct quad *next;
};

struct quad *quad_head = NULL;


struct quad * getLastQuad() {
  struct quad *last = quad_head;
  if (last != NULL) {
    while (last -> next != NULL) {
      last = last -> next;
    }
  }
  
  return last;
}

void addNewQuadNode(struct quad *newQuad) {
  if (quad_head == NULL) {
    quad_head = newQuad;
  } else {
    struct quad *temp = getLastQuad();
    temp -> next = newQuad;
  }
}

struct quad * mkNewQuadNode(char *pop, char *psrc1, char *psrc2, char * pdest) {
  struct quad *newQuad = malloc( sizeof(struct quad) );
  newQuad -> op_type = pop;
  newQuad -> src1 = psrc1;
  newQuad -> src2 = psrc2;
  newQuad -> dest = pdest;
  newQuad -> next = NULL;
  
  return newQuad;
}


/*
// quad methods
void setQuadListHead(struct quad *node) {
  if (quad_head == NULL) {
    quad_head = node;
  }
}*/



void printQuadList() {
  int counter = 1;
  struct quad *currquad = quad_head;
  while(currquad != NULL) {
    printf("---------------\n");
    printf("Quad number: %d\n", counter);
    printf("Operation: %s\n", currquad->op_type);
    printf("Source: %s\n", currquad->src1);
    printf("Source: %s\n", currquad->src2);
    printf("Destination: %s\n", currquad->dest);
    printf("---------------\n");
    counter++;
    currquad = currquad -> next;
  }
}

/*
 typedef struct parseTree {
  char *name;
  type_t type;
  struct parseTree *left;
  struct parseTree *middle;
  struct parseTree *right;
  struct list *symlink;
} parsenode, *parseptr;
 */

struct parseTree *parseTree_root = NULL;

void setParseTreeRoot(struct parseTree *node) {
  parseTree_root = node;
}

struct parseTree * mkNewParseNode(char *newname, struct parseTree *newleft, struct parseTree *newright) {
  struct parseTree *newnode = malloc( sizeof(struct parseTree) );
  newnode -> name = newname;
  newnode -> left = newleft;
  newnode -> right = newright;
  newnode -> symlink = NULL;
  
  // set the symbol link
  if (isInLocal(newname)) {
    newnode -> symlink = getFromLocal(newname);
  } else if (isInGlobal(newname)) {
    newnode -> symlink = getFromGlobal(newname);
  }
  
  return newnode;
}

struct parseTree * mkNewParseIfNode(char *newname, struct parseTree *bool, struct parseTree *body, struct parseTree *otw) {
  struct parseTree *newnode = malloc( sizeof(struct parseTree) );
  newnode -> name = newname;
  newnode -> left = bool;
  newnode -> middle = body;
  newnode -> right = otw;
  return newnode;
}



/*Processing functions
 * 
 */
void hasSymbolLink(struct parseTree *node) {
  struct list *symbol = node -> symlink;
  if ( (symbol->vartype == FUNC) && (symbol->myparams != NULL) ) {
    struct quad *last = getLastQuad();
    // add the local variable
    if (strcmp(last->op_type, "func decl") == 0) {
      addNewQuadNode(mkNewQuadNode("load local", symbol->myparams->name, symbol->name, "local"));
    }
    
  }
}

int boolOperator(char *op){
  if (strcmp(op, "<") == 0) {
    return 1;
  }
  if (strcmp(op, ">") == 0) {
    return 1;
  }
  if (strcmp(op, "<=") == 0) {
    return 1;
  }
  if (strcmp(op, ">=") == 0) {
    return 1;
  }
  if (strcmp(op, "==") == 0) {
    return 1;
  }
  if (strcmp(op, "!=") == 0) {
    return 1;
  }
  return 0;
}

char * getVarSource(char *name, struct parseTree *node) {
  char *newName = name;
  if (node != NULL) {
    newName = concat(newName, " ");
    newName = concat(newName, node->name);
  }
  return newName;
}

void walkParseTree(struct parseTree *node) {
  if (node != NULL) {
    if (strcmp(node->name, "func decl") == 0) { // quad for a new procedure
      addNewQuadNode(mkNewQuadNode(node->name, node->left->name, node->left->left->name, "global"));
    }
    
    // add local parameter to quad list
    // if our parse node has a sym link, then it is a symbol
    // if it is a function, add its parameter to the quad list
    if (node->symlink != NULL) {
      hasSymbolLink(node);
    }
    
    if (strcmp(node->name, "=") == 0) {
      // get src1 and dest
      struct list *temp = getFromGlobal(node->left->name);
      char *dest = "destination";
      switch(temp->vartype) {
	case VAR: dest = node->left->name;
	  break;
	case FUNC: dest = node->left->name;
	  break;
	case ARRAY: dest = getVarSource(node->left->name, node->left->left);
	  break;
	default:
	  break;
      }
      
      temp = getFromGlobal(node->right->name);
      char *src = "source";
      if (temp == NULL) {
	src = node->right->name;
      } else {
	switch(temp->vartype) {
	  case VAR: src = node->right->name;
	    break;
	  case FUNC: src = node->right->name;
	    break;
	  case ARRAY: src = getVarSource(node->right->name, node->right->left);
	    break;
	  default:
	    break;
	}
      }
      
      addNewQuadNode(mkNewQuadNode("=", src, "null", dest));
    } // end assignment
    
    if (strcmp(node->name, "procedure call") == 0) {
      char *param1 = getVarSource(node->left->left->left->name, node->left->left->left->left);
      
      
      char *dest = "global";
      if (isInLocal(node->left->left->left->name)) {
	dest = "local";
      }
      addNewQuadNode(mkNewQuadNode(node->name, node->left->name, param1, dest));
    }
    
    if (strcmp(node->name, "while") == 0) {
      addNewQuadNode(mkNewQuadNode(node->name, node->left->name, node->right->name, "dest"));
    }
    
    if (strcmp(node->name, "for") == 0) {
      addNewQuadNode(mkNewQuadNode(node->name, node->left->name, node->right->name, "dest"));
    }
    
    if ( (strcmp(node->name, "+") == 0) ||
      (strcmp(node->name, "-") == 0) ||
      (strcmp(node->name, "/") == 0) ||
      (strcmp(node->name, "*") == 0) ) {
      char *src1 = getVarSource(node->left->name, node->left->left);
      char *src2 = getVarSource(node->right->name, node->right->left);
      addNewQuadNode(mkNewQuadNode(node->name, src1, src2, "assignee"));
    }
    
    if ( (strcmp(node->name, "!") == 0) ||
      (strcmp(node->name, "Uminus") == 0) ) {
      addNewQuadNode(mkNewQuadNode(node->name, node->left->name, "null", "assignee"));
    }
    
    if ( (strcmp(node->name, "return") == 0) && 
      (node->left != NULL) ) {
      addNewQuadNode(mkNewQuadNode(node->name, node->left->name, "null", "global"));
      return;
    }
    
    if (strcmp(node->name, "if") == 0) {
      char *elseBlockName = "null";
      if (node->right != NULL) {
	elseBlockName = node->right->name;
      }
      addNewQuadNode(mkNewQuadNode(node->name, node->left->name, node->middle->name, elseBlockName));
      // walk parse trees on left, right, and middle until end stmt
      walkParseTree(node->left);
      walkParseTree(node->middle);
      if (strcmp(elseBlockName, "null") != 0) {
	walkParseTree(node->right);
      }
      return;
    }
    
    if (strcmp(node->name, "else") == 0) {
      addNewQuadNode(mkNewQuadNode(node->name, node->left->name, "null", "null"));
      walkParseTree(node->left);
      walkParseTree(node->right);
      return;
    }
    
    if(boolOperator(node->name)) {
      char *src1 = getVarSource(node->left->name, node->left->left);
      char *src2 = getVarSource(node->right->name, node->right->left);
      addNewQuadNode(mkNewQuadNode(node->name, src1, src2, "null"));
      return;
    }
    
    if (strcmp(node->name, "end stmt") == 0) {
      addNewQuadNode(mkNewQuadNode(node->name, "null", "null", "null"));
      return;
    }
    if (strcmp(node->name, "end for init") == 0) {
      addNewQuadNode(mkNewQuadNode(node->name, "null", "null", "null"));
      return;
    }
    
    
    if (strcmp(node->name, "func end") == 0) {
      addNewQuadNode(mkNewQuadNode(node->name, "local", "null", "global"));
      return;
    }
    
    if (node -> left != NULL) {
      walkParseTree(node -> left);
    }
    if (node -> middle != NULL) {
      walkParseTree(node -> middle);
    }
    if (node -> right != NULL) {
      walkParseTree(node -> right);
    }
    
  }
  
}

// calculates the number of bytes needed for a variable declaration
char * calcMemSize(struct list *node) {
  int size = 1;
  if (node -> vartype == ARRAY) {
    size = node -> arraysize; // number of variables is size of array, default is 1
  }
  if (node -> datatype == INT) {
    size = size * 4; // four bytes for an int, default 1 for char
  }
  
  char *size_as_string = (char*)malloc( 128 * sizeof(char) ); // too big
  sprintf(size_as_string, "%d", size);
  return size_as_string;
}

char * generateNewName(int counter) {
  char *number = (char*)malloc( 128 * sizeof(char) ); // too big
  sprintf(number, "%d", counter);
  char *newname = concat("const_", number);
  return newname;
}

// walks the global symbol table and creates quad nodes for declarations
void mapGlobalToQuad() { // only adds variables to the quad
  struct list *current = globalhead;
  while (current != NULL) {
    switch (current -> vartype) {
      case ARRAY: addNewQuadNode(mkNewQuadNode("decl", current->name, calcMemSize(current), "global"));
	break;
      case FUNC: 
	break;
      case VAR: addNewQuadNode(mkNewQuadNode("decl", current -> name, calcMemSize(current), "global"));
	break;
      case CONST: addNewQuadNode(mkNewQuadNode("const", current -> name, 0, current->value));
	break;
      default:
	break;
    }
    current = current -> next;
  }
  
}

// creates our quad list that will be used to generate assembly code
void createQuadList(struct parseTree *node) {
  if (quad_head != NULL) {
    free(quad_head);
  }
  quad_head = NULL;
  walkParseTree(node);
}

/*Cleanup functions
 * 
 */
// walks our parse tree and frees up the memory
void clearParseTree(struct parseTree *node) {
  if (node != NULL) {
    if (node -> left != NULL) {
      clearParseTree(node -> left);
    }
    if (node -> middle != NULL) {
      clearParseTree(node -> middle);
    }
    if (node -> right != NULL) {
      clearParseTree(node -> right);
    }
    
    free(node);
  }
  
}

// frees the memory of the quad list
void clearQuadList() {
  struct quad *currquad = quad_head;
  while(currquad != NULL) {
    quad_head = quad_head -> next;
    free(currquad);
    currquad = quad_head;
  }
}

// our main cleaning function
void clearData() {
  clearGlobal();
  clearLocal();
  clearProto();
  deleteParam();
  clearParseTree(parseTree_root);
  clearQuadList();
}
