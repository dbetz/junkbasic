/* parse.c - expression parser
 *
 * Copyright (c) 2020 by David Michael Betz.  All rights reserved.
 *
 */

#include <stdlib.h>
#include "compile.h"

/* local function prototypes */
static void EnterBuiltInSymbols(ParseContext *c);
static void EnterBuiltInVariable(ParseContext *c, const char *name, VMVALUE addr);
static void EnterBuiltInFunction(ParseContext *c, const char *name, VMVALUE addr);

/* InitCompileContext - initialize the compile (parse) context */
ParseContext *InitCompileContext(System *sys)
{
    ParseContext *c;
    
    if (!(c = InitParseContext(sys))
    ||  !(c->g = InitGenerateContext(sys)))
        VM_printf("insufficient memory");
    return c;
}
       
/* Compile - parse a program */
void Compile(ParseContext *c)
{
    /* setup an error target */
    if (setjmp(c->sys->errorTarget) != 0)
        return;

    /* initialize the string table */
    c->strings = NULL;

    /* initialize block nesting table */
    c->btop = (Block *)((char *)c->blockBuf + sizeof(c->blockBuf));
    c->bptr = &c->blockBuf[0] - 1;

    /* create the main function */
    c->mainFunction = StartFunction(c, NULL);
    
    /* initialize the global symbol table */
    InitSymbolTable(&c->globals);
    
    /* enter the built-in symbols */
    EnterBuiltInSymbols(c);

    /* initialize scanner */
    InitScan(c);
    
    /* parse the program */
    while (GetLine(c->sys, &c->lineNumber)) {
        int tkn;
        if ((tkn = GetToken(c)) != T_EOL)
            ParseStatement(c, tkn);
    }
    
    PrintNode(c->mainFunction, 0);
    
    /* generate code for the main function */
    Generate(c->g, c->mainFunction);
    
    DumpFunctions(c->g);
    DumpSymbols(&c->globals, "Globals");
}

/* EnterBuiltInSymbols - enter the built-in symbols */
static void EnterBuiltInSymbols(ParseContext *c)
{
    EnterBuiltInFunction(c, "printInt",  (VMVALUE)0);
    EnterBuiltInFunction(c, "printStr",  (VMVALUE)0);
    EnterBuiltInFunction(c, "printNL",  (VMVALUE)0);
}

/* EnterBuiltInVariable - enter a built-in variable */
static void EnterBuiltInVariable(ParseContext *c, const char *name, VMVALUE addr)
{
    AddGlobal(c, name, SC_VARIABLE, &c->integerType, addr);
}

/* EnterBuiltInFunction - enter a built-in function */
static void EnterBuiltInFunction(ParseContext *c, const char *name, VMVALUE addr)
{
    AddGlobal(c, name, SC_FUNCTION, &c->integerFunctionType, addr);
}
