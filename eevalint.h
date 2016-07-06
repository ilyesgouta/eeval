
/* Expression Evaluation Library, (C) Ilyes Gouta, 2007. */

#ifndef __EEVAL_INTERNAL__
#define __EEVAL_INTERNAL__

#include <eeval.h>

typedef union {
    int ivalue;
    float fvalue;
    int* iptr;
    float* fptr;
} binding_slot;

typedef struct eevalctx {
    char* inbuffer;
    int inbuffer_size;
    YYSTYPE yylval;
    yyscan_t scanner;
    astnode* ast_root;
    void* genbuf;
    void* genbuf_alignedptr;
    void* genbuf_wptr;
    int genbuf_size;
    int prologue_size; /* in bytes */
    int scope;
    IDTYPE decl_type;
    astnode* ast_stack[256];
    int stack_ptr; /* points to the top of the stack. */
    identifier id_list;
    int indirection_level;
    astnode array_dim;
    int args_count;
    int init_args_count;
    binding_slot* args_bindings;
    IDTYPE *args_type;
    void* regs_manager;
    void* stack_manager;
    char miscbuf[256];
} eevalctx;

void initialize_identifiers(eevalctx* pctx);
identifier* new_identifier(eevalctx* ptcx, char* name);
int get_identifiers_count(eevalctx* pctx);
void free_identifiers(eevalctx *pctx);

identifier* lookup(eevalctx* pctx, char* name);
identifier* lookup_new(eevalctx* pctx, char* name);
identifier* lookup_argument(eevalctx* pctx, int slot);

EEVAL_ERROR declare(eevalctx* pctx, char* name, IDTYPE type, int arg);
EEVAL_ERROR adeclare_cmpt(eevalctx* pctx, int i);
EEVAL_ERROR declare_array(eevalctx* pctx, IDTYPE type, int arg);
EEVAL_ERROR adeclare_reset(eevalctx* pctx);

astnode* new_astnode(eevalctx* pctx, NODETYPE type);
astnode* new_astconstnode(eevalctx* pctx, IDTYPE type, ...);

int propagate_type(eevalctx* pctx);
void free_astree(eevalctx* pctx);
void render_astree(eevalctx* pctx, char* fn);

#endif
