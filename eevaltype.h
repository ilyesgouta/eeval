
/* Expression Eval Library, (C) Ilyes Gouta, 2007. */

#ifndef __EEVALTYPE__
#define __EEVALTYPE__

typedef struct genlist {
    void* prev;
    void* next;
} genlist;

void* genlist_add(void* head, int size);

typedef enum {BLOCK = 1, IDENT, UNARY, BINARY, CONST_INT, CONST_FLOAT, STRING, INDEX, JMP, CJMP, SPILL_REG, LOAD_REG} NODETYPE;
typedef enum {ADD = 1, SUB, MUL, DIV, LOGIC_OR, LOGIC_AND, ARITH_OR, ARITH_AND, XOR, CMPLE, CMPLT, CMPGE, CMPGT, CMPEQ, CMPNE, ASSIGN, EXPR_FETCH, EXPR_STORE} BINARYOP;
typedef enum {MINUS = 1, ICNVRT, FCNVRT, NOT, BITWISE_NOT} UNARYOP;

typedef enum {INT_TYPE = 1, FLOAT_TYPE, INT_PTR_TYPE, FLOAT_PTR_TYPE, CONST_INT_TYPE, CONST_FLOAT_TYPE, ID_LAST_TYPE = 0xffffffff} IDTYPE;

#define IS_ARG 0x100L

struct astnode;

typedef struct identifier {
    genlist list;
    char name[32];
    IDTYPE type;
    int scope;
    unsigned int pc;
    int is_spilled;
    int indirections_count;
    struct astnode* indirections;
    int array_size;
    int arg_position;
    union {
        int ivalue;
        float fvalue;
    } value;
} identifier;

typedef struct astnode {
    genlist list;
    struct astnode *parent;
    struct astnode *right;
    struct astnode *left;
    NODETYPE nodetype;
    identifier *id;
    int ireg; /* The associated arch's integer register#. */
    int freg; /* The associated arch's float register#. */
    BINARYOP binaryop;
    UNARYOP unaryop;
    IDTYPE propagated_type;
    union {
        int ivalue;
        float fvalue;
        char* buffer;
    } value;
} YYSTYPE, astnode;

#define new_nodelist(h) (astnode*)genlist_add((h), sizeof(astnode))

#endif
