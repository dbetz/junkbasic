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
    *pGetLine = sys->getLine;
    *pGetLineCookie = sys->getLineCookie;
}

/* SetMainSource - set the main source */
void SetMainSource(System *sys, GetLineHandler *getLine, void *getLineCookie)
{
    sys->getLine = getLine;
    sys->getLineCookie = getLineCookie;
}

/* GetLine - get the next input line */
int GetLine(System *sys, int *pLineNumber)
{
    if (!(*sys->getLine)(sys->lineBuf, sizeof(sys->lineBuf) - 1, pLineNumber, sys->getLineCookie))
        return VMFALSE;
    sys->linePtr = sys->lineBuf;
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
