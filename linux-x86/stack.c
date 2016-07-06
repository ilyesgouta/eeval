
/* Expression Evaluation Library, (C) Ilyes Gouta, 2007-2008. */

#include <stdio.h>
#include <assert.h>
#include <eevaltype.h>
#include <tokenizer.h>
#include <eevalint.h>

typedef struct {
    void* base; /* 32-bit machines only */
    int size;
} x86stackman;

int arch_init_stackman(eevalctx *pctx)
{
    return 0;
}

/* push the arguments on the stack just before the compiled code invocation */
int arch_prepare_stack(eevalctx *pctx)
{
    return 0;
}

int arch_get_stacksize(eevalctx *pctx)
{
}

void arch_free_stackman(eevalctx *pctx)
{
}
