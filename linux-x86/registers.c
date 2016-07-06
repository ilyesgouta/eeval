
/* Expression Evaluation Library, (C) Ilyes Gouta, 2007-2008. */

#include <stdio.h>
#include <assert.h>
#include <eevaltype.h>
#include <tokenizer.h>
#include <eevalint.h>

// The x86 architecture has upto 6 or 7 useable generic registers that could be
// used to handle integer operations. The current implementation would only use 4 of
// these registers, namely EAX, EBX, ECX and EDX. This would increase the registers
// pressure but also help stick with the programming guidelines for the x86 since the
// rest of the registers (like ESI and EDI) aren't really designed for such operations.
// In this implementation, every identifier is backed by a memory location that serves
// as a dedicated spill location whenever it's required too.

#define REGS_COUNT 4
#define REG(a) (1L << (a))

#define STATUS_REG 16

typedef enum {
    REG_FREE = 0,
    REG_DISPOSABLE = 1, /* The register is bound to an indentifier but it's OK to assign it to something else. */
    REG_BOUND = 2
} reg_status;

typedef struct {
    eevalctx *pctx;
    struct {
        int pool_size;
        void* pool_ptr;
    } constant_pool;
    int* spillbuf; /* Offset buffer for every spill location. */
    identifier *id[REGS_COUNT]; /* Bound identifiers per register, provides a fast lookup of the registers status. */
    reg_status status[REGS_COUNT];
} x86regman;

static char* NodeTypeStr[] = {NULL, "Block", "Identifier", "Unary", "Binary", "Const Int", "Const Float", "String", "Index", "Jump", "Cond. Jump", "Reg. Spill", "Reg. Load"};
static char* BinaryOpStr[] = {NULL, "+", "-", "*", "/", "||", "&&", "|", "&", "^", "<=", "<", ">=", ">", "==", "!=", "=", "Fetch", "Store"};
static char* UnaryOpStr[] = {NULL, "-", "(int)", "(float)", "!", "~"};
static char* IdTypeStr[] = {NULL, "int", "float", "int*", "float*", "const int", "const float"};

#define DUMP_NODE(a) \
    do { \
        printf("Node type: %s, Operation: %s, Id: %s, Parent: 0x%08x\n", NodeTypeStr[a->nodetype], \
        (a->nodetype == BINARY) ? BinaryOpStr[a->binaryop] : (a->nodetype == UNARY) ? UnaryOpStr[a->unaryop] : NULL, \
        (a->nodetype == IDENT) ? a->id->name : NULL, a->parent); \
    } while (0);

int arch_init_regman(eevalctx *pctx)
{
    pctx->regs_manager = (x86regman*)calloc(1, sizeof(x86regman));

    return (pctx->regs_manager == NULL);
}

static void insert_spillnode(x86regman* regman, astnode *node, int reg)
{
    identifier *target = regman->id[reg]; /* Target identifier to be spilled. */
    astnode *spill = new_astnode(regman->pctx, SPILL_REG);

    if (!spill)
        return;

    assert(target);
    assert((node->id->type & ~IS_ARG) == INT_TYPE); /* Only integer identifiers are supported for now. */

    astnode* prev = node->parent;
    int is_left = (prev->left == node) ? 1 : 0;

    assert(prev);

    spill->left = node;
    spill->parent = prev;

    if (is_left)
        prev->left = spill;
    else
        prev->right = spill;

    node->parent = spill;

    spill->id = target;
    spill->ireg = reg;
    target->is_spilled = 1;
}

static int get_reg(x86regman *regman, astnode* node, int exclude_mask)
{
    int i;

    for (i = 1; i < REGS_COUNT; i++) {
        if (regman->status[i] == REG_FREE)
            return i;

        if (!(REG(i) & exclude_mask)) {
            if (regman->status[i] & REG_BOUND) {
                if (regman->id[i])
                    insert_spillnode(regman, node, i);
                return i;
            }
        }
    }

    return -1; /* No free registers */
}

static void recurse_assign_reg(astnode* node, x86regman* regman)
{   
    if (node->left)
        recurse_assign_reg(node->left, regman);

    if (node->right)
        recurse_assign_reg(node->right, regman);

    if (node->nodetype == BINARY) {
        if (node->binaryop <= XOR)
            node->ireg = node->left->ireg; /* x86: Inherit the left child register. */
        else
        if (node->binaryop <= CMPNE)
            node->ireg = STATUS_REG; /* Result of the operation is available in the CPU's status register. */
        else
        if (node->binaryop == ASSIGN)
            node->ireg = -1; /* Result will be written to the identifier. */
        else {
            DUMP_NODE(node);
            assert(0);
        }
    }

    if (node->nodetype == UNARY)
        node->ireg = node->left->ireg; /* x86: Inherit the left child register. */

    if (node->nodetype == IDENT)
    {
        int reg = get_reg(regman, node, REG(node->parent->ireg));

        assert(reg >= 0);

        /* We found a free register */
        node->ireg = reg;

        regman->id[reg] = node->id;
        regman->status[reg] |= REG_BOUND;
    }
}

int arch_bind_resources(eevalctx *pctx)
{
    int spillsize, i;
    x86regman* regman = (x86regman*)pctx->regs_manager;
    
    if (!regman)
        return -1;

    spillsize = get_identifiers_size(pctx);

    if (spillsize > 0) {
        regman->spillbuf = calloc(1, spillsize);
        if (!regman->spillbuf)
            return -1;
    }

    for (i = 0; i < REGS_COUNT; i++)
        regman->status[i] = REG_FREE;

    recurse_assign_reg(pctx->ast_root, regman);
    return 0;
}

void arch_free_regman(eevalctx *pctx)
{
    if (pctx->regs_manager) {
        x86regman* regman = (x86regman*)pctx->regs_manager;
        
        if (regman->spillbuf)
            free(regman->spillbuf);
            
        free(pctx->regs_manager);
        pctx->regs_manager = NULL;
    }
}
