/* system.h - global system context
 *
 * Copyright (c) 2020 by David Michael Betz.  All rights reserved.
 *
 */

#include <stdio.h>
#include <stdarg.h>
#include "system.h"

/* InitSystem - initialize the compiler */
System *InitSystem(uint8_t *freeSpace, size_t freeSize)
{
    System *sys = (System *)freeSpace;
    if (freeSize < sizeof(System))
        return NULL;
    sys->freeSpace = freeSpace + sizeof(System);
    sys->freeTop = freeSpace + freeSize;
    sys->nextLow = sys->freeSpace;
    sys->nextHigh = sys->freeTop;
    sys->heapSize = sys->freeTop - sys->freeSpace;
    sys->maxHeapUsed = 0;
    sys->currentFile = &sys->mainFile;
    return sys;
}

/* AllocateHighMemory - allocate high memory from the heap */
void *AllocateHighMemory(System *sys, size_t size)
{
    size = (size + ALIGN_MASK) & ~ALIGN_MASK;
    if (sys->nextHigh - size < sys->nextLow)
        Abort(sys, "insufficient memory");
    sys->nextHigh -= size;
    if (sys->heapSize - (sys->nextHigh - sys->nextLow) > sys->maxHeapUsed)
        sys->maxHeapUsed = sys->heapSize - (sys->nextHigh - sys->nextLow);
    return sys->nextHigh;
}

/* AllocateLowMemory - allocate low memory from the heap */
void *AllocateLowMemory(System *sys, size_t size)
{
    uint8_t *p = sys->nextLow;
    size = (size + ALIGN_MASK) & ~ALIGN_MASK;
    if (p + size > sys->nextHigh)
        Abort(sys, "insufficient memory");
    sys->nextLow += size;
    if (sys->heapSize - (sys->nextHigh - sys->nextLow) > sys->maxHeapUsed)
        sys->maxHeapUsed = sys->heapSize - (sys->nextHigh - sys->nextLow);
    return p;
}

/* GetMainSource - get the main source */
void GetMainSource(System *sys, GetLineHandler **pGetLine, void **pGetLineCookie)
{
    *pGetLine = sys->mainFile.u.main.getLine;
    *pGetLineCookie = sys->mainFile.u.main.getLineCookie;
}

/* SetMainSource - set the main source */
void SetMainSource(System *sys, GetLineHandler *getLine, void *getLineCookie)
{
    sys->mainFile.u.main.getLine = getLine;
    sys->mainFile.u.main.getLineCookie = getLineCookie;
}

/* PushFile - push a file onto the input file stack */
int PushFile(System *sys, const char *name)
{
    IncludedFile *inc;
    ParseFile *f;
    void *fp;
    
    /* check to see if the file has already been included */
    for (inc = sys->includedFiles; inc != NULL; inc = inc->next)
        if (strcmp(name, inc->name) == 0)
            return VMTRUE;
    
    /* add this file to the list of already included files */
    if (!(inc = (IncludedFile *)AllocateHighMemory(sys, sizeof(IncludedFile) + strlen(name))))
        Abort(sys, "insufficient memory");
    strcpy(inc->name, name);
    inc->next = sys->includedFiles;
    sys->includedFiles = inc;

    /* open the input file */
    if (!(fp = VM_open(sys, name, "r")))
        return VMFALSE;
    
    /* allocate a parse file structure */
    if (!(f = (ParseFile *)AllocateHighMemory(sys, sizeof(ParseFile))))
        Abort(sys, "insufficient memory");
    
    /* initialize the parse file structure */
    f->u.file.fp = fp;
    f->u.file.file = inc;
    f->u.file.lineNumber = 0;
    
    /* push the file onto the input file stack */
    f->next = sys->currentFile;
    sys->currentFile = f;
    
    /* return successfully */
    return VMTRUE;
}

/* GetLine - get the next input line */
int GetLine(System *sys, int *pLineNumber)
{
    ParseFile *f;
    int len;

    /* get the next input line */
    for (;;) {
        
        /* get a line from the main input */
        if ((f = sys->currentFile) == &sys->mainFile) {
            if ((*f->u.main.getLine)(sys->lineBuf, sizeof(sys->lineBuf) - 1, pLineNumber, f->u.main.getLineCookie))
                break;
            else
                return VMFALSE;
        }
        
        /* get a line from the current include file */
        else {
            if (VM_getline(sys->lineBuf, sizeof(sys->lineBuf) - 1, f->u.file.fp)) {
             	*pLineNumber = ++f->u.file.lineNumber;
               	break;
            }
            else {
                sys->currentFile = f->next;
                fclose(f->u.file.fp);
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

/* VM_printf - formatted print */
void VM_printf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    VM_vprintf(fmt, ap);
    va_end(ap);
}

void VM_vprintf(const char *fmt, va_list ap)
{
    char buf[80], *p = buf;
    vsprintf(buf, fmt, ap);
    while (*p != '\0')
        VM_putchar(*p++);
}

void Abort(System *sys, const char *fmt, ...)
{
    char buf[100], *p = buf;
    va_list ap;
    va_start(ap, fmt);
    VM_printf("error: ");
    vsprintf(buf, fmt, ap);
    while (*p != '\0')
        VM_putchar(*p++);
    VM_putchar('\n');
    va_end(ap);
    longjmp(sys->errorTarget, 1);
}

