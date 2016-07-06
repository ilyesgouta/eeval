
/* Expression Evaluation Library, (C) Ilyes Gouta, 2007-2008. */

#include <eevaltype.h>
#include <tokenizer.h>
#include <eevalint.h>

#define EMIT8(a) *((unsigned char*)pctx->genbuf_wptr) = (a); pctx->genbuf_wptr += 1;
#define EMIT16(a) *((unsigned short*)pctx->genbuf_wptr) = (a); pctx->genbuf_wptr += 2;
#define EMIT32(a) *((unsigned int*)pctx->genbuf_wptr) = (a); pctx->genbuf_wptr += 4;

#define MODRM(mod, reg, rm) ((rm & 0x7) | ((reg & 0x7) << 3) | ((mod & 0x3) << 6))

enum X86REG {
    EAX = 0,
    ECX = 1,
    EDX = 2,
    EBX = 3,
    ESP = 4, /* won't be used */
    EBP = 5, /* won't be used */
    ESI = 6,
    EDI = 7
};

int arch_emit_prologue(eevalctx *pctx)
{
    /*int s = arch_get_stacksize(pctx);
    
    arch_prepare_stack(pctx);
    
    __asm volatile ("mov %%eax, %0\n"
                    "call *%%eax\n" : : "r" (pctx->genbuf_alignedptr));
    */
    return 0;
}

int arch_emit_opcodes(eevalctx *pctx)
{
    return 0;
}

void arch_call_method(eevalctx *pctx)
{
}
