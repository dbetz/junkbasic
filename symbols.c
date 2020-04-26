/* symbols.c - symbol table routines
 *
 * Copyright (c) 2020 by David Michael Betz.  All rights reserved.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "compile.h"

/* local function prototypes */
static Symbol *AddLocalSymbol(ParseContext *c, SymbolTable *table, const char *name, StorageClass storageClass, Type *type, VMVALUE value);
static Symbol *FindSymbol(SymbolTable *table, const char *name);

/* InitSymbolTable - initialize a symbol table */
void InitSymbolTable(SymbolTable *table)
{
    table->head = NULL;
    table->pTail = &table->head;
    table->count = 0;
}

/* AddGlobal - add a global symbol to the symbol table */
Symbol *AddGlobal(ParseContext *c, const char *name, StorageClass storageClass, Type *type, VMVALUE value)
{
    size_t size = sizeof(Symbol) + strlen(name);
    Symbol *sym;
    
    /* allocate the symbol structure */
    sym = (Symbol *)AllocateHighMemory(c->sys, size);
    strcpy(sym->name, name);
    sym->placed = VMFALSE;
    sym->storageClass = storageClass;
    sym->value = value;
    sym->next = NULL;

    /* add it to the symbol table */
    *c->globals.pTail = sym;
    c->globals.pTail = &sym->next;
    ++c->globals.count;
    
    /* return the symbol */
    return sym;
}

/* AddArgument - add an argument symbol to symbol table */
Symbol *AddArgument(ParseContext *c, const char *name, StorageClass storageClass, Type *type, VMVALUE value)
{
    return AddLocalSymbol(c, &c->currentFunction->u.functionDefinition.arguments, name, storageClass, type, value);
}

/* AddLocal - add a local symbol to the symbol table */
Symbol *AddLocal(ParseContext *c, const char *name, StorageClass storageClass, Type *type, VMVALUE value)
{
    return AddLocalSymbol(c, &c->currentFunction->u.functionDefinition.locals, name, storageClass, type, value);
}

/* AddLocalSymbol - add a symbol to a local symbol table */
static Symbol *AddLocalSymbol(ParseContext *c, SymbolTable *table, const char *name, StorageClass storageClass, Type *type, VMVALUE value)
{
    size_t size = sizeof(Symbol) + strlen(name);
    Symbol *sym;
    
    /* allocate the symbol structure */
    sym = (Symbol *)AllocateLowMemory(c->sys, size);
    strcpy(sym->name, name);
    sym->placed = VMTRUE;
    sym->storageClass = storageClass;
    sym->type = type;
    sym->value = value;
    sym->next = NULL;

    /* add it to the symbol table */
    *table->pTail = sym;
    table->pTail = &sym->next;
    ++table->count;
    
    /* return the symbol */
    return sym;
}

/* FindGlobal - find a global symbol */
Symbol *FindGlobal(ParseContext *c, const char *name)
{
    return FindSymbol(&c->globals, name);
}

/* FindArgument - find an argument symbol */
Symbol *FindArgument(ParseContext *c, const char *name)
{
    return FindSymbol(&c->currentFunction->u.functionDefinition.arguments, name);
}

/* FindLocal - find an local symbol */
Symbol *FindLocal(ParseContext *c, const char *name)
{
    return FindSymbol(&c->currentFunction->u.functionDefinition.locals, name);
}

/* FindSymbol - find a symbol in a symbol table */
static Symbol *FindSymbol(SymbolTable *table, const char *name)
{
    Symbol *sym = table->head;
    while (sym) {
        if (strcasecmp(name, sym->name) == 0)
            return sym;
        sym = sym->next;
    }
    return NULL;
}

/* DumpSymbols - dump a symbol table */
void DumpSymbols(SymbolTable *table, const char *tag)
{
    Symbol *sym;
    if ((sym = table->head) != NULL) {
        VM_printf("%s:\n", tag);
        for (; sym != NULL; sym = sym->next)
            VM_printf("  %s %08x\n", sym->name, sym->value);
    }
}
