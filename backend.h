
/* Expression Evaluation Library, (C) Ilyes Gouta, 2007-2008. */

#ifndef __BACKEND__
#define __BACKEND__

/* Implemented in $ARCH/optimizer.c */
void arch_optimize_astree(eevalctx *pctx);

/* Implemented in $ARCH/registers.c */
int arch_init_regman(eevalctx *pctx); /* returns 0 on success */
int arch_bind_resources(eevalctx *pctx); /* returns 0 on success */
void arch_free_regman(eevalctx *pctx);
void arch_spillnode(eevalctx *pctx, astnode* node);

/* Implemented in $ARCH/stack.c */
int arch_init_stackman(eevalctx *pctx); /* returns 0 on success */
int arch_prepare_stack(eevalctx *pctx);
int arch_get_stacksize(eevalctx *pctx);
void arch_free_stackman(eevalctx *pctx);

/* Implemented in $ARCH/opcodes.c */
int arch_emit_prologue(eevalctx *pctx); /* returns 0 on success */
int arch_emit_opcodes(eevalctx *pctx); /* returns 0 on success */

#endif
