
/* Expression Evaluation Library, (C) Ilyes Gouta, 2007. */

#ifndef __EEVAL__
#define __EEVAL__

typedef enum {ARGUMENT_INT = 1, ARGUMENT_FLOAT, ARGUMENT_INT_PTR, ARGUMENT_FLOAT_PTR, ARGUMENT_LAST = 0xffffffff} ARGUMENTTYPE;

typedef enum {
    EEVAL_NOERROR = 0,
    EEVAL_NOMEM,
    EEVAL_STACK_EXHAUSTED,
    EEVAL_UNRESOLVED_IDENTIFIER,
    EEVAL_INVALID_IDENTIFIER,
    EEVAL_INVALID_BINARY,
    EEVAL_INVALID_UNARY,
    EEVAL_INVALID_FETCH,
    EEVAL_INVALID_STORE,
    EEVAL_INVALID_CJMP,
    EEVAL_BAD_TYPE,
    EEVAL_BAD_ARGUMENT,
    EEVAL_MISSING_ARGUMENTS,
    EEVAL_INVALID_SLOT,
    EEVAL_UNRESOLVED_SPILLS,
    EEVAL_BAD_ARGUMENT_TYPE,
    EEVAL_BIND_FAILED,
    EEVAL_BAD_PROLOGUE,
    EEVAL_EMIT_FAILED,
    EEVAL_BAD_STATE
} EEVAL_ERROR;

static char* eeval_error_msg[] = {
    "No error",
    "Not enough memory",
    "Stack exhausted",
    "Undefined or duplicated identifier",
    "Invalid identifier",
    "Invalid binary operation",
    "Invalid unary operation",
    "Invalid array fetch operand",
    "Invalid array store operand",
    "Invalid conditional jump",
    "Bad type propagation",
    "Bad argument",
    "Missing arguments",
    "Invalid binding slot",
    "Unresolved spill locations",
    "Bad argument type",
    "Failed allocating registers",
    "Bad prologue code",
    "Instruction emitter failure",
    "Bad state"
};

typedef void* EEVALCTX;

EEVALCTX eeval_initialize(char* codebuf, int size, int args_count);
EEVAL_ERROR eeval_bind_constint(EEVALCTX ctx, int slot, int constant);
EEVAL_ERROR eeval_bind_constfloat(EEVALCTX ctx, int slot, float constant);
EEVAL_ERROR eeval_bind_identifier(EEVALCTX ctx, int slot, ARGUMENTTYPE type, void* identifier);
EEVAL_ERROR eeval_compile(EEVALCTX ctx, int gencode_size);
EEVAL_ERROR eeval_execute(EEVALCTX ctx);
void eeval_free(EEVALCTX ctx);

#endif
