/* compile.c - compiler
 *
 * Copyright (c) 2020 by David Michael Betz.  All rights reserved.
 *
 */

#include <stdlib.h>
#include "compile.h"
#include "vmint.h"

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
    VMVALUE mainCode;
    Interpreter *i;
    Symbol *symbol;
    
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
    
    /* initialize scanner */
    InitScan(c);
    
    /* parse the program */
    while (ParseGetLine(c)) {
        int tkn;
        if ((tkn = GetToken(c)) != T_EOL)
            ParseStatement(c, tkn);
    }
    
    PrintNode(c->mainFunction, 0);
    
    /* generate code for the main function */
    mainCode = Generate(c->g, c->mainFunction);
    
    /* store all implicitly declared global variables */
    for (symbol = c->globals.head; symbol != NULL; symbol = symbol->next) {
        if (symbol->storageClass == SC_VARIABLE && !symbol->placed) {
            VMVALUE value = 0;
            VMVALUE addr = StoreVector(c->g, &value, 1);
            PlaceSymbol(c->g, symbol, addr);
        }
    }

    DumpFunctions(c->g);
    DumpSymbols(&c->globals, "Globals");
    DumpStrings(c);
    
    if (!(i = InitInterpreter(c->sys, c->g->codeBuf, 1024)))
        VM_printf("insufficient memory");
    else {
        Execute(i, mainCode);
    }
}

/* PushFile - push a file onto the input file stack */
int PushFile(ParseContext *c, const char *name)
{
    System *sys = c->sys;
    IncludedFile *inc;
    ParseFile *f;
    void *fp;
    
    /* check to see if the file has already been included */
    for (inc = c->includedFiles; inc != NULL; inc = inc->next)
        if (strcmp(name, inc->name) == 0)
            return VMTRUE;
    
    /* add this file to the list of already included files */
    if (!(inc = (IncludedFile *)AllocateHighMemory(sys, sizeof(IncludedFile) + strlen(name))))
        Abort(sys, "insufficient memory");
    strcpy(inc->name, name);
    inc->next = c->includedFiles;
    c->includedFiles = inc;

    /* open the input file */
    if (!(fp = VM_open(sys, name, "r")))
        return VMFALSE;
    
    /* allocate a parse file structure */
    if (!(f = (ParseFile *)AllocateHighMemory(sys, sizeof(ParseFile))))
        Abort(sys, "insufficient memory");
    
    /* initialize the parse file structure */
    f->fp = fp;
    f->file = inc;
    f->lineNumber = 0;
    
    /* push the file onto the input file stack */
    f->next = c->currentFile;
    c->currentFile = f;
    
    /* return successfully */
    return VMTRUE;
}

/* ParseGetLine - get the next input line */
int ParseGetLine(ParseContext *c)
{
    System *sys = c->sys;
    ParseFile *f;
    int len;

    /* get the next input line */
    for (;;) {
        
        /* get a line from the main input */
        if (!(f = c->currentFile)) {
            if (GetLine(sys, sys->getLineCookie))
                break;
            else
                return VMFALSE;
        }
        
        /* get a line from the current include file */
        else {
            if (VM_getline(sys->lineBuf, sizeof(sys->lineBuf) - 1, f->fp)) {
             	c->lineNumber = ++f->lineNumber;
               	break;
            }
            else {
                c->currentFile = f->next;
                fclose(f->fp);
            }
        }        
    }
    
    /* make sure the line is correctly terminated */
    len = strlen(sys->lineBuf);
    if (len == 0 || sys->lineBuf[len - 1] != '\n') {
        sys->lineBuf[len++] = '\n';
        sys->lineBuf[len] = '\0';
    }

    /* initialize the input buffer */
    sys->linePtr = sys->lineBuf;
    
    /* return successfully */
    return VMTRUE;
}

