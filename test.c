
/* Expression Evaluation Library, (C) Ilyes Gouta, 2007-2008. */

#include <stdio.h>

#include <eeval.h>

int main(int argc, char** argv)
{
    EEVAL_ERROR result;
    EEVALCTX pctx;
    
    int a, b;
    
    pctx = eeval_initialize(argv[1], 0, 2);
    
    if (!pctx) {
        printf("failed to initialize a context.\n");
        return 0;
    }
    
    eeval_bind_identifier(pctx, 0, ARGUMENT_INT, &a);
    eeval_bind_identifier(pctx, 1, ARGUMENT_INT, &b);
    
    result = eeval_compile(pctx, 0);
    
    if (result != EEVAL_NOERROR)
        printf("error: %s (%d)\n", eeval_error_msg[result], result);
    else eeval_execute(pctx);
    
    eeval_free(pctx);
}
