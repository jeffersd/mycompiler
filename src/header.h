typedef enum {ARRAY, INT, CHAR, FUNC, VAR, VOID, STRING, CONST} type_t;


/*typedef struct treenode {
  type_t type;
  int arraysize;
  struct treenode *left;
  struct treenode *right;
} typenode, *typepointer;*/

typedef struct parseTree {
  char *name;
  type_t type;
  struct parseTree *left;
  struct parseTree *middle;
  struct parseTree *right;
  struct list *symlink;
} parsenode, *typepointer;