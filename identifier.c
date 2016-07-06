
/* Expression Evaluation Library, (C) Ilyes Gouta, 2007. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#include <eevaltype.h>
#include <tokenizer.h>
#include <eevalint.h>

EEVAL_ERROR push_node(eevalctx* pctx, astnode* node);
astnode* pop_node(eevalctx* pctx);
astnode* peek_node(eevalctx* pctx);

void* genlist_add(void* head, int size)
{
    genlist *headptr = (genlist*)head;
    
    void* node = malloc(size);
    if (!node)
        return NULL;

    genlist *ptr = (genlist*)node;
    
    ptr->next = headptr->next;
    ((genlist*)headptr->next)->prev = ptr;
    headptr->next = ptr;
    ptr->prev = headptr;
    
    return (ptr);
}

void genlist_free(void* head)
{
    genlist *headptr = ((genlist*)head)->next;
    
    while (headptr != (genlist*)head) {
        genlist* tmp = headptr->next;
        free(headptr);
        headptr = tmp;
    }
    
    ((genlist*)head)->prev = head;
    ((genlist*)head)->next = head;
}

void genlist_reset(void* head)
{
    ((genlist*)head)->prev = head;
    ((genlist*)head)->next = head;
}

void initialize_identifiers(eevalctx* pctx)
{
    pctx->id_list.list.prev = &pctx->id_list;
    pctx->id_list.list.next = &pctx->id_list;
    
    pctx->array_dim.list.prev = &pctx->array_dim.list;
    pctx->array_dim.list.next = &pctx->array_dim.list;
}

int get_identifiers_count(eevalctx* pctx)
{
    identifier *id = pctx->id_list.list.next;
    int x = 0;

    while (id != &pctx->id_list) {
        id = (identifier*)id->list.next;
        x++;
    }
    
    return (x);
}

int get_identifiers_size(eevalctx* pctx)
{
    identifier *id = pctx->id_list.list.next;
    int size = 0;

    /* ints, floats are 4 bytes. */
    while (id != &pctx->id_list) {
        size += ((id->type == INT_PTR_TYPE || id->type == FLOAT_PTR_TYPE) ? id->array_size : 1) * 4;
        id = (identifier*)id->list.next;
    }

    return (size);
}

int get_identifier_offset(eevalctx* pctx, identifier* target)
{
    identifier *id = pctx->id_list.list.next;
    int offset = 0;

    /* ints, floats are 4 bytes. */
    while ((id != target) && (id != &pctx->id_list)) {
        offset += ((id->type == INT_PTR_TYPE || id->type == FLOAT_PTR_TYPE) ? id->array_size : 1);
        id = (identifier*)id->list.next;
    }
    
    if (id == &pctx->id_list) /* Not found! */
        return -1;

    return (offset * 4);
}

identifier* new_identifier(eevalctx* pctx, char* name)
{
    identifier* id = (identifier*)malloc(sizeof(identifier));
    
    memset(id, 0, sizeof(identifier));

    id->list.next = (identifier*)pctx->id_list.list.next;
    pctx->id_list.list.next = id;
    id->list.prev = &pctx->id_list;

    strncpy(id->name, name, sizeof(id->name) - 1);
    id->scope = pctx->scope;

    return (id);
}

identifier* lookup(eevalctx* pctx, char* name)
{
    identifier *id;
    int scope = pctx->scope;

    while (scope > 0)
    {
        id = (identifier*)pctx->id_list.list.next;

        while (id != &pctx->id_list) {
            if ((id->scope == scope) && !strcmp(id->name, name))
                return (id);

            id = (identifier*)id->list.next;
        }

        scope--;
    }

    return NULL;
}

identifier* lookup_scoped(eevalctx* pctx, char* name, int level)
{
    identifier *id;
    int scope = level;

    while (scope > 0)
    {
        id = (identifier*)pctx->id_list.list.next;

        while (id != &pctx->id_list) {
            if ((id->scope == scope) && !strcmp(id->name, name))
                return (id);

            id = (identifier*)id->list.next;
        }

        scope--;
    }

    return NULL;
}

identifier* lookup_new(eevalctx* pctx, char* name)
{
    identifier *id = pctx->id_list.list.next;

    while (id != &pctx->id_list)
    {
        if ((id->scope == pctx->scope) && !strcmp(id->name, name))
            return (id);

        id = (identifier*)id->list.next;
    }

    return NULL;
}

identifier* lookup_argument(eevalctx* pctx, int slot)
{
    identifier *id = pctx->id_list.list.next;

    while (id != &pctx->id_list) {
        if ((id->arg_position == (slot + 1)) && (id->type & IS_ARG))
            return (id);
        id = (identifier*)id->list.next;
    }

    return NULL;
}

void free_identifiers(eevalctx* pctx)
{
   identifier *id = pctx->id_list.list.next;

    while (id != &pctx->id_list) {
        identifier* next = (identifier*)id->list.next;
        free(id);
        id = next;
    }
}

EEVAL_ERROR declare(eevalctx* pctx, char* name, IDTYPE type, int arg)
{
    if (arg)
        pctx->scope++; /* HACK: increase the scope temporarely for arguments. */

    if (lookup_new(pctx, name)) {
        if (arg)
            pctx->scope--;
        return EEVAL_INVALID_IDENTIFIER;
    }
    
    identifier* id = new_identifier(pctx, name);

    if (!id) {
        if (arg)
            pctx->scope--;
        return EEVAL_NOMEM;
    }
    
    id->type = type;
    
    if (type == INT_TYPE)
        id->value.ivalue = 0;
    else id->value.fvalue = 0; /* is it enough to initialize ivalue? */
    
    if (arg) {
        id->arg_position = ++pctx->args_count; /* arguments start at position 1 */
        pctx->scope--;
        id->type |= IS_ARG;
    }
    
    return EEVAL_NOERROR;
}

EEVAL_ERROR adeclare_cmpt(eevalctx* pctx, int i)
{
    astnode* node = new_nodelist(&pctx->array_dim);
    
    if (!node)
        return EEVAL_NOMEM;

    node->value.ivalue = i;
    node->nodetype = CONST_INT;

    pctx->indirection_level++;
    
    return EEVAL_NOERROR;
}

EEVAL_ERROR declare_array(eevalctx* pctx, IDTYPE type, int arg)
{
    int size = 1;
    astnode* ptr;
    
    ptr = pop_node(pctx);
    assert(ptr->nodetype == STRING);
    
    if (arg)
        pctx->scope++; /* HACK: increase the scope temporarely for arguments. */
    
    if (lookup_new(pctx, ptr->value.buffer)) {
        if (arg)
            pctx->scope--;
        free(ptr->value.buffer);
        return EEVAL_INVALID_IDENTIFIER;
    }

    identifier* id = new_identifier(pctx, ptr->value.buffer);

    if (!id) {
        if (arg)
            pctx->scope--;
        return EEVAL_NOMEM;
    }
    
    free(ptr->value.buffer);
    
    ptr = (astnode*)pctx->array_dim.list.next;

    while (&ptr->list != &pctx->array_dim.list) {
        size *= ptr->value.ivalue;
        ptr = (astnode*)ptr->list.next;
    }

    /* Break the linked list. */
    ((genlist*)pctx->array_dim.list.prev)->next = NULL;
    
    id->type = (type == INT_TYPE) ? INT_PTR_TYPE : FLOAT_PTR_TYPE;
    id->indirections = (astnode*)pctx->array_dim.list.next;
    id->indirections_count = pctx->indirection_level;
    id->array_size = size;
    
    if (arg) {
        id->arg_position = ++pctx->args_count; /* arguments start at position 1 */
        pctx->scope--;
        id->type |= IS_ARG;
    }
    
    adeclare_reset(pctx);
    
    return EEVAL_NOERROR;
}

EEVAL_ERROR adeclare_reset(eevalctx* pctx)
{
    genlist_reset(&pctx->array_dim);
    pctx->indirection_level = 0;
}
