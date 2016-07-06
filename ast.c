
/* Expression Evaluation Library, (C) Ilyes Gouta, 2007-2008. */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <eevaltype.h>
#include <tokenizer.h>
#include <eevalint.h>

astnode* new_astnode(eevalctx* pctx, NODETYPE type)
{
    astnode* node = (astnode*)calloc(1, sizeof(astnode));
    if (!node) return NULL;
    
    node->nodetype = type;
    return (node);
}

astnode* new_astconstnode(eevalctx* pctx, IDTYPE type, ...)
{
    astnode* node = (astnode*)calloc(1, sizeof(astnode));
    if (!node) return NULL;
    
    node->nodetype = type;
    
    va_list list;
    va_start(list, type);
    
    if (type == INT_TYPE) {
        node->nodetype = CONST_INT;
        node->value.ivalue = va_arg(list, int);
        va_end(list);
    }
    
    if (type == FLOAT_TYPE) {
        node->nodetype = CONST_FLOAT;
        node->value.fvalue = (float)va_arg(list, double); /* float gets promoted to double in variable arguments */
        va_end(list);
    }
    
    return (node);
}

static int propagate_nodetype(eevalctx* pctx, astnode* node)
{
    int result = 1;
    
    while (node)
    {
        astnode* cnvtnode;
        astnode* next = (astnode*)node->list.next;
        
        if (node->left && (node->nodetype != JMP))
            result = propagate_nodetype(pctx, node->left);

        if (result && node->right)
            result = propagate_nodetype(pctx, node->right);
        
        if (!result)
            return 0;
        
        switch (node->nodetype) {
        case IDENT:
            node->propagated_type = node->id->type & ~IS_ARG;
            break;
        case CONST_INT:
            node->propagated_type = INT_TYPE;
            break;
        case CONST_FLOAT:
            node->propagated_type = FLOAT_TYPE;
            break;
        case BINARY:
            if (node->left->propagated_type != node->right->propagated_type)
                if (node->binaryop == ASSIGN)
                    node->propagated_type = node->left->propagated_type;
                else
                    node->propagated_type = FLOAT_TYPE;
            else
                node->propagated_type = node->left->propagated_type;
            
            switch (node->binaryop) {
            case ASSIGN:
            case ADD:
            case SUB:
            case MUL:
            case DIV:
                if (node->left->propagated_type != node->propagated_type) {
                    cnvtnode = new_astnode(pctx, UNARY);
                    if (!cnvtnode)
                        return 0;
                    
                    cnvtnode->unaryop = (node->propagated_type == INT_TYPE) ? ICNVRT : FCNVRT;
                    cnvtnode->left = node->left;
                    cnvtnode->parent = node;
                    
                    node->left->parent = cnvtnode;
                    node->left = cnvtnode;
                }
                
                if (node->right->propagated_type != node->propagated_type) {
                    cnvtnode = new_astnode(pctx, UNARY);
                    if (!cnvtnode)
                        return 0;
                    
                    cnvtnode->unaryop = (node->propagated_type == INT_TYPE) ? ICNVRT : FCNVRT;
                    cnvtnode->left = node->right;
                    cnvtnode->parent = node;
                    
                    node->right->parent = cnvtnode;
                    node->right = cnvtnode;
                }
                break;
            default: /* These are the integer-only operators. */
                if ((node->left->propagated_type != INT_TYPE) ||
                    (node->right->propagated_type != INT_TYPE))
                    return 0;
            }
            break;
        default:
            break;
        }

        node = next;
    }
    
    return (result);
}

int propagate_type(eevalctx* pctx)
{
    return (pctx->ast_root ? propagate_nodetype(pctx, pctx->ast_root) : 0);
}

static void free_astnode(astnode* node)
{
    while (node)
    {
        astnode* next = (astnode*)node->list.next;
        
        if (node->left && (node->nodetype != JMP))
            free_astnode(node->left);
        if (node->right && (node->right->nodetype != JMP))
            free_astnode(node->right);

        free(node);
        node = next;
    }
}

void free_astree(eevalctx* pctx)
{
    if (pctx->ast_root)
        free_astnode(pctx->ast_root);
}

static char* binaryop_map[] = {NULL, "+", "-", "*", "/", "||", "&&", "|", "&", "^", "<=", "<", ">=", ">", "==", "!=", "=", "FETCH", "STORE"};
static char* unaryop_map[] = {NULL, "-", "(int)", "(float)", "!", "~"};

static void dump_nodename(astnode *node, FILE* output, char* buf)
{
    char text[256];

    switch (node->nodetype) {
    case BLOCK:
        sprintf(buf, "\t{ node [label=\"BLOCK\"] %d; }\n", node);
        fputs(buf, output);
        break;
    case BINARY:
        sprintf(buf, "\t{ node [label=\"%s\"] %d; }\n", binaryop_map[node->binaryop], node);
        fputs(buf, output);
        break;
    case JMP:
        sprintf(buf, "\t{ node [label=\"JMP\"] %d; }\n", node);
        fputs(buf, output);
        break;
    case CJMP:
        sprintf(buf, "\t{ node [label=\"CJMP\"] %d; }\n", node);
        fputs(buf, output);
    case UNARY:
        sprintf(buf, "\t{ node [label=\"%s\"] %d; }\n", unaryop_map[node->unaryop], node);
        fputs(buf, output);
        break;
    case CONST_INT:
        sprintf(buf, "\t{ node [label=\"%d\"] %d; }\n", node->value.ivalue, node);
        fputs(buf, output);
        break;
    case CONST_FLOAT:
        sprintf(buf, "\t{ node [label=\"%f\"] %d; }\n", node->value.fvalue, node);
        fputs(buf, output);
        break;
    case IDENT:
        sprintf(text, "%s (reg: %d)", node->id ? node->id->name : "null", node->ireg);
        sprintf(buf, "\t{ node [label=\"%s\"] %d; }\n", text, node);
        fputs(buf, output);
        break;
    case INDEX:
        sprintf(buf, "\t{ node [label=\"INDEX\"] %d; }\n", node);
        fputs(buf, output);
        break;
    case SPILL_REG:
        sprintf(buf, "\t{ node [label=\"SPILL (reg: %d)\"] %d; }\n", node->ireg, node);
        fputs(buf, output);
        break;
    case LOAD_REG:
        sprintf(buf, "\t{ node [label=\"LOAD (reg: %d)\"] %d; }\n", node, node->ireg);
        fputs(buf, output);
        break;
    }
}

static void render_node(astnode* node, FILE* output, char* buf)
{
    while (node) {
        astnode* next = (astnode*)node->list.next;
        
        dump_nodename(node, output, buf);
        
        if (node->left) {
            dump_nodename(node->left, output, buf);
            sprintf(buf, "\t%d -> %d\n", node, node->left);
            fputs(buf, output);
            if (node->nodetype != JMP)
                render_node(node->left, output, buf);
        }
        
        if (node->right) {
            dump_nodename(node->right, output, buf);
            sprintf(buf, "\t%d -> %d\n", node, node->right);
            fputs(buf, output);
            if (node->nodetype != JMP)
                render_node(node->right, output, buf);
        }
        
        if (next) {
            dump_nodename(next, output, buf);
            sprintf(buf, "\t%d -> %d\n", node, next);
            fputs(buf, output);
        }
        
        node = next;
    }
}

void render_astree(eevalctx* pctx, char* fn)
{
    FILE *f = fopen(fn, "wb");
    char buf[256];
    
    fputs("digraph AST {\n", f);
    
    if (pctx->ast_root)
        render_node(pctx->ast_root, f, buf);
        
    fputs("}\n", f);

    fclose(f);
}
