
/* Expression Evaluation Library, (C) Ilyes Gouta, 2007-2008. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <eevaltype.h>
#include <tokenizer.h>
#include <eevalint.h>
#include <backend.h>

int yyerror(char* s)
{
}

EEVALCTX eeval_initialize(char* codebuf, int size, int args_count)
{
    eevalctx *pctx;
    identifier *id;
    
    if ((args_count < 0) || !codebuf || (size < 0))
        return NULL;
    
    pctx = (eevalctx*)malloc(sizeof(eevalctx));
    if (!pctx)
        return NULL;
    
    id = (identifier*)malloc(sizeof(identifier));
    if (!id) {
        free(pctx);
        return NULL;
    }
    
    memset(pctx, 0, sizeof(eevalctx));

    yylex_init(&pctx->scanner);
    yyset_in(fopen(codebuf, "r"), pctx->scanner); /* TODO: temporary hack, should take a memory pointer instead. */
    
    pctx->inbuffer = codebuf;
    pctx->inbuffer_size = size;
    
    /* Set our context as the extra word to be accessible later by flex and bison. */
    yyset_extra(pctx, pctx->scanner);
    
    initialize_identifiers(pctx);
    
    memset(&pctx->yylval, 0, sizeof(pctx->yylval));
    pctx->yylval.id = id;
    
    pctx->init_args_count = args_count;
    
    pctx->args_bindings = calloc(args_count, sizeof(binding_slot));
    pctx->args_type = calloc(args_count, sizeof(IDTYPE));
    
    if (!pctx->args_bindings || !pctx->args_type)
        return NULL;
    
    if (arch_init_regman(pctx))
        return NULL;
                
    if (arch_init_stackman(pctx))
        return NULL;
    
    return ((EEVALCTX)pctx);
}

EEVAL_ERROR eeval_bind_constint(EEVALCTX ctx, int slot, int constant)
{
    eevalctx* pctx = (eevalctx*)ctx;
    
    if (!pctx)
        return;

    if (slot >= pctx->init_args_count)
        return EEVAL_INVALID_SLOT;
    
    pctx->args_bindings[slot].ivalue = constant;
    pctx->args_type[slot] = CONST_INT_TYPE;

    return EEVAL_NOERROR;
}

EEVAL_ERROR eeval_bind_constfloat(EEVALCTX ctx, int slot, float constant)
{
    eevalctx* pctx = (eevalctx*)ctx;
    
    if (!pctx)
        return;

    if (slot >= pctx->init_args_count)
        return EEVAL_INVALID_SLOT;

    pctx->args_bindings[slot].fvalue = constant;
    pctx->args_type[slot] = CONST_FLOAT_TYPE;

    return EEVAL_NOERROR;
}

EEVAL_ERROR eeval_bind_identifier(EEVALCTX ctx, int slot, ARGUMENTTYPE type, void* identifier)
{
    eevalctx* pctx = (eevalctx*)ctx;
    
    if (!pctx)
        return;
    
    if (slot >= pctx->init_args_count)
        return EEVAL_INVALID_SLOT;
    
    switch (type) {
    case ARGUMENT_INT:
        pctx->args_bindings[slot].iptr = (int*)identifier;
        pctx->args_type[slot] = INT_TYPE;
        break;
    case ARGUMENT_FLOAT:
        pctx->args_bindings[slot].fptr = (float*)identifier;
        pctx->args_type[slot] = FLOAT_TYPE;
        break;
    case ARGUMENT_INT_PTR:
        pctx->args_bindings[slot].iptr = (int*)identifier;
        pctx->args_type[slot] = INT_PTR_TYPE;
        break;
    case ARGUMENT_FLOAT_PTR:
        pctx->args_bindings[slot].fptr = (float*)identifier;
        pctx->args_type[slot] = FLOAT_PTR_TYPE;
        break;
    default:
        return EEVAL_INVALID_SLOT;
    }

    return EEVAL_NOERROR;
}

EEVAL_ERROR eeval_compile(EEVALCTX ctx, int gencode_size)
{
    eevalctx* pctx = (eevalctx*)ctx;
    EEVAL_ERROR result;
    int i;
    
    if (!pctx)
        return EEVAL_BAD_ARGUMENT;
    
    result = (EEVAL_ERROR)yyparse(pctx);
    yylex_destroy(pctx->scanner);
    
    if (result == EEVAL_NOERROR) {
        pctx->ast_root = pctx->ast_stack[0]; /* This is the root of our AST. */
        
        if (pctx->init_args_count != pctx->args_count)
            return EEVAL_MISSING_ARGUMENTS;
        
        /* Bound arguments should have the same type as declared in the script to compile. */
        for (i = 0; i < pctx->args_count; i++) {
            identifier* arg = lookup_argument(pctx, i);
            if (!arg)
                return EEVAL_MISSING_ARGUMENTS; /* shouldn't happen */
            
            assert(arg->type & IS_ARG);
            
            if ((arg->type & ~IS_ARG) != pctx->args_type[i])
                return EEVAL_BAD_ARGUMENT_TYPE;
        }
        
        /* This handles the implicit types conversions */
        if (propagate_type(pctx))
        {
            /* Run the optimizer */
            arch_optimize_astree(pctx);
            
            if (!arch_bind_resources(pctx))
            {
                /* If it's OK then continue with code generation */
                pctx->genbuf_size = (gencode_size < 256) ? 256 : gencode_size;
                
                pctx->genbuf = (unsigned char*)malloc(pctx->genbuf_size + 15);
                pctx->genbuf_alignedptr = (void*)(((unsigned int)pctx->genbuf + 15L) & !15L); // 16 bytes aligned
                pctx->genbuf_wptr = pctx->genbuf_alignedptr;
                
                memset(pctx->genbuf, 0, pctx->genbuf_size);
                
                if (!pctx->genbuf)
                    return EEVAL_NOMEM;
            
                render_astree(pctx, "ast.dot");
                
                if (arch_emit_prologue(pctx))
                    return EEVAL_BAD_PROLOGUE;
                
                if (arch_emit_opcodes(pctx))
                    return EEVAL_EMIT_FAILED;
                
                return EEVAL_NOERROR;
            } else
                return EEVAL_BIND_FAILED;
        } else {
            return EEVAL_BAD_TYPE;
        }
    }
}

EEVAL_ERROR eeval_execute(EEVALCTX ctx)
{
    eevalctx* pctx = (eevalctx*)ctx;
    
    if (!pctx)
        return EEVAL_BAD_ARGUMENT;
    
    if (!pctx->regs_manager || !pctx->stack_manager)
        return EEVAL_BAD_STATE;
        
    arch_emit_prologue(pctx); /* update the prologue again if necessary, should always have the same size. */
    arch_call_method(pctx);
}

void eeval_free(EEVALCTX ctx)
{
    eevalctx* pctx = (eevalctx*)ctx;
    
    if (!pctx)
        return;

    free_identifiers(pctx);
    free_astree(pctx);
    
    if (pctx->genbuf) {
        free(pctx->genbuf);
        pctx->genbuf = NULL;
    }
    
    if (pctx->regs_manager) {
        arch_free_regman(pctx);
        pctx->regs_manager = NULL;
    }

    if (pctx->stack_manager) {
        arch_free_stackman(pctx);
        pctx->stack_manager = NULL;
    }
    
    if (pctx->args_bindings) {
        free(pctx->args_bindings);
        pctx->args_bindings = NULL;
    }
        
    if (pctx->args_type) {
        free(pctx->args_bindings);
        pctx->args_bindings = NULL;
    }
    
    if (pctx->yylval.id) {
        free(pctx->yylval.id);
        pctx->yylval.id = NULL;
    }
    
    free(pctx);
}
